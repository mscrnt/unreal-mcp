"""
Project Tools for Unreal MCP.

This module provides tools for managing project-wide settings and configuration.
"""

import logging
from typing import Dict, Any
from mcp.server.fastmcp import FastMCP, Context

# Get logger
logger = logging.getLogger("UnrealMCP")

def register_project_tools(mcp: FastMCP):
    """Register project tools with the MCP server."""
    
    @mcp.tool()
    def create_input_mapping(
        ctx: Context,
        action_name: str,
        key: str,
        input_type: str = "Action"
    ) -> Dict[str, Any]:
        """
        Create an input mapping for the project.
        
        Args:
            action_name: Name of the input action
            key: Key to bind (SpaceBar, LeftMouseButton, etc.)
            input_type: Type of input mapping (Action or Axis)
            
        Returns:
            Response indicating success or failure
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "action_name": action_name,
                "key": key,
                "input_type": input_type
            }
            
            logger.info(f"Creating input mapping '{action_name}' with key '{key}'")
            response = unreal.send_command("create_input_mapping", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Input mapping creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error creating input mapping: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    # ============================================================
    # Phase 6: Project Settings & Game Framework
    # ============================================================

    @mcp.tool()
    def set_default_game_mode(
        ctx: Context,
        game_mode_class: str
    ) -> Dict[str, Any]:
        """
        Set the default game mode for the project.

        Args:
            game_mode_class: Class path or name of the game mode (e.g. "/Script/MyGame.MyGameMode")
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("set_default_game_mode", {
                "game_mode_class": game_mode_class
            })
            return response or {}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_default_map(
        ctx: Context,
        map_path: str,
        map_type: str = "game"
    ) -> Dict[str, Any]:
        """
        Set the default map for the project.

        Args:
            map_path: Content path to the map (e.g. "/Game/Maps/MainMenu")
            map_type: "game" for game default map or "editor" for editor startup map
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("set_default_map", {
                "map_path": map_path,
                "map_type": map_type
            })
            return response or {}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def create_enhanced_input_action(
        ctx: Context,
        name: str,
        value_type: str = "Boolean",
        path: str = "/Game/Input"
    ) -> Dict[str, Any]:
        """
        Create an Enhanced Input Action asset.

        Args:
            name: Name for the input action (will be prefixed with IA_)
            value_type: Value type - Boolean, Axis1D, Axis2D, or Axis3D
            path: Content browser path for the asset
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("create_enhanced_input_action", {
                "name": name,
                "value_type": value_type,
                "path": path
            })
            return response or {}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def create_input_mapping_context(
        ctx: Context,
        name: str,
        path: str = "/Game/Input",
        mappings: list = None
    ) -> Dict[str, Any]:
        """
        Create an Input Mapping Context asset with optional key mappings.

        Args:
            name: Name for the mapping context (will be prefixed with IMC_)
            path: Content browser path for the asset
            mappings: Optional list of mappings, each with "action" (path) and "key" (name)
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            params = {"name": name, "path": path}
            if mappings:
                params["mappings"] = mappings
            response = unreal.send_command("create_input_mapping_context", params)
            return response or {}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_project_setting(
        ctx: Context,
        section: str,
        key: str,
        value: str
    ) -> Dict[str, Any]:
        """
        Set a project configuration setting (DefaultGame.ini).

        Args:
            section: INI section (e.g. "/Script/Engine.GameSession")
            key: Setting key
            value: Setting value
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("set_project_setting", {
                "section": section,
                "key": key,
                "value": value
            })
            return response or {}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_project_setting(
        ctx: Context,
        section: str,
        key: str
    ) -> Dict[str, Any]:
        """
        Get a project configuration setting value.

        Args:
            section: INI section (e.g. "/Script/Engine.GameSession")
            key: Setting key
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("get_project_setting", {
                "section": section,
                "key": key
            })
            return response or {}
        except Exception as e:
            return {"success": False, "message": str(e)}

    logger.info("Project tools registered successfully")