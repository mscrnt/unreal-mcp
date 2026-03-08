"""
Tool Scope Manager for Unreal MCP.

Controls which tool groups appear in tools/list dynamically, reducing context
window usage by only exposing tools relevant to the current task.

Scopes and their approximate tool counts (after merges):
  editor          ~22 tools  - Actor CRUD, viewport, screenshots, editor utilities
  assets          ~10 tools  - List/find/manage content browser assets
  level           ~ 9 tools  - Levels, PIE, console, lighting, world settings, execute_python
  process         ~ 5 tools  - Start/stop editor, cache management
  blueprint       ~12 tools  - Create BPs, components, compile, introspection, metadata
  blueprint_nodes ~18 tools  - Node graph authoring, function management
  materials       ~11 tools  - Material creation, expressions, params, get_material_info
  animation       ~ 7 tools  - AnimBP, state machines, transitions
  umg             ~ 6 tools  - Widget blueprints, buttons, text blocks
  project         ~ 7 tools  - Game mode, input, project settings
  worldbuilding   ~ 3 tools  - Procedural structures, buildings, infrastructure
  user            ~ ? tools  - Auto-discovered from UserTools/*.py

Default config (tool_scopes.json) activates: editor, assets, level, process (~46 tools).
Agents can activate additional scopes at runtime with activate_tool_scope().
"""

import asyncio
import logging
import json
import os
from typing import Any, Dict, List, Set

logger = logging.getLogger("UnrealMCP")

# FastMCP instance (set at startup)
_mcp = None

# scope_name -> {tool_name: FunctionTool}
_scope_tools: Dict[str, Dict[str, Any]] = {}

# Currently active scopes
_active_scopes: Set[str] = set()

# Count of always-on tools (scope management tools registered directly on mcp)
_always_on_count: int = 0


def _count_active_tools() -> int:
    """Count currently active tools from our own tracking data."""
    scoped = sum(len(tools) for scope, tools in _scope_tools.items() if scope in _active_scopes)
    return scoped + _always_on_count


def init(mcp_instance, default_scopes: List[str]) -> None:
    """Initialize with the FastMCP instance and the list of scopes to activate by default."""
    global _mcp, _active_scopes
    _mcp = mcp_instance
    _active_scopes = set(default_scopes)
    logger.info(f"Scope manager initialized. Default active scopes: {sorted(default_scopes)}")


def set_always_on_count(count: int) -> None:
    """Set the number of always-on tools (registered outside of scopes)."""
    global _always_on_count
    _always_on_count = count


def register_scope(scope_name: str, register_fn) -> List[str]:
    """
    Register a tool module as a named scope.

    Calls register_fn(mcp) which registers tools via @mcp.tool().
    Tools are then stored in the scope registry. If the scope is not in the
    default active set, tools are immediately removed so they do not appear
    in tools/list.

    Returns the sorted list of tool names registered in this scope.
    """
    global _mcp, _scope_tools, _active_scopes

    lp = _mcp.local_provider

    # Snapshot tool names before registration
    before = {t.name for t in asyncio.run(lp.list_tools())}
    register_fn(_mcp)
    after = {t.name for t in asyncio.run(lp.list_tools())}
    new_names = after - before

    # Store tool objects so we can re-add them later
    _scope_tools[scope_name] = {
        name: asyncio.run(lp.get_tool(name))
        for name in new_names
    }

    if scope_name not in _active_scopes:
        for name in new_names:
            lp.remove_tool(name)
        logger.info(f"Scope '{scope_name}' registered — {len(new_names)} tools (inactive)")
    else:
        logger.info(f"Scope '{scope_name}' registered — {len(new_names)} tools (active)")

    return sorted(new_names)


def activate(scope_name: str) -> Dict[str, Any]:
    """Add a scope's tools to the active tool listing."""
    if scope_name not in _scope_tools:
        available = sorted(_scope_tools.keys())
        return {
            "success": False,
            "message": f"Unknown scope '{scope_name}'. Available: {available}"
        }
    if scope_name in _active_scopes:
        return {"success": False, "message": f"Scope '{scope_name}' is already active"}

    tools = _scope_tools[scope_name]
    for name, tool in tools.items():
        _mcp.add_tool(tool)
    _active_scopes.add(scope_name)

    logger.info(f"Activated scope '{scope_name}' (+{len(tools)} tools)")
    return {
        "success": True,
        "scope": scope_name,
        "tools_added": sorted(tools.keys()),
        "total_active_tools": _count_active_tools(),
    }


def deactivate(scope_name: str) -> Dict[str, Any]:
    """Remove a scope's tools from the active tool listing."""
    PROTECTED = {"core"}
    if scope_name in PROTECTED:
        return {"success": False, "message": f"Scope '{scope_name}' cannot be deactivated"}
    if scope_name not in _active_scopes:
        return {"success": False, "message": f"Scope '{scope_name}' is not currently active"}

    tools = _scope_tools.get(scope_name, {})
    lp = _mcp.local_provider
    for name in tools:
        try:
            lp.remove_tool(name)
        except Exception:
            pass
    _active_scopes.discard(scope_name)

    logger.info(f"Deactivated scope '{scope_name}' (-{len(tools)} tools)")
    return {
        "success": True,
        "scope": scope_name,
        "tools_removed": sorted(tools.keys()),
        "total_active_tools": _count_active_tools(),
    }


def list_scopes() -> Dict[str, Any]:
    """Return all scopes, their active status, tool counts, and tool names."""
    return {
        "active_scopes": sorted(_active_scopes),
        "total_active_tools": _count_active_tools(),
        "scopes": {
            scope: {
                "active": scope in _active_scopes,
                "tool_count": len(tools),
                "tools": sorted(tools.keys()),
            }
            for scope, tools in sorted(_scope_tools.items())
        },
    }


def load_config(config_path: str) -> List[str]:
    """Load default scopes from a JSON config file. Returns the default scope list."""
    if not os.path.exists(config_path):
        logger.warning(f"Scope config not found at {config_path}, using all scopes")
        return []
    try:
        with open(config_path) as f:
            config = json.load(f)
        scopes = config.get("default_scopes", [])
        logger.info(f"Loaded scope config from {config_path}: {scopes}")
        return scopes
    except Exception as e:
        logger.error(f"Failed to load scope config: {e}")
        return []
