"""
Process Tools for Unreal MCP.

OS-level tools for managing the Unreal Editor process lifecycle.
These tools operate independently of the TCP connection — they use
subprocess/taskkill to start and stop the editor process.
"""

import logging
import os
import subprocess
import socket
import time
from pathlib import Path
from typing import Dict, Any, Optional
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("UnrealMCP")

# Cached paths discovered from a running editor process
_cached_editor_path: Optional[str] = None
_cached_project_path: Optional[str] = None


def _is_editor_running() -> bool:
    """Check if UnrealEditor.exe is currently running."""
    try:
        result = subprocess.run(
            ['tasklist', '/FI', 'IMAGENAME eq UnrealEditor.exe', '/NH'],
            capture_output=True, text=True, timeout=10
        )
        return 'UnrealEditor.exe' in result.stdout
    except Exception:
        return False


def _get_running_editor_info() -> Dict[str, Optional[str]]:
    """Get the executable path and project path from a running UnrealEditor process."""
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
            # PowerShell returns a single object or array
            if isinstance(data, list):
                data = data[0]

            editor_path = data.get("ExecutablePath")
            cmd_line = data.get("CommandLine", "")

            # Extract .uproject path from command line
            project_path = None
            # Check for quoted paths first
            import re
            matches = re.findall(r'"([^"]+\.uproject)"', cmd_line)
            if matches:
                project_path = matches[0]
            else:
                # Check unquoted
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
    # Go up from tools/ -> Python/ -> repo root
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


def _is_port_open(host: str = "127.0.0.1", port: int = 55557, timeout: float = 2.0) -> bool:
    """Check if the MCP TCP port is accepting connections."""
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(timeout)
            s.connect((host, port))
            return True
    except (socket.timeout, ConnectionRefusedError, OSError):
        return False


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

        if not _is_editor_running():
            return {"success": False, "message": "Unreal Editor is not running"}

        # Cache paths from the running process before killing it
        info = _get_running_editor_info()
        if info["editor_path"]:
            _cached_editor_path = info["editor_path"]
        if info["project_path"]:
            _cached_project_path = info["project_path"]

        # Try to save before stopping
        if save:
            try:
                from unreal_mcp_server import get_unreal_connection
                unreal = get_unreal_connection()
                if unreal:
                    unreal.send_command("save_all_levels", {})
                    logger.info("Saved all levels before stopping editor")
            except Exception as e:
                logger.warning(f"Could not save before stopping: {e}")

        # Kill the process
        try:
            subprocess.run(
                ['taskkill', '/F', '/IM', 'UnrealEditor.exe'],
                capture_output=True, text=True, timeout=30
            )

            # Wait for process to exit (usually immediate with /F)
            for _ in range(15):
                if not _is_editor_running():
                    break
                time.sleep(1)

            if _is_editor_running():
                return {"success": False, "message": "Editor process did not exit after 15 seconds"}

            # Reset the cached TCP connection
            try:
                import unreal_mcp_server
                if unreal_mcp_server._unreal_connection:
                    unreal_mcp_server._unreal_connection.disconnect()
                    unreal_mcp_server._unreal_connection = None
            except Exception:
                pass

            return {
                "success": True,
                "message": "Unreal Editor stopped",
                "cached_editor_path": _cached_editor_path,
                "cached_project_path": _cached_project_path,
                "note": "Use start_unreal_editor to relaunch. Cached paths will be used automatically."
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

        if _is_editor_running():
            if _is_port_open():
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
                # Reset connection state so next tool call creates a fresh connection
                try:
                    import unreal_mcp_server
                    if unreal_mcp_server._unreal_connection:
                        unreal_mcp_server._unreal_connection.disconnect()
                        unreal_mcp_server._unreal_connection = None
                except Exception:
                    pass

                return {
                    "success": True,
                    "message": "Unreal Editor is running and MCP connection is ready",
                    "editor_path": resolved_editor,
                    "project_path": resolved_project,
                    "startup_time_seconds": round(time.time() - start_time, 1)
                }
            time.sleep(3)

        # Timed out but editor may still be loading
        if _is_editor_running():
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
        running = _is_editor_running()
        port_open = _is_port_open() if running else False

        return {
            "running": running,
            "mcp_connected": port_open,
            "cached_editor_path": _cached_editor_path,
            "cached_project_path": _cached_project_path
        }

    logger.info("Process tools registered successfully")
