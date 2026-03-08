"""
Process Tools for Unreal MCP.

OS-level tools for managing the Unreal Editor process lifecycle.

Detection strategy (in priority order):
  1. TCP port check — works in ALL environments (stdio, Docker, Git Bash, WSL)
  2. UnrealConnection state — if main connection is alive, editor is up
  3. OS process check — tasklist / PowerShell (may fail in non-native shells)

Shutdown strategy (in priority order):
  1. TCP command: unreal.SystemLibrary.quit_editor() — clean, no save dialog
  2. OS process kill: taskkill / Stop-Process — forceful fallback
"""

import logging
import os
import platform
import subprocess
import socket
import time
from pathlib import Path
from typing import Dict, Any, Optional
from fastmcp import FastMCP, Context

logger = logging.getLogger("UnrealMCP")

# Detect container mode (Linux + SSE transport = likely Docker)
_IS_CONTAINER = os.environ.get("MCP_TRANSPORT") == "sse" and platform.system() != "Windows"

# Cached paths discovered from a running editor process
_cached_editor_path: Optional[str] = None
_cached_project_path: Optional[str] = None


def _get_mcp_host_port() -> tuple:
    """Get the MCP host and port from environment or defaults."""
    host = os.environ.get("UNREAL_HOST", "127.0.0.1")
    port = int(os.environ.get("UNREAL_PORT", "55557"))
    return host, port


def _is_port_open(host: str = None, port: int = None, timeout: float = 2.0) -> bool:
    """Check if the MCP TCP port is accepting connections.
    This is the most reliable detection method — works in all environments."""
    if host is None or port is None:
        h, p = _get_mcp_host_port()
        host = host or h
        port = port or p
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(timeout)
            s.connect((host, port))
            return True
    except (socket.timeout, ConnectionRefusedError, OSError):
        return False


def _is_editor_process_running() -> bool:
    """Check if UnrealEditor.exe process exists at the OS level.
    May fail in non-native shells (Git Bash, WSL). Use _is_port_open() as primary."""
    if _IS_CONTAINER:
        return False  # Can't detect host processes from inside Docker

    # Method 1: tasklist
    try:
        result = subprocess.run(
            ['tasklist', '/FI', 'IMAGENAME eq UnrealEditor.exe', '/NH'],
            capture_output=True, text=True, timeout=10
        )
        if 'UnrealEditor.exe' in result.stdout:
            return True
    except Exception:
        pass

    # Method 2: PowerShell Get-Process
    try:
        result = subprocess.run(
            ['powershell', '-NoProfile', '-Command',
             '(Get-Process -Name UnrealEditor -ErrorAction SilentlyContinue) -ne $null'],
            capture_output=True, text=True, timeout=10
        )
        if result.stdout.strip().lower() == 'true':
            return True
    except Exception:
        pass

    return False


def _is_editor_running() -> bool:
    """Check if the Unreal Editor is running using all available methods.
    TCP port check is primary; OS process check is supplementary."""
    # Primary: TCP port check (works everywhere)
    if _is_port_open():
        return True
    # Fallback: OS process check (may fail in some shells)
    return _is_editor_process_running()


def _kill_editor_process() -> bool:
    """Attempt to kill the editor process at the OS level.
    Returns True if successful."""
    if _IS_CONTAINER:
        return False

    # Method 1: taskkill
    try:
        result = subprocess.run(
            ['taskkill', '/F', '/IM', 'UnrealEditor.exe'],
            capture_output=True, text=True, timeout=30
        )
        if result.returncode == 0:
            return True
    except Exception:
        pass

    # Method 2: PowerShell Stop-Process
    try:
        result = subprocess.run(
            ['powershell', '-NoProfile', '-Command',
             'Stop-Process -Name UnrealEditor -Force -ErrorAction SilentlyContinue; $true'],
            capture_output=True, text=True, timeout=30
        )
        if result.stdout.strip().lower() == 'true':
            return True
    except Exception:
        pass

    return False


def _quit_via_tcp() -> bool:
    """Send quit command to the editor via TCP bridge.
    Most reliable shutdown method — no save dialog, works in all environments."""
    try:
        from unreal_mcp_server import get_unreal_connection
        conn = get_unreal_connection()
        if conn and conn.connected:
            result = conn.send_command("execute_python", {
                "code": "import unreal; unreal.SystemLibrary.quit_editor()"
            })
            logger.info("Sent quit_editor() via TCP bridge")
            return True
    except Exception as e:
        logger.warning(f"TCP quit failed: {e}")
    return False


def _get_running_editor_info() -> Dict[str, Optional[str]]:
    """Get the executable path and project path from a running UnrealEditor process."""
    if _IS_CONTAINER:
        return {"editor_path": None, "project_path": None}
    try:
        result = subprocess.run(
            ['powershell', '-NoProfile', '-Command',
             'Get-CimInstance Win32_Process -Filter "name=\'UnrealEditor.exe\'" | '
             'Select-Object ProcessId, ExecutablePath, CommandLine | '
             'ConvertTo-Json'],
            capture_output=True, text=True, timeout=15
        )
        if result.returncode == 0 and result.stdout.strip():
            import json
            data = json.loads(result.stdout)
            if isinstance(data, list):
                data = data[0]

            editor_path = data.get("ExecutablePath")
            cmd_line = data.get("CommandLine", "")

            project_path = None
            import re
            matches = re.findall(r'"([^"]+\.uproject)"', cmd_line)
            if matches:
                project_path = matches[0]
            else:
                for token in cmd_line.split():
                    if token.endswith('.uproject'):
                        project_path = token
                        break

            return {"editor_path": editor_path, "project_path": project_path}
    except Exception as e:
        logger.warning(f"Could not get running editor info: {e}")
    return {"editor_path": None, "project_path": None}


def _find_uproject_file() -> Optional[str]:
    """Find a .uproject file relative to this Python server's directory."""
    repo_root = Path(__file__).resolve().parent.parent.parent
    for uproject in sorted(repo_root.glob("*/*.uproject")):
        return str(uproject)
    for uproject in sorted(repo_root.glob("*.uproject")):
        return str(uproject)
    return None


def _find_ue_editor() -> Optional[str]:
    """Try to find UnrealEditor.exe from common install locations."""
    common_bases = [
        r"C:\Program Files\Epic Games",
        r"D:\Epic Games",
        r"C:\Epic Games",
        r"E:\Epic Games",
    ]
    for base in common_bases:
        if os.path.isdir(base):
            try:
                for item in sorted(os.listdir(base), reverse=True):
                    editor = os.path.join(base, item, "Engine", "Binaries", "Win64", "UnrealEditor.exe")
                    if os.path.exists(editor):
                        return editor
            except OSError:
                continue
    return None


def _reset_tcp_connection():
    """Reset the cached TCP connection so the next tool call reconnects."""
    try:
        import unreal_mcp_server
        if unreal_mcp_server._unreal_connection:
            unreal_mcp_server._unreal_connection.disconnect()
            unreal_mcp_server._unreal_connection = None
    except Exception:
        pass


def register_process_tools(mcp: FastMCP):
    """Register process lifecycle tools with the MCP server."""

    @mcp.tool()
    def stop_unreal_editor(
        ctx: Context,
        save: bool = True
    ) -> Dict[str, Any]:
        """
        Stop the Unreal Editor process. Caches the editor and project paths
        so start_unreal_editor can relaunch without explicit paths.

        Args:
            save: If True, attempts to save all levels before stopping (requires active connection)
        """
        global _cached_editor_path, _cached_project_path

        if _IS_CONTAINER:
            return {
                "success": False,
                "message": "stop_unreal_editor is not available in container mode. "
                           "The Unreal Editor process runs on the host OS and cannot be "
                           "controlled from inside the Docker container."
            }

        if not _is_editor_running():
            return {"success": False, "message": "Unreal Editor is not running"}

        # Cache paths from the running process before stopping
        info = _get_running_editor_info()
        if info["editor_path"]:
            _cached_editor_path = info["editor_path"]
        if info["project_path"]:
            _cached_project_path = info["project_path"]

        # Try to save before stopping
        if save:
            try:
                from unreal_mcp_server import get_unreal_connection
                conn = get_unreal_connection()
                if conn and conn.connected:
                    conn.send_command("save_all_levels", {})
                    logger.info("Saved all levels before stopping editor")
            except Exception as e:
                logger.warning(f"Could not save before stopping: {e}")

        # Strategy 1: Graceful quit via TCP (most reliable, no save dialog)
        tcp_quit_sent = _quit_via_tcp()

        if tcp_quit_sent:
            # Wait for editor to exit after TCP quit
            for i in range(20):  # Up to 20 seconds
                time.sleep(1)
                if not _is_port_open(timeout=1.0):
                    # Port closed = editor shut down
                    _reset_tcp_connection()
                    return {
                        "success": True,
                        "message": "Unreal Editor stopped (graceful quit)",
                        "method": "tcp_quit_editor",
                        "cached_editor_path": _cached_editor_path,
                        "cached_project_path": _cached_project_path,
                    }

            # TCP quit was sent but editor hasn't closed yet — fall through to force kill
            logger.warning("TCP quit_editor sent but editor still running after 20s, trying force kill")

        # Strategy 2: Force kill via OS (fallback)
        try:
            killed = _kill_editor_process()
            if killed:
                # Wait for process to fully exit
                for _ in range(10):
                    if not _is_port_open(timeout=1.0) and not _is_editor_process_running():
                        break
                    time.sleep(1)

            _reset_tcp_connection()

            if not _is_port_open(timeout=1.0):
                return {
                    "success": True,
                    "message": "Unreal Editor stopped (force kill)",
                    "method": "process_kill",
                    "cached_editor_path": _cached_editor_path,
                    "cached_project_path": _cached_project_path,
                }
            else:
                return {
                    "success": False,
                    "message": "Editor process did not exit after TCP quit and force kill attempts. "
                               "The port is still responding. Try closing the editor manually."
                }
        except Exception as e:
            return {"success": False, "message": f"Failed to stop editor: {e}"}

    @mcp.tool()
    def start_unreal_editor(
        ctx: Context,
        project_path: str = "",
        editor_path: str = "",
        wait_for_connection: bool = True,
        timeout: int = 120
    ) -> Dict[str, Any]:
        """
        Start the Unreal Editor. Auto-discovers paths from a previous
        stop_unreal_editor call, or from common install locations.

        Args:
            project_path: Path to the .uproject file (auto-discovered if empty)
            editor_path: Path to UnrealEditor.exe (auto-discovered if empty)
            wait_for_connection: If True, waits until the MCP TCP server is ready
            timeout: Max seconds to wait for the editor to be ready (default 120)
        """
        global _cached_editor_path, _cached_project_path

        if _IS_CONTAINER:
            return {
                "success": False,
                "message": "start_unreal_editor is not available in container mode. "
                           "Launch the Unreal Editor on the host OS before starting the container."
            }

        if _is_editor_running():
            port_open = _is_port_open()
            if port_open:
                return {"success": True, "message": "Unreal Editor is already running and MCP is connected"}
            else:
                return {
                    "success": True,
                    "message": "Unreal Editor is running but MCP TCP port is not yet available. "
                               "The plugin may still be loading."
                }

        # Resolve project path
        resolved_project = project_path or _cached_project_path or _find_uproject_file()
        if not resolved_project:
            return {
                "success": False,
                "message": "Could not find .uproject file. Provide project_path explicitly."
            }
        if not os.path.exists(resolved_project):
            return {
                "success": False,
                "message": f"Project file not found: {resolved_project}"
            }

        # Resolve editor path
        resolved_editor = editor_path or _cached_editor_path or _find_ue_editor()
        if not resolved_editor:
            return {
                "success": False,
                "message": "Could not find UnrealEditor.exe. Provide editor_path explicitly."
            }
        if not os.path.exists(resolved_editor):
            return {
                "success": False,
                "message": f"Editor executable not found: {resolved_editor}"
            }

        # Cache for future use
        _cached_editor_path = resolved_editor
        _cached_project_path = resolved_project

        # Launch the editor
        try:
            subprocess.Popen(
                [resolved_editor, resolved_project],
                creationflags=subprocess.DETACHED_PROCESS | subprocess.CREATE_NEW_PROCESS_GROUP
            )
            logger.info(f"Launched Unreal Editor: {resolved_editor} {resolved_project}")
        except Exception as e:
            return {"success": False, "message": f"Failed to launch editor: {e}"}

        if not wait_for_connection:
            return {
                "success": True,
                "message": "Unreal Editor launched. Use is_unreal_editor_running to check status.",
                "editor_path": resolved_editor,
                "project_path": resolved_project
            }

        # Wait for the MCP TCP server to become available
        start_time = time.time()
        while time.time() - start_time < timeout:
            if _is_port_open():
                _reset_tcp_connection()
                # Clear any previous crash state
                try:
                    from unreal_mcp_server import clear_crash_state
                    clear_crash_state()
                except ImportError:
                    pass
                return {
                    "success": True,
                    "message": "Unreal Editor is running and MCP connection is ready",
                    "editor_path": resolved_editor,
                    "project_path": resolved_project,
                    "startup_time_seconds": round(time.time() - start_time, 1)
                }
            time.sleep(3)

        # Timed out
        if _is_editor_process_running():
            return {
                "success": False,
                "message": f"Editor is running but MCP TCP port not available after {timeout}s. "
                           "The plugin may not be loaded or may have failed to start.",
                "editor_path": resolved_editor,
                "project_path": resolved_project
            }
        else:
            return {
                "success": False,
                "message": "Editor process exited unexpectedly. Check the UE log for errors.",
                "editor_path": resolved_editor,
                "project_path": resolved_project
            }

    @mcp.tool()
    def is_unreal_editor_running(ctx: Context) -> Dict[str, Any]:
        """
        Check if the Unreal Editor process is currently running
        and whether the MCP TCP connection is available.
        """
        if _IS_CONTAINER:
            host, port = _get_mcp_host_port()
            port_open = _is_port_open(host=host, port=port)
            return {
                "running": "unknown (container mode)",
                "mcp_connected": port_open,
                "note": "Process detection unavailable in container mode. "
                        "TCP connectivity to UE plugin is the only check available."
            }

        # Always check TCP first (most reliable)
        port_open = _is_port_open()
        # Also check OS process (supplementary info)
        process_running = _is_editor_process_running()

        result = {
            "running": port_open or process_running,
            "mcp_connected": port_open,
            "cached_editor_path": _cached_editor_path,
            "cached_project_path": _cached_project_path
        }

        # If editor is not running, check for crash
        if not port_open and not process_running:
            try:
                from unreal_mcp_server import check_for_editor_crash
                crash_info = check_for_editor_crash()
                if crash_info:
                    result["crashed"] = True
                    result["crash_info"] = crash_info
            except ImportError:
                pass

        return result

    @mcp.tool()
    def clear_mcp_cache(ctx: Context) -> Dict[str, Any]:
        """
        Clear the in-memory MCP result cache immediately.

        Use this when you have made changes outside of MCP (e.g. manually creating
        or deleting assets in the Unreal Editor) and want to ensure the next tool
        call returns fresh data rather than a cached result.

        Also returns cache hit/miss statistics from the current session.
        """
        from mcp_cache import cache_clear, cache_stats
        stats_before = cache_stats()
        removed = cache_clear()
        return {
            "success": True,
            "entries_cleared": removed,
            "session_stats": stats_before
        }

    @mcp.tool()
    def get_mcp_cache_stats(ctx: Context) -> Dict[str, Any]:
        """
        Return cache hit/miss statistics and current entry counts.
        Useful for understanding whether caching is helping in a session.
        """
        from mcp_cache import cache_stats
        return cache_stats()

    logger.info("Process tools registered successfully")
