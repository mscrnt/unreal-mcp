"""
Level Tools for Unreal MCP.

This module provides tools for level/world management including creating, loading,
saving levels, Play-In-Editor control, console commands, and world settings.
"""

import logging
from typing import Dict, List, Any, Optional
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("UnrealMCP")


def register_level_tools(mcp: FastMCP):
    """Register level and world management tools with the MCP server."""

    @mcp.tool()
    def new_level(
        ctx: Context,
        asset_path: str = "",
        template: str = ""
    ) -> Dict[str, Any]:
        """
        Create a new level, optionally from a template.

        Args:
            asset_path: Path to save the new level (e.g. "/Game/Maps/MyLevel")
            template: Optional template level path
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            params = {"asset_path": asset_path}
            if template:
                params["template"] = template
            response = unreal.send_command("new_level", params)
            return response or {}
        except Exception as e:
            logger.error(f"Error creating new level: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def load_level(ctx: Context, level_path: str) -> Dict[str, Any]:
        """
        Load an existing level.

        Args:
            level_path: Content path of the level (e.g. "/Game/Maps/MyLevel")
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("load_level", {"level_path": level_path})
            return response or {}
        except Exception as e:
            logger.error(f"Error loading level: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def save_level(ctx: Context, save_all: bool = False) -> Dict[str, Any]:
        """
        Save the current level. Pass save_all=True to save all modified levels.

        Args:
            save_all: If True, saves all dirty levels; if False (default), saves only the current level
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            cmd = "save_all_levels" if save_all else "save_level"
            response = unreal.send_command(cmd, {})
            return response or {}
        except Exception as e:
            logger.error(f"Error saving level: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_current_level(ctx: Context) -> Dict[str, Any]:
        """Get the name and path of the currently loaded level."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("get_current_level", {})
            return response or {}
        except Exception as e:
            logger.error(f"Error getting current level: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def play_in_editor(
        ctx: Context,
        action: str = "start"
    ) -> Dict[str, Any]:
        """
        Control the Play-In-Editor (PIE) session.

        Args:
            action: One of:
              "start"  — begin a PIE session
              "stop"   — end the current PIE session
              "query"  — check whether PIE is currently active (returns {"is_playing": bool})
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            action = action.lower().strip()
            cmd_map = {
                "start": "play_in_editor",
                "stop":  "stop_play_in_editor",
                "query": "is_playing",
            }
            cmd = cmd_map.get(action)
            if cmd is None:
                return {"success": False, "message": f"Invalid action '{action}'. Use 'start', 'stop', or 'query'."}

            response = unreal.send_command(cmd, {})
            return response or {}
        except Exception as e:
            logger.error(f"Error in play_in_editor({action}): {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def execute_console_command(ctx: Context, command: str) -> Dict[str, Any]:
        """
        Execute a console command in the Unreal Editor.

        Args:
            command: The console command to execute (e.g. "stat fps", "r.SetRes 1920x1080")
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("execute_console_command", {"command": command})
            return response or {}
        except Exception as e:
            logger.error(f"Error executing console command: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def build_lighting(
        ctx: Context,
        quality: str = "Production",
        with_reflection_captures: bool = False
    ) -> Dict[str, Any]:
        """
        Build lighting for the current level.

        Args:
            quality: Lighting quality — Preview, Medium, High, or Production
            with_reflection_captures: Whether to also build reflection captures
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("build_lighting", {
                "quality": quality,
                "with_reflection_captures": with_reflection_captures
            })
            return response or {}
        except Exception as e:
            logger.error(f"Error building lighting: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_world_settings(
        ctx: Context,
        game_mode: str = None,
        kill_z: float = None
    ) -> Dict[str, Any]:
        """
        Set world settings for the current level.

        Args:
            game_mode: Game mode class path or name to set as default
            kill_z: Z coordinate below which actors are killed
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            params = {}
            if game_mode is not None:
                params["game_mode"] = game_mode
            if kill_z is not None:
                params["kill_z"] = kill_z
            response = unreal.send_command("set_world_settings", params)
            return response or {}
        except Exception as e:
            logger.error(f"Error setting world settings: {e}")
            return {"success": False, "message": str(e)}

    logger.info("Level tools registered successfully")
