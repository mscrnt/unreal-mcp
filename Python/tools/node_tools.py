"""
Blueprint Node Tools for Unreal MCP.

This module provides tools for manipulating Blueprint graph nodes and connections.
"""

import logging
from typing import Dict, List, Any, Optional
from mcp.server.fastmcp import FastMCP, Context

# Get logger
logger = logging.getLogger("UnrealMCP")

def register_blueprint_node_tools(mcp: FastMCP):
    """Register Blueprint node manipulation tools with the MCP server."""
    
    @mcp.tool()
    def add_blueprint_event_node(
        ctx: Context,
        blueprint_name: str,
        event_name: str,
        node_position = None
    ) -> Dict[str, Any]:
        """
        Add an event node to a Blueprint's event graph.
        
        Args:
            blueprint_name: Name of the target Blueprint
            event_name: Name of the event. Use 'Receive' prefix for standard events:
                       - 'ReceiveBeginPlay' for Begin Play
                       - 'ReceiveTick' for Tick
                       - etc.
            node_position: Optional [X, Y] position in the graph
            
        Returns:
            Response containing the node ID and success status
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            # Handle default value within the method body
            if node_position is None:
                node_position = [0, 0]
            
            params = {
                "blueprint_name": blueprint_name,
                "event_name": event_name,
                "node_position": node_position
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding event node '{event_name}' to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_event_node", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Event node creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding event node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def add_blueprint_input_action_node(
        ctx: Context,
        blueprint_name: str,
        action_name: str,
        node_position = None
    ) -> Dict[str, Any]:
        """
        Add an input action event node to a Blueprint's event graph.
        
        Args:
            blueprint_name: Name of the target Blueprint
            action_name: Name of the input action to respond to
            node_position: Optional [X, Y] position in the graph
            
        Returns:
            Response containing the node ID and success status
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            # Handle default value within the method body
            if node_position is None:
                node_position = [0, 0]
            
            params = {
                "blueprint_name": blueprint_name,
                "action_name": action_name,
                "node_position": node_position
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding input action node for '{action_name}' to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_input_action_node", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Input action node creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding input action node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def add_blueprint_function_node(
        ctx: Context,
        blueprint_name: str,
        target: str,
        function_name: str,
        params = None,
        node_position = None
    ) -> Dict[str, Any]:
        """
        Add a function call node to a Blueprint's event graph.
        
        Args:
            blueprint_name: Name of the target Blueprint
            target: Target object for the function (component name or self)
            function_name: Name of the function to call
            params: Optional parameters to set on the function node
            node_position: Optional [X, Y] position in the graph
            
        Returns:
            Response containing the node ID and success status
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            # Handle default values within the method body
            if params is None:
                params = {}
            if node_position is None:
                node_position = [0, 0]
            
            command_params = {
                "blueprint_name": blueprint_name,
                "target": target,
                "function_name": function_name,
                "params": params,
                "node_position": node_position
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding function node '{function_name}' to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_function_node", command_params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Function node creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding function node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
            
    @mcp.tool()
    def connect_blueprint_nodes(
        ctx: Context,
        blueprint_name: str,
        source_node_id: str,
        source_pin: str,
        target_node_id: str,
        target_pin: str
    ) -> Dict[str, Any]:
        """
        Connect two nodes in a Blueprint's event graph.
        
        Args:
            blueprint_name: Name of the target Blueprint
            source_node_id: ID of the source node
            source_pin: Name of the output pin on the source node
            target_node_id: ID of the target node
            target_pin: Name of the input pin on the target node
            
        Returns:
            Response indicating success or failure
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            params = {
                "blueprint_name": blueprint_name,
                "source_node_id": source_node_id,
                "source_pin": source_pin,
                "target_node_id": target_node_id,
                "target_pin": target_pin
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Connecting nodes in blueprint '{blueprint_name}'")
            response = unreal.send_command("connect_blueprint_nodes", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Node connection response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error connecting nodes: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def add_blueprint_variable(
        ctx: Context,
        blueprint_name: str,
        variable_name: str,
        variable_type: str,
        is_exposed: bool = False
    ) -> Dict[str, Any]:
        """
        Add a variable to a Blueprint.
        
        Args:
            blueprint_name: Name of the target Blueprint
            variable_name: Name of the variable
            variable_type: Type of the variable (Boolean, Integer, Float, Vector, etc.)
            is_exposed: Whether to expose the variable to the editor
            
        Returns:
            Response indicating success or failure
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            params = {
                "blueprint_name": blueprint_name,
                "variable_name": variable_name,
                "variable_type": variable_type,
                "is_exposed": is_exposed
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding variable '{variable_name}' to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_variable", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Variable creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding variable: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def add_blueprint_get_self_component_reference(
        ctx: Context,
        blueprint_name: str,
        component_name: str,
        node_position = None
    ) -> Dict[str, Any]:
        """
        Add a node that gets a reference to a component owned by the current Blueprint.
        This creates a node similar to what you get when dragging a component from the Components panel.
        
        Args:
            blueprint_name: Name of the target Blueprint
            component_name: Name of the component to get a reference to
            node_position: Optional [X, Y] position in the graph
            
        Returns:
            Response containing the node ID and success status
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            # Handle None case explicitly in the function
            if node_position is None:
                node_position = [0, 0]
            
            params = {
                "blueprint_name": blueprint_name,
                "component_name": component_name,
                "node_position": node_position
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding self component reference node for '{component_name}' to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_get_self_component_reference", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Self component reference node creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding self component reference node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def add_blueprint_self_reference(
        ctx: Context,
        blueprint_name: str,
        node_position = None
    ) -> Dict[str, Any]:
        """
        Add a 'Get Self' node to a Blueprint's event graph that returns a reference to this actor.
        
        Args:
            blueprint_name: Name of the target Blueprint
            node_position: Optional [X, Y] position in the graph
            
        Returns:
            Response containing the node ID and success status
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            if node_position is None:
                node_position = [0, 0]
                
            params = {
                "blueprint_name": blueprint_name,
                "node_position": node_position
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding self reference node to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_self_reference", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Self reference node creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding self reference node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def find_blueprint_nodes(
        ctx: Context,
        blueprint_name: str,
        node_type = None,
        event_type = None
    ) -> Dict[str, Any]:
        """
        Find nodes in a Blueprint's event graph.
        
        Args:
            blueprint_name: Name of the target Blueprint
            node_type: Optional type of node to find (Event, Function, Variable, etc.)
            event_type: Optional specific event type to find (BeginPlay, Tick, etc.)
            
        Returns:
            Response containing array of found node IDs and success status
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            params = {
                "blueprint_name": blueprint_name,
                "node_type": node_type,
                "event_type": event_type
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Finding nodes in blueprint '{blueprint_name}'")
            response = unreal.send_command("find_blueprint_nodes", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Node find response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error finding nodes: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    # ============================================================
    # Phase 4: Advanced Blueprint Nodes
    # ============================================================

    @mcp.tool()
    def add_blueprint_branch_node(
        ctx: Context,
        blueprint_name: str,
        node_position: List[float] = None
    ) -> Dict[str, Any]:
        """
        Add a Branch (if/else) node to a Blueprint's event graph.

        Args:
            blueprint_name: Name of the target Blueprint
            node_position: Optional [X, Y] position in the graph
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if node_position is None:
                node_position = [0, 0]
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("add_blueprint_branch_node", {
                "blueprint_name": blueprint_name,
                "node_position": node_position
            })
            return response or {}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_blueprint_for_loop_node(
        ctx: Context,
        blueprint_name: str,
        first_index: int = 0,
        last_index: int = 0,
        node_position: List[float] = None
    ) -> Dict[str, Any]:
        """
        Add a ForLoop macro node to a Blueprint's event graph.

        Args:
            blueprint_name: Name of the target Blueprint
            first_index: Starting index for the loop
            last_index: Ending index for the loop
            node_position: Optional [X, Y] position in the graph
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if node_position is None:
                node_position = [0, 0]
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("add_blueprint_for_loop_node", {
                "blueprint_name": blueprint_name,
                "first_index": first_index,
                "last_index": last_index,
                "node_position": node_position
            })
            return response or {}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_blueprint_delay_node(
        ctx: Context,
        blueprint_name: str,
        duration: float = 1.0,
        node_position: List[float] = None
    ) -> Dict[str, Any]:
        """
        Add a Delay node to a Blueprint's event graph.

        Args:
            blueprint_name: Name of the target Blueprint
            duration: Delay duration in seconds
            node_position: Optional [X, Y] position in the graph
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if node_position is None:
                node_position = [0, 0]
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("add_blueprint_delay_node", {
                "blueprint_name": blueprint_name,
                "duration": duration,
                "node_position": node_position
            })
            return response or {}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_blueprint_print_string_node(
        ctx: Context,
        blueprint_name: str,
        text: str = "Hello",
        node_position: List[float] = None
    ) -> Dict[str, Any]:
        """
        Add a PrintString node to a Blueprint's event graph (useful for debugging).

        Args:
            blueprint_name: Name of the target Blueprint
            text: Default text to print
            node_position: Optional [X, Y] position in the graph
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if node_position is None:
                node_position = [0, 0]
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("add_blueprint_print_string_node", {
                "blueprint_name": blueprint_name,
                "text": text,
                "node_position": node_position
            })
            return response or {}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_blueprint_set_timer_node(
        ctx: Context,
        blueprint_name: str,
        function_name: str,
        time: float = 1.0,
        looping: bool = False,
        node_position: List[float] = None
    ) -> Dict[str, Any]:
        """
        Add a SetTimerByFunctionName node to a Blueprint's event graph.

        Args:
            blueprint_name: Name of the target Blueprint
            function_name: Name of the function to call on timer
            time: Timer interval in seconds
            looping: Whether the timer should loop
            node_position: Optional [X, Y] position in the graph
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if node_position is None:
                node_position = [0, 0]
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("add_blueprint_set_timer_node", {
                "blueprint_name": blueprint_name,
                "function_name": function_name,
                "time": time,
                "looping": looping,
                "node_position": node_position
            })
            return response or {}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_blueprint_custom_event_node(
        ctx: Context,
        blueprint_name: str,
        event_name: str,
        node_position: List[float] = None
    ) -> Dict[str, Any]:
        """
        Add a Custom Event node to a Blueprint's event graph.

        Args:
            blueprint_name: Name of the target Blueprint
            event_name: Name for the custom event
            node_position: Optional [X, Y] position in the graph
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if node_position is None:
                node_position = [0, 0]
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("add_blueprint_custom_event_node", {
                "blueprint_name": blueprint_name,
                "event_name": event_name,
                "node_position": node_position
            })
            return response or {}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_blueprint_variable_get_node(
        ctx: Context,
        blueprint_name: str,
        variable_name: str,
        node_position: List[float] = None
    ) -> Dict[str, Any]:
        """
        Add a Variable Get node to a Blueprint's event graph.

        Args:
            blueprint_name: Name of the target Blueprint
            variable_name: Name of the variable to get
            node_position: Optional [X, Y] position in the graph
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if node_position is None:
                node_position = [0, 0]
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("add_blueprint_variable_get_node", {
                "blueprint_name": blueprint_name,
                "variable_name": variable_name,
                "node_position": node_position
            })
            return response or {}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_blueprint_variable_set_node(
        ctx: Context,
        blueprint_name: str,
        variable_name: str,
        node_position: List[float] = None
    ) -> Dict[str, Any]:
        """
        Add a Variable Set node to a Blueprint's event graph.

        Args:
            blueprint_name: Name of the target Blueprint
            variable_name: Name of the variable to set
            node_position: Optional [X, Y] position in the graph
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if node_position is None:
                node_position = [0, 0]
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("add_blueprint_variable_set_node", {
                "blueprint_name": blueprint_name,
                "variable_name": variable_name,
                "node_position": node_position
            })
            return response or {}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def set_node_pin_default_value(
        ctx: Context,
        blueprint_name: str,
        node_id: str,
        pin_name: str,
        value: str
    ) -> Dict[str, Any]:
        """
        Set the default value of a pin on a Blueprint node.

        Args:
            blueprint_name: Name of the target Blueprint
            node_id: GUID of the node
            pin_name: Name of the pin to set
            value: String value to set as default
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("set_node_pin_default_value", {
                "blueprint_name": blueprint_name,
                "node_id": node_id,
                "pin_name": pin_name,
                "value": value
            })
            return response or {}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def add_blueprint_math_node(
        ctx: Context,
        blueprint_name: str,
        operation: str,
        node_position: List[float] = None
    ) -> Dict[str, Any]:
        """
        Add a math operation node to a Blueprint's event graph.

        Args:
            blueprint_name: Name of the target Blueprint
            operation: Math operation. Integer: +, -, *, /, >, <, ==, !=
                       Float: +f, -f, *f, /f, >f, <f
                       Named: Add, Subtract, Multiply, Divide, Greater, Less, Equal, NotEqual,
                       AddFloat, SubtractFloat, MultiplyFloat, DivideFloat, GreaterFloat, LessFloat
            node_position: Optional [X, Y] position in the graph
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            if node_position is None:
                node_position = [0, 0]
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("add_blueprint_math_node", {
                "blueprint_name": blueprint_name,
                "operation": operation,
                "node_position": node_position
            })
            return response or {}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def remove_blueprint_variable(
        ctx: Context,
        blueprint_name: str,
        variable_name: str
    ) -> dict:
        """
        Remove a variable from a Blueprint. Also removes all graph nodes referencing it.

        Args:
            blueprint_name: Name of the target Blueprint
            variable_name: Name of the variable to remove
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("remove_blueprint_variable", {
                "blueprint_name": blueprint_name,
                "variable_name": variable_name
            })
            return response or {}
        except Exception as e:
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def change_blueprint_variable_type(
        ctx: Context,
        blueprint_name: str,
        variable_name: str,
        new_type: str
    ) -> dict:
        """
        Change the type of an existing Blueprint variable.

        WARNING: This will break any existing node connections using the old type.
        Consider removing and recreating nodes that reference the variable after changing its type.

        Args:
            blueprint_name: Name of the target Blueprint
            variable_name: Name of the variable to change
            new_type: New type. Options: Boolean, Integer, Float, Double, String, Vector, Rotator, Name, Text, Byte
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            response = unreal.send_command("change_blueprint_variable_type", {
                "blueprint_name": blueprint_name,
                "variable_name": variable_name,
                "new_type": new_type
            })
            return response or {}
        except Exception as e:
            return {"success": False, "message": str(e)}

    logger.info("Blueprint node tools registered successfully")