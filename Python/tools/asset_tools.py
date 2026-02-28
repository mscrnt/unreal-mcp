"""
Asset Tools for Unreal MCP.

This module provides tools for managing content assets: listing, finding,
duplicating, deleting, renaming, importing, and saving assets.
"""

import logging
from typing import Dict, List, Any, Optional
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("UnrealMCP")


def register_asset_tools(mcp: FastMCP):
    """Register asset management tools with the MCP server."""

    @mcp.tool()
    def list_assets(
        ctx: Context,
        path: str = "/Game",
        class_filter: str = "",
        recursive: bool = True
    ) -> Dict[str, Any]:
        """
        List assets in a content browser directory.

        Args:
            path: Content path to list (e.g. "/Game/Blueprints")
            class_filter: Optional class name to filter by (e.g. "Blueprint", "Material")
            recursive: Whether to search subdirectories (default True)
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            params = {"path": path, "recursive": recursive}
            if class_filter:
                params["class_filter"] = class_filter
            response = unreal.send_command("list_assets", params)
            return response or {}
        except Exception as e:
            logger.error(f"Error listing assets: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def find_asset(ctx: Context, name: str) -> Dict[str, Any]:
        """
        Search for assets by name across the project.

        Args:
            name: Name or partial name to search for
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("find_asset", {"name": name})
            return response or {}
        except Exception as e:
            logger.error(f"Error finding asset: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def does_asset_exist(ctx: Context, path: str) -> Dict[str, Any]:
        """
        Check if an asset exists at the given path.

        Args:
            path: Full content path of the asset (e.g. "/Game/Materials/M_Red")
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("does_asset_exist", {"path": path})
            return response or {}
        except Exception as e:
            logger.error(f"Error checking asset existence: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def duplicate_asset(
        ctx: Context,
        source_path: str,
        dest_path: str
    ) -> Dict[str, Any]:
        """
        Duplicate an asset to a new path.

        Args:
            source_path: Path of the asset to duplicate
            dest_path: Destination path for the duplicate
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("duplicate_asset", {
                "source_path": source_path,
                "dest_path": dest_path
            })
            return response or {}
        except Exception as e:
            logger.error(f"Error duplicating asset: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def delete_asset(ctx: Context, path: str) -> Dict[str, Any]:
        """
        Delete an asset at the given path.

        Args:
            path: Full content path of the asset to delete
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            # Use delete_asset_file to avoid collision with editor_tools delete_actor
            response = unreal.send_command("delete_asset_file", {"path": path})
            return response or {}
        except Exception as e:
            logger.error(f"Error deleting asset: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def rename_asset(
        ctx: Context,
        source_path: str,
        dest_path: str
    ) -> Dict[str, Any]:
        """
        Rename/move an asset to a new path.

        Args:
            source_path: Current path of the asset
            dest_path: New path for the asset
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("rename_asset", {
                "source_path": source_path,
                "dest_path": dest_path
            })
            return response or {}
        except Exception as e:
            logger.error(f"Error renaming asset: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def create_folder(ctx: Context, path: str) -> Dict[str, Any]:
        """
        Create a new folder in the content browser.

        Args:
            path: Content path for the new folder (e.g. "/Game/MyFolder")
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("create_folder", {"path": path})
            return response or {}
        except Exception as e:
            logger.error(f"Error creating folder: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def import_asset(
        ctx: Context,
        source_file: str,
        dest_path: str = "/Game",
        dest_name: str = ""
    ) -> Dict[str, Any]:
        """
        Import an external file as an asset.

        Args:
            source_file: Absolute path to the file to import (e.g. "C:/Textures/wood.png")
            dest_path: Content browser destination path
            dest_name: Optional name for the imported asset
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            params = {"source_file": source_file, "dest_path": dest_path}
            if dest_name:
                params["dest_name"] = dest_name
            response = unreal.send_command("import_asset", params)
            return response or {}
        except Exception as e:
            logger.error(f"Error importing asset: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def save_asset(ctx: Context, path: str) -> Dict[str, Any]:
        """
        Save a specific asset to disk.

        Args:
            path: Content path of the asset to save
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("save_asset", {"path": path})
            return response or {}
        except Exception as e:
            logger.error(f"Error saving asset: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def open_asset(ctx: Context, path: str) -> Dict[str, Any]:
        """
        Open an asset in the Unreal Editor (e.g. open a Blueprint in the Blueprint Editor).

        Args:
            path: Content path of the asset to open (e.g. "/Game/Blueprints/BP_Player")
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("open_asset", {"path": path})
            return response or {}
        except Exception as e:
            logger.error(f"Error opening asset: {e}")
            return {"success": False, "message": str(e)}

    logger.info("Asset tools registered successfully")
