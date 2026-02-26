"""
Animation Blueprint Tools for Unreal MCP.

This module provides tools for creating and configuring Animation Blueprints
including state machines, states, transitions, and animation assignments.
"""

import logging
from typing import Dict, List, Any, Optional
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("UnrealMCP")


def register_anim_tools(mcp: FastMCP):
    """Register animation blueprint tools with the MCP server."""

    @mcp.tool()
    def create_anim_blueprint(
        ctx: Context,
        name: str,
        skeleton: str,
        path: str = "/Game/"
    ) -> Dict[str, Any]:
        """
        Create a new Animation Blueprint for a skeleton.

        Args:
            name: Name for the AnimBP (e.g. 'ABP_Character')
            skeleton: Path to the USkeleton asset (e.g. '/Game/Characters/Mesh/SK_Mannequin_Skeleton')
            path: Package path to create the AnimBP in (default '/Game/')

        Returns:
            Dict with name, path, skeleton
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("create_anim_blueprint", {
                "name": name,
                "skeleton": skeleton,
                "path": path
            })
            if response and "status" in response and response["status"] == "success":
                return {"success": True, "data": response.get("result", response)}
            return {"success": False, "message": response.get("error", "Unknown error") if response else "No response"}
        except Exception as e:
            logger.error(f"create_anim_blueprint error: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_anim_state_machine(
        ctx: Context,
        anim_blueprint: str,
        machine_name: str = "Locomotion"
    ) -> Dict[str, Any]:
        """
        Add a state machine to an Animation Blueprint and connect it to the output.

        Args:
            anim_blueprint: Name or path of the AnimBP
            machine_name: Name for the state machine (default 'Locomotion')

        Returns:
            Dict with machine_name, node_id, connected_to_root
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("add_anim_state_machine", {
                "anim_blueprint": anim_blueprint,
                "machine_name": machine_name
            })
            if response and "status" in response and response["status"] == "success":
                return {"success": True, "data": response.get("result", response)}
            return {"success": False, "message": response.get("error", "Unknown error") if response else "No response"}
        except Exception as e:
            logger.error(f"add_anim_state_machine error: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_anim_state(
        ctx: Context,
        anim_blueprint: str,
        state_name: str,
        machine_name: str = "Locomotion",
        is_default: bool = False,
        position: Optional[List[float]] = None
    ) -> Dict[str, Any]:
        """
        Add a state to a state machine in an Animation Blueprint.

        Args:
            anim_blueprint: Name or path of the AnimBP
            state_name: Name for the state (e.g. 'Idle', 'Walk', 'Run')
            machine_name: Which state machine to add to (default 'Locomotion')
            is_default: If True, connects this state as the entry/default state
            position: Optional [x, y] position in the graph

        Returns:
            Dict with state_name, node_id, is_default
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            params = {
                "anim_blueprint": anim_blueprint,
                "state_name": state_name,
                "machine_name": machine_name,
                "is_default": is_default
            }
            if position:
                params["position"] = position
            response = unreal.send_command("add_anim_state", params)
            if response and "status" in response and response["status"] == "success":
                return {"success": True, "data": response.get("result", response)}
            return {"success": False, "message": response.get("error", "Unknown error") if response else "No response"}
        except Exception as e:
            logger.error(f"add_anim_state error: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_anim_state_animation(
        ctx: Context,
        anim_blueprint: str,
        state_name: str,
        animation: str,
        machine_name: str = "Locomotion",
        looping: bool = True
    ) -> Dict[str, Any]:
        """
        Set the animation played by a state in a state machine.

        Args:
            anim_blueprint: Name or path of the AnimBP
            state_name: Name of the state to set animation for
            animation: Path to the AnimSequence asset
            machine_name: Which state machine the state is in (default 'Locomotion')
            looping: Whether the animation should loop (default True)

        Returns:
            Dict with state_name, animation path, connected_to_result
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("set_anim_state_animation", {
                "anim_blueprint": anim_blueprint,
                "state_name": state_name,
                "animation": animation,
                "machine_name": machine_name,
                "looping": looping
            })
            if response and "status" in response and response["status"] == "success":
                return {"success": True, "data": response.get("result", response)}
            return {"success": False, "message": response.get("error", "Unknown error") if response else "No response"}
        except Exception as e:
            logger.error(f"set_anim_state_animation error: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_anim_transition(
        ctx: Context,
        anim_blueprint: str,
        from_state: str,
        to_state: str,
        machine_name: str = "Locomotion",
        duration: float = 0.2,
        automatic: bool = False
    ) -> Dict[str, Any]:
        """
        Add a transition between two states in a state machine.

        Args:
            anim_blueprint: Name or path of the AnimBP
            from_state: Name of the source state
            to_state: Name of the destination state
            machine_name: Which state machine (default 'Locomotion')
            duration: Crossfade duration in seconds (default 0.2)
            automatic: If True, transition triggers automatically when source animation finishes

        Returns:
            Dict with from_state, to_state, node_id, crossfade_duration
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("add_anim_transition", {
                "anim_blueprint": anim_blueprint,
                "from_state": from_state,
                "to_state": to_state,
                "machine_name": machine_name,
                "duration": duration,
                "automatic": automatic
            })
            if response and "status" in response and response["status"] == "success":
                return {"success": True, "data": response.get("result", response)}
            return {"success": False, "message": response.get("error", "Unknown error") if response else "No response"}
        except Exception as e:
            logger.error(f"add_anim_transition error: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def get_anim_blueprint_info(
        ctx: Context,
        anim_blueprint: str
    ) -> Dict[str, Any]:
        """
        Get detailed information about an Animation Blueprint including
        state machines, states, transitions, and animations.

        Args:
            anim_blueprint: Name or path of the AnimBP

        Returns:
            Dict with name, path, skeleton, state_machines (with states and transitions)
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("get_anim_blueprint_info", {
                "anim_blueprint": anim_blueprint
            })
            if response and "status" in response and response["status"] == "success":
                return {"success": True, "data": response.get("result", response)}
            return {"success": False, "message": response.get("error", "Unknown error") if response else "No response"}
        except Exception as e:
            logger.error(f"get_anim_blueprint_info error: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_anim_transition_rule(
        ctx: Context,
        anim_blueprint: str,
        from_state: str,
        to_state: str,
        variable_name: str,
        operator: str = "",
        value: float = 0.0,
        negate: bool = False,
        machine_name: str = "Locomotion"
    ) -> Dict[str, Any]:
        """
        Set a condition rule on an animation state transition.

        For bool variables: just provide variable_name (and optionally negate=True for NOT).
        For float comparisons: provide variable_name, operator, and value.

        Examples:
            - Bool check: variable_name="IsFalling" (transitions when IsFalling is true)
            - Negated bool: variable_name="IsFalling", negate=True (transitions when NOT falling)
            - Float compare: variable_name="Speed", operator=">", value=0 (when Speed > 0)
            - Float compare: variable_name="Speed", operator="<=", value=10

        Args:
            anim_blueprint: Name or path of the AnimBP
            from_state: Source state name
            to_state: Destination state name
            variable_name: AnimBP variable to check (must already exist on the AnimBP)
            operator: Comparison operator for float vars: >, <, >=, <=, ==, != (empty for bool)
            value: Comparison value for float vars (default 0.0)
            negate: If True, negate a bool variable check (NOT variable)
            machine_name: Which state machine (default 'Locomotion')

        Returns:
            Dict with from_state, to_state, rule description
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            params = {
                "anim_blueprint": anim_blueprint,
                "from_state": from_state,
                "to_state": to_state,
                "variable_name": variable_name,
                "machine_name": machine_name
            }
            if operator:
                params["operator"] = operator
                params["value"] = value
            if negate:
                params["negate"] = negate
            response = unreal.send_command("set_anim_transition_rule", params)
            if response and "status" in response and response["status"] == "success":
                return {"success": True, "data": response.get("result", response)}
            return {"success": False, "message": response.get("error", "Unknown error") if response else "No response"}
        except Exception as e:
            logger.error(f"set_anim_transition_rule error: {e}")
            return {"success": False, "message": str(e)}
