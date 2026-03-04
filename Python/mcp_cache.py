"""
Simple TTL cache for Unreal MCP tool results.

Usage:
    from mcp_cache import cache_get, cache_set, cache_invalidate, cache_stats

    # Read (returns None on miss or expiry)
    result = cache_get("assets:list:/Game/")
    if result is not None:
        return result

    # ... do actual work ...

    # Write
    cache_set("assets:list:/Game/", result, ttl=30.0)
    return result

    # Invalidate by key prefix (call after any mutation)
    cache_invalidate("assets:")   # clears all asset keys
    cache_invalidate("actors:")   # clears all actor keys
"""

import time
import logging
from threading import Lock
from typing import Any, Dict, List, Optional, Tuple

logger = logging.getLogger("UnrealMCP")

# TTL defaults per category (seconds)
TTL_ASSETS = 30.0   # asset registry — stable, slow to query
TTL_ACTORS = 5.0    # actor list — changes frequently, but short TTL is safe
TTL_BPS    = 30.0   # blueprint/material structure — stable until compiled

_cache: Dict[str, Tuple[Any, float]] = {}   # key -> (value, expires_at)
_lock  = Lock()
_hits  = 0
_misses = 0


def cache_get(key: str) -> Optional[Any]:
    """Return cached value or None if missing/expired."""
    global _hits, _misses
    with _lock:
        entry = _cache.get(key)
        if entry is not None:
            value, expires_at = entry
            if time.monotonic() < expires_at:
                _hits += 1
                return value
            # expired — remove it
            del _cache[key]
        _misses += 1
        return None


def cache_set(key: str, value: Any, ttl: float = TTL_ASSETS) -> None:
    """Store value with a TTL."""
    with _lock:
        _cache[key] = (value, time.monotonic() + ttl)


def cache_invalidate(*prefixes: str) -> int:
    """Delete all entries whose keys start with any of the given prefixes.
    Returns number of entries removed."""
    with _lock:
        to_delete = [k for k in _cache if any(k.startswith(p) for p in prefixes)]
        for k in to_delete:
            del _cache[k]
        if to_delete:
            logger.debug(f"Cache: invalidated {len(to_delete)} entries for prefixes {prefixes}")
        return len(to_delete)


def cache_clear() -> int:
    """Remove all entries. Returns count removed."""
    global _hits, _misses
    with _lock:
        count = len(_cache)
        _cache.clear()
        _hits = 0
        _misses = 0
        return count


def cache_stats() -> Dict[str, Any]:
    """Return cache statistics."""
    with _lock:
        now = time.monotonic()
        valid   = sum(1 for _, exp in _cache.values() if exp > now)
        expired = len(_cache) - valid
        total   = _hits + _misses
        return {
            "entries_valid":   valid,
            "entries_expired": expired,
            "hits":   _hits,
            "misses": _misses,
            "hit_rate": round(_hits / total, 3) if total > 0 else 0.0,
        }
