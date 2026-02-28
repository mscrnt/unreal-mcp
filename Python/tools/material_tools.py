"""
Material Tools for Unreal MCP.

This module provides tools for creating and editing materials, material instances,
material expressions, and applying materials to actors.
"""

import logging
from typing import Dict, List, Any, Optional
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("UnrealMCP")


def register_material_tools(mcp: FastMCP):
    """Register material tools with the MCP server."""

    @mcp.tool()
    def create_material(
        ctx: Context,
        name: str,
        path: str = "/Game/Materials"
    ) -> Dict[str, Any]:
        """
        Create a new Material asset.

        Args:
            name: Name of the material
            path: Content browser path where the material should be created
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("create_material", {"name": name, "path": path})
            return response or {}
        except Exception as e:
            logger.error(f"Error creating material: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def create_material_instance(
        ctx: Context,
        name: str,
        parent_material: str,
        path: str = "/Game/Materials"
    ) -> Dict[str, Any]:
        """
        Create a Material Instance from an existing material.

        Args:
            name: Name of the material instance
            parent_material: Path to the parent material
            path: Content browser path where the instance should be created
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("create_material_instance", {
                "name": name,
                "parent_material": parent_material,
                "path": path
            })
            return response or {}
        except Exception as e:
            logger.error(f"Error creating material instance: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_material_scalar_param(
        ctx: Context,
        material_name: str,
        param_name: str,
        value: float
    ) -> Dict[str, Any]:
        """
        Set a scalar parameter value on a material instance.

        Args:
            material_name: Path to the material instance
            param_name: Name of the scalar parameter
            value: Float value to set
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("set_material_scalar_param", {
                "material_name": material_name,
                "param_name": param_name,
                "value": value
            })
            return response or {}
        except Exception as e:
            logger.error(f"Error setting scalar param: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_material_vector_param(
        ctx: Context,
        material_name: str,
        param_name: str,
        value: List[float]
    ) -> Dict[str, Any]:
        """
        Set a vector/color parameter value on a material instance.

        Args:
            material_name: Path to the material instance
            param_name: Name of the vector parameter
            value: [R, G, B] or [R, G, B, A] color values (0.0 to 1.0)
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("set_material_vector_param", {
                "material_name": material_name,
                "param_name": param_name,
                "value": value
            })
            return response or {}
        except Exception as e:
            logger.error(f"Error setting vector param: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_material_texture_param(
        ctx: Context,
        material_name: str,
        param_name: str,
        texture_path: str
    ) -> Dict[str, Any]:
        """
        Set a texture parameter value on a material instance.

        Args:
            material_name: Path to the material instance
            param_name: Name of the texture parameter
            texture_path: Path to the texture asset
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("set_material_texture_param", {
                "material_name": material_name,
                "param_name": param_name,
                "texture_path": texture_path
            })
            return response or {}
        except Exception as e:
            logger.error(f"Error setting texture param: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_material_expression(
        ctx: Context,
        material_name: str,
        expression_type: str,
        position: List[float] = None,
        param_name: str = None,
        value: float = None,
        color: List[float] = None,
        texture_path: str = None
    ) -> Dict[str, Any]:
        """
        Add a material expression node to a material's graph.

        Args:
            material_name: Path to the material
            expression_type: Type of expression (Constant, Constant3Vector, Constant4Vector,
                TextureSample, ScalarParameter, VectorParameter, Multiply, Add, Lerp,
                Subtract, Divide, Power, Abs, Min, Max, Clamp, Saturate, OneMinus,
                Dot, CrossProduct, SmoothStep, Step, Append, ComponentMask, Fresnel,
                Normalize, VertexNormalWS, PixelNormalWS, CameraPositionWS, WorldPosition,
                TextureCoordinate, Time, Panner, VertexColor, If, StaticSwitch, StaticBool,
                StaticBoolParameter, TextureObjectParameter, CurveAtlasRowParameter).
                Any UMaterialExpression subclass name also works (without the UMaterialExpression prefix).
            position: Optional [X, Y] position in the graph
            param_name: Optional parameter name (for ScalarParameter, VectorParameter)
            value: Optional default value (for Constant, ScalarParameter)
            color: Optional [R,G,B] or [R,G,B,A] color (for vector constants/parameters)
            texture_path: Optional content path to a texture asset (for TextureSample, TextureObjectParameter)

        Returns:
            Dict with expression_name and expression_index
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            params = {
                "material_name": material_name,
                "expression_type": expression_type
            }
            if position is not None:
                params["position"] = position
            if param_name is not None:
                params["param_name"] = param_name
            if value is not None:
                params["value"] = value
            if color is not None:
                params["color"] = color
            if texture_path is not None:
                params["texture_path"] = texture_path
            response = unreal.send_command("add_material_expression", params)
            return response or {}
        except Exception as e:
            logger.error(f"Error adding material expression: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def connect_material_expressions(
        ctx: Context,
        material_name: str,
        from_expression_index: int,
        to_expression_index: int,
        from_output: str = "",
        to_input: str = ""
    ) -> Dict[str, Any]:
        """
        Connect two material expression nodes together.

        Args:
            material_name: Path to the material
            from_expression_index: Index of the source expression
            to_expression_index: Index of the target expression
            from_output: Name of the output pin on the source expression
            to_input: Name of the input pin on the target expression
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("connect_material_expressions", {
                "material_name": material_name,
                "from_expression_index": from_expression_index,
                "to_expression_index": to_expression_index,
                "from_output": from_output,
                "to_input": to_input
            })
            return response or {}
        except Exception as e:
            logger.error(f"Error connecting material expressions: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def connect_material_property(
        ctx: Context,
        material_name: str,
        from_expression_index: int,
        property: str,
        from_output: str = ""
    ) -> Dict[str, Any]:
        """
        Connect a material expression to a material property (like BaseColor, Roughness, etc.).

        Args:
            material_name: Path to the material
            from_expression_index: Index of the source expression
            property: Material property name (BaseColor, Metallic, Specular, Roughness,
                EmissiveColor, Opacity, OpacityMask, Normal, AmbientOcclusion)
            from_output: Name of the output pin on the source expression
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("connect_material_property", {
                "material_name": material_name,
                "from_expression_index": from_expression_index,
                "property": property,
                "from_output": from_output
            })
            return response or {}
        except Exception as e:
            logger.error(f"Error connecting to material property: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def apply_material_to_actor(
        ctx: Context,
        actor_name: str,
        material_path: str,
        slot_index: int = 0
    ) -> Dict[str, Any]:
        """
        Apply a material to an actor's static mesh component.

        Args:
            actor_name: Name of the target actor
            material_path: Path to the material or material instance
            slot_index: Material slot index (default 0)
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("apply_material_to_actor", {
                "actor_name": actor_name,
                "material_path": material_path,
                "slot_index": slot_index
            })
            return response or {}
        except Exception as e:
            logger.error(f"Error applying material: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def recompile_material(ctx: Context, material_name: str) -> Dict[str, Any]:
        """
        Recompile a material after making changes to its graph.

        Args:
            material_name: Path to the material to recompile
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("recompile_material", {"material_name": material_name})
            return response or {}
        except Exception as e:
            logger.error(f"Error recompiling material: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_material_expression_property(
        ctx: Context,
        material_name: str,
        expression_index: int,
        property_name: str,
        property_value: str
    ) -> Dict[str, Any]:
        """
        Set a property on a material expression node. Use this to configure expression
        nodes after creation — for example, setting the Atlas/Curve on a CurveAtlasRowParameter,
        the Texture on a TextureSample, or R/G/B/A mask flags on a ComponentMask.

        For object reference properties (textures, curve atlases, etc.), pass the content path
        as the value (e.g. "/Game/Textures/MyTexture").

        Args:
            material_name: Path to the material
            expression_index: Index of the expression (from add_material_expression result)
            property_name: Name of the property to set (e.g. "Texture", "Atlas", "Curve", "R", "G", "B", "A")
            property_value: Value to set the property to (string, number, or object path)

        Returns:
            Dict containing response from Unreal with operation status
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("set_material_expression_property", {
                "material_name": material_name,
                "expression_index": expression_index,
                "property_name": property_name,
                "property_value": property_value
            })
            return response or {}
        except Exception as e:
            logger.error(f"Error setting material expression property: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_material_expressions(
        ctx: Context,
        material_name: str
    ) -> Dict[str, Any]:
        """
        Get all expression nodes in a material's graph. Returns each expression's
        index, name, class, and caption. Useful for inspecting a material before
        connecting or modifying expressions.

        Args:
            material_name: Path to the material

        Returns:
            Dict with expression_count and expressions array
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("get_material_expressions", {
                "material_name": material_name
            })
            return response or {}
        except Exception as e:
            logger.error(f"Error getting material expressions: {e}")
            return {"success": False, "message": str(e)}

    logger.info("Material tools registered successfully")
