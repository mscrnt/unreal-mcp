"""
Editor Tools for Unreal MCP.

This module provides tools for controlling the Unreal Editor viewport and other editor functionality.
"""

import logging
from typing import Dict, List, Any, Optional
from mcp.server.fastmcp import FastMCP, Context

# Get logger
logger = logging.getLogger("UnrealMCP")

def register_editor_tools(mcp: FastMCP):
    """Register editor tools with the MCP server."""
    
    @mcp.tool()
    def get_actors_in_level(ctx: Context) -> List[Dict[str, Any]]:
        """Get a list of all actors in the current level."""
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return []
                
            response = unreal.send_command("get_actors_in_level", {})
            
            if not response:
                logger.warning("No response from Unreal Engine")
                return []
                
            # Log the complete response for debugging
            logger.info(f"Complete response from Unreal: {response}")
            
            # Check response format
            if "result" in response and "actors" in response["result"]:
                actors = response["result"]["actors"]
                logger.info(f"Found {len(actors)} actors in level")
                return actors
            elif "actors" in response:
                actors = response["actors"]
                logger.info(f"Found {len(actors)} actors in level")
                return actors
                
            logger.warning(f"Unexpected response format: {response}")
            return []
            
        except Exception as e:
            logger.error(f"Error getting actors: {e}")
            return []

    @mcp.tool()
    def find_actors_by_name(ctx: Context, pattern: str) -> List[str]:
        """Find actors by name pattern."""
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.warning("Failed to connect to Unreal Engine")
                return []
                
            response = unreal.send_command("find_actors_by_name", {
                "pattern": pattern
            })
            
            if not response:
                return []

            # Response may be wrapped in "result"
            data = response.get("result", response)
            return data.get("actors", [])

        except Exception as e:
            logger.error(f"Error finding actors: {e}")
            return []
    
    @mcp.tool()
    def spawn_actor(
        ctx: Context,
        name: str,
        type: str,
        location: List[float] = [0.0, 0.0, 0.0],
        rotation: List[float] = [0.0, 0.0, 0.0]
    ) -> Dict[str, Any]:
        """Create a new actor in the current level.
        
        Args:
            ctx: The MCP context
            name: The name to give the new actor (must be unique)
            type: The type of actor to create (e.g. StaticMeshActor, PointLight)
            location: The [x, y, z] world location to spawn at
            rotation: The [pitch, yaw, roll] rotation in degrees
            
        Returns:
            Dict containing the created actor's properties
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            # Ensure all parameters are properly formatted
            params = {
                "name": name,
                "type": type.upper(),  # Make sure type is uppercase
                "location": location,
                "rotation": rotation
            }
            
            # Validate location and rotation formats
            for param_name in ["location", "rotation"]:
                param_value = params[param_name]
                if not isinstance(param_value, list) or len(param_value) != 3:
                    logger.error(f"Invalid {param_name} format: {param_value}. Must be a list of 3 float values.")
                    return {"success": False, "message": f"Invalid {param_name} format. Must be a list of 3 float values."}
                # Ensure all values are float
                params[param_name] = [float(val) for val in param_value]
            
            logger.info(f"Creating actor '{name}' of type '{type}' with params: {params}")
            response = unreal.send_command("spawn_actor", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            # Log the complete response for debugging
            logger.info(f"Actor creation response: {response}")
            
            # Handle error responses correctly
            if response.get("status") == "error":
                error_message = response.get("error", "Unknown error")
                logger.error(f"Error creating actor: {error_message}")
                return {"success": False, "message": error_message}
            
            return response
            
        except Exception as e:
            error_msg = f"Error creating actor: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def delete_actor(ctx: Context, name: str) -> Dict[str, Any]:
        """Delete an actor by name."""
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
                
            response = unreal.send_command("delete_actor", {
                "name": name
            })
            return response or {}
            
        except Exception as e:
            logger.error(f"Error deleting actor: {e}")
            return {}
    
    @mcp.tool()
    def set_actor_transform(
        ctx: Context,
        name: str,
        location: List[float]  = None,
        rotation: List[float]  = None,
        scale: List[float] = None
    ) -> Dict[str, Any]:
        """Set the transform of an actor."""
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
                
            params = {"name": name}
            if location is not None:
                params["location"] = location
            if rotation is not None:
                params["rotation"] = rotation
            if scale is not None:
                params["scale"] = scale
                
            response = unreal.send_command("set_actor_transform", params)
            return response or {}
            
        except Exception as e:
            logger.error(f"Error setting transform: {e}")
            return {}
    
    @mcp.tool()
    def get_actor_properties(ctx: Context, name: str) -> Dict[str, Any]:
        """Get all properties of an actor."""
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
                
            response = unreal.send_command("get_actor_properties", {
                "name": name
            })
            return response or {}
            
        except Exception as e:
            logger.error(f"Error getting properties: {e}")
            return {}

    @mcp.tool()
    def set_actor_property(
        ctx: Context,
        name: str,
        property_name: str,
        property_value,
    ) -> Dict[str, Any]:
        """
        Set a property on an actor.
        
        Args:
            name: Name of the actor
            property_name: Name of the property to set
            property_value: Value to set the property to
            
        Returns:
            Dict containing response from Unreal with operation status
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
                
            response = unreal.send_command("set_actor_property", {
                "name": name,
                "property_name": property_name,
                "property_value": property_value
            })
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Set actor property response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error setting actor property: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    # @mcp.tool() commented out because it's buggy
    def focus_viewport(
        ctx: Context,
        target: str = None,
        location: List[float] = None,
        distance: float = 1000.0,
        orientation: List[float] = None
    ) -> Dict[str, Any]:
        """
        Focus the viewport on a specific actor or location.
        
        Args:
            target: Name of the actor to focus on (if provided, location is ignored)
            location: [X, Y, Z] coordinates to focus on (used if target is None)
            distance: Distance from the target/location
            orientation: Optional [Pitch, Yaw, Roll] for the viewport camera
            
        Returns:
            Response from Unreal Engine
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
                
            params = {}
            if target:
                params["target"] = target
            elif location:
                params["location"] = location
            
            if distance:
                params["distance"] = distance
                
            if orientation:
                params["orientation"] = orientation
                
            response = unreal.send_command("focus_viewport", params)
            return response or {}
            
        except Exception as e:
            logger.error(f"Error focusing viewport: {e}")
            return {"status": "error", "message": str(e)}

    @mcp.tool()
    def spawn_blueprint_actor(
        ctx: Context,
        blueprint_name: str,
        actor_name: str,
        location: List[float] = [0.0, 0.0, 0.0],
        rotation: List[float] = [0.0, 0.0, 0.0]
    ) -> Dict[str, Any]:
        """Spawn an actor from a Blueprint.
        
        Args:
            ctx: The MCP context
            blueprint_name: Name of the Blueprint to spawn from
            actor_name: Name to give the spawned actor
            location: The [x, y, z] world location to spawn at
            rotation: The [pitch, yaw, roll] rotation in degrees
            
        Returns:
            Dict containing the spawned actor's properties
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            # Ensure all parameters are properly formatted
            params = {
                "blueprint_name": blueprint_name,
                "actor_name": actor_name,
                "location": location or [0.0, 0.0, 0.0],
                "rotation": rotation or [0.0, 0.0, 0.0]
            }
            
            # Validate location and rotation formats
            for param_name in ["location", "rotation"]:
                param_value = params[param_name]
                if not isinstance(param_value, list) or len(param_value) != 3:
                    logger.error(f"Invalid {param_name} format: {param_value}. Must be a list of 3 float values.")
                    return {"success": False, "message": f"Invalid {param_name} format. Must be a list of 3 float values."}
                # Ensure all values are float
                params[param_name] = [float(val) for val in param_value]
            
            logger.info(f"Spawning blueprint actor with params: {params}")
            response = unreal.send_command("spawn_blueprint_actor", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Spawn blueprint actor response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error spawning blueprint actor: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    # ============================================================
    # Phase 5: Editor Enhancements
    # ============================================================

    @mcp.tool()
    def select_actors(ctx: Context, names: List[str]) -> Dict[str, Any]:
        """
        Select actors in the editor by name.

        Args:
            names: List of actor names to select
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("select_actors", {"names": names})
            return response or {}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_selected_actors(ctx: Context) -> Dict[str, Any]:
        """Get the currently selected actors in the editor."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("get_selected_actors", {})
            return response or {}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def duplicate_actor(
        ctx: Context,
        name: str,
        new_name: str = "",
        location: List[float] = None
    ) -> Dict[str, Any]:
        """
        Duplicate an actor in the level.

        Args:
            name: Name of the actor to duplicate
            new_name: Optional name for the duplicated actor
            location: Optional [X, Y, Z] location for the duplicate
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            params = {"name": name}
            if new_name:
                params["new_name"] = new_name
            if location is not None:
                params["location"] = location
            response = unreal.send_command("duplicate_actor", params)
            return response or {}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_viewport_camera(
        ctx: Context,
        location: List[float] = None,
        rotation: List[float] = None
    ) -> Dict[str, Any]:
        """
        Set the editor viewport camera position and rotation.

        Args:
            location: [X, Y, Z] world position
            rotation: [Pitch, Yaw, Roll] rotation in degrees
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            params = {}
            if location is not None:
                params["location"] = location
            if rotation is not None:
                params["rotation"] = rotation
            response = unreal.send_command("set_viewport_camera", params)
            return response or {}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_viewport_camera(ctx: Context) -> Dict[str, Any]:
        """Get the current editor viewport camera position and rotation."""
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("get_viewport_camera", {})
            return response or {}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_actor_mobility(
        ctx: Context,
        name: str,
        mobility: str
    ) -> Dict[str, Any]:
        """
        Set the mobility of an actor's root component.

        Args:
            name: Name of the actor
            mobility: Mobility type - "Static", "Stationary", or "Movable"
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("set_actor_mobility", {
                "name": name,
                "mobility": mobility
            })
            return response or {}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_actor_tags(
        ctx: Context,
        name: str,
        tags: List[str],
        mode: str = "set"
    ) -> Dict[str, Any]:
        """
        Set, add, or remove tags on an actor. Tags are FName arrays used for
        identification (e.g. finding RL agents by tag during PIE).

        Args:
            name: Name of the actor
            tags: List of tag strings (e.g. ["RLRed", "Player"])
            mode: "set" (replace all tags), "add" (append), or "remove" (remove specific tags)
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("set_actor_tags", {
                "name": name,
                "tags": tags,
                "mode": mode
            })
            return response or {}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_actor_tags(
        ctx: Context,
        name: str
    ) -> Dict[str, Any]:
        """
        Get all tags on an actor.

        Args:
            name: Name of the actor
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("get_actor_tags", {
                "name": name
            })
            return response or {}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_actor_material(
        ctx: Context,
        name: str,
        material_path: str,
        slot: int = 0
    ) -> Dict[str, Any]:
        """
        Set the material on an actor's mesh component (StaticMesh or SkeletalMesh).

        Args:
            name: Name of the actor
            material_path: Content path to the material
            slot: Material slot index (default 0)
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("set_actor_material", {
                "name": name,
                "material_path": material_path,
                "slot": slot
            })
            return response or {}
        except Exception as e:
            return {"success": False, "message": str(e)}

    # ============================================================
    # PIE / RL Tools
    # ============================================================

    @mcp.tool()
    def add_movement_input(
        ctx: Context,
        actor_name: str,
        direction: List[float] = [0.0, 1.0, 0.0],
        scale: float = 1.0
    ) -> dict:
        """
        Add movement input to a Pawn during PIE. This is a single-frame input —
        call it each step for continuous movement.

        Works on any APawn (Characters, custom pawns). The pawn's movement
        component processes the input on the next tick.

        Args:
            actor_name: Name of the PIE pawn actor (e.g. 'BP_Character_C_0')
            direction: World direction vector [X, Y, Z] (will be normalized by the movement component)
            scale: Movement scale factor (0.0 to 1.0, default 1.0)

        Returns:
            Dict with current location and velocity after input
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("add_movement_input", {
                "actor_name": actor_name,
                "direction": direction,
                "scale": scale
            })
            return response or {}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def pawn_action(
        ctx: Context,
        actor_name: str,
        action: str,
        velocity: Optional[List[float]] = None
    ) -> dict:
        """
        Execute a discrete action on a Pawn/Character during PIE.

        Supported actions:
            - jump: Make the character jump (ACharacter only)
            - stop_jumping: Release jump (ACharacter only)
            - crouch: Start crouching (ACharacter only)
            - uncrouch: Stop crouching (ACharacter only)
            - launch: Launch with a velocity vector (provide 'velocity' param)

        Args:
            actor_name: Name of the PIE pawn actor (e.g. 'BP_Character_C_0')
            action: Action to perform (jump, stop_jumping, crouch, uncrouch, launch)
            velocity: [X, Y, Z] velocity for 'launch' action only

        Returns:
            Dict with current location, velocity, and state flags
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            params = {
                "actor_name": actor_name,
                "action": action
            }
            if velocity is not None:
                params["velocity"] = velocity
            response = unreal.send_command("pawn_action", params)
            return response or {}
        except Exception as e:
            return {"success": False, "message": str(e)}

    logger.info("Editor tools registered successfully")
