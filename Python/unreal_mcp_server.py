"""
Unreal Engine MCP Server

A simple MCP server for interacting with Unreal Engine.
"""

import logging
import socket
import sys
import json
from contextlib import asynccontextmanager
from typing import AsyncIterator, Dict, Any, Optional
from mcp.server.fastmcp import FastMCP

# Configure logging with more detailed format
logging.basicConfig(
    level=logging.DEBUG,  # Change to DEBUG level for more details
    format='%(asctime)s - %(name)s - %(levelname)s - [%(filename)s:%(lineno)d] - %(message)s',
    handlers=[
        logging.FileHandler('unreal_mcp.log'),
        # logging.StreamHandler(sys.stdout) # Remove this handler to unexpected non-whitespace characters in JSON
    ]
)
logger = logging.getLogger("UnrealMCP")

# Configuration
UNREAL_HOST = "127.0.0.1"
UNREAL_PORT = 55557

class UnrealConnection:
    """Connection to an Unreal Engine instance."""
    
    def __init__(self):
        """Initialize the connection."""
        self.socket = None
        self.connected = False
    
    def connect(self) -> bool:
        """Connect to the Unreal Engine instance."""
        try:
            # Close any existing socket
            if self.socket:
                try:
                    self.socket.close()
                except:
                    pass
                self.socket = None
            
            logger.info(f"Connecting to Unreal at {UNREAL_HOST}:{UNREAL_PORT}...")
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(30)  # 30 second connect timeout

            # Set socket options for better stability
            self.socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
            
            # Set larger buffer sizes
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 65536)
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 65536)
            
            self.socket.connect((UNREAL_HOST, UNREAL_PORT))
            self.connected = True
            logger.info("Connected to Unreal Engine")
            return True
            
        except Exception as e:
            logger.error(f"Failed to connect to Unreal: {e}")
            self.connected = False
            return False
    
    def disconnect(self):
        """Disconnect from the Unreal Engine instance."""
        if self.socket:
            try:
                self.socket.close()
            except:
                pass
        self.socket = None
        self.connected = False

    def receive_full_response(self, sock, buffer_size=4096) -> bytes:
        """Receive a complete response from Unreal, handling chunked data."""
        chunks = []
        sock.settimeout(30)  # 30 second receive timeout (LoadObject can be slow)
        try:
            while True:
                chunk = sock.recv(buffer_size)
                if not chunk:
                    if not chunks:
                        raise Exception("Connection closed before receiving data")
                    break
                chunks.append(chunk)
                
                # Process the data received so far
                data = b''.join(chunks)
                decoded_data = data.decode('utf-8')
                
                # Try to parse as JSON to check if complete
                try:
                    json.loads(decoded_data)
                    logger.info(f"Received complete response ({len(data)} bytes)")
                    return data
                except json.JSONDecodeError:
                    # Not complete JSON yet, continue reading
                    logger.debug(f"Received partial response, waiting for more data...")
                    continue
                except Exception as e:
                    logger.warning(f"Error processing response chunk: {str(e)}")
                    continue
        except socket.timeout:
            logger.warning("Socket timeout during receive")
            if chunks:
                # If we have some data already, try to use it
                data = b''.join(chunks)
                try:
                    json.loads(data.decode('utf-8'))
                    logger.info(f"Using partial response after timeout ({len(data)} bytes)")
                    return data
                except:
                    pass
            raise Exception("Timeout receiving Unreal response")
        except Exception as e:
            logger.error(f"Error during receive: {str(e)}")
            raise
    
    def send_command(self, command: str, params: Dict[str, Any] = None) -> Optional[Dict[str, Any]]:
        """Send a command to Unreal Engine and get the response."""
        # Always reconnect for each command, since Unreal closes the connection after each command
        # This is different from Unity which keeps connections alive
        if self.socket:
            try:
                self.socket.close()
            except:
                pass
            self.socket = None
            self.connected = False
        
        if not self.connect():
            logger.error("Failed to connect to Unreal Engine for command")
            return None
        
        try:
            # Match Unity's command format exactly
            command_obj = {
                "type": command,  # Use "type" instead of "command"
                "params": params or {}  # Use Unity's params or {} pattern
            }
            
            # Send without newline, exactly like Unity
            command_json = json.dumps(command_obj)
            logger.info(f"Sending command: {command_json}")
            self.socket.sendall(command_json.encode('utf-8'))
            
            # Read response using improved handler
            response_data = self.receive_full_response(self.socket)
            response = json.loads(response_data.decode('utf-8'))
            
            # Log complete response for debugging
            logger.info(f"Complete response from Unreal: {response}")
            
            # Check for both error formats: {"status": "error", ...} and {"success": false, ...}
            if response.get("status") == "error":
                error_message = response.get("error") or response.get("message", "Unknown Unreal error")
                logger.error(f"Unreal error (status=error): {error_message}")
                # We want to preserve the original error structure but ensure error is accessible
                if "error" not in response:
                    response["error"] = error_message
            elif response.get("success") is False:
                # This format uses {"success": false, "error": "message"} or {"success": false, "message": "message"}
                error_message = response.get("error") or response.get("message", "Unknown Unreal error")
                logger.error(f"Unreal error (success=false): {error_message}")
                # Convert to the standard format expected by higher layers
                response = {
                    "status": "error",
                    "error": error_message
                }
            
            # Always close the connection after command is complete
            # since Unreal will close it on its side anyway
            try:
                self.socket.close()
            except:
                pass
            self.socket = None
            self.connected = False
            
            return response
            
        except Exception as e:
            logger.error(f"Error sending command: {e}")
            # Always reset connection state on any error
            self.connected = False
            try:
                self.socket.close()
            except:
                pass
            self.socket = None
            return {
                "status": "error",
                "error": str(e)
            }

# Global connection state
_unreal_connection: UnrealConnection = None

def get_unreal_connection() -> Optional[UnrealConnection]:
    """Get the connection to Unreal Engine."""
    global _unreal_connection
    try:
        if _unreal_connection is None:
            _unreal_connection = UnrealConnection()
            if not _unreal_connection.connect():
                logger.warning("Could not connect to Unreal Engine")
                _unreal_connection = None
        else:
            # Verify connection is still valid with a ping-like test
            try:
                # Simple test by sending an empty buffer to check if socket is still connected
                _unreal_connection.socket.sendall(b'\x00')
                logger.debug("Connection verified with ping test")
            except Exception as e:
                logger.warning(f"Existing connection failed: {e}")
                _unreal_connection.disconnect()
                _unreal_connection = None
                # Try to reconnect
                _unreal_connection = UnrealConnection()
                if not _unreal_connection.connect():
                    logger.warning("Could not reconnect to Unreal Engine")
                    _unreal_connection = None
                else:
                    logger.info("Successfully reconnected to Unreal Engine")
        
        return _unreal_connection
    except Exception as e:
        logger.error(f"Error getting Unreal connection: {e}")
        return None

@asynccontextmanager
async def server_lifespan(server: FastMCP) -> AsyncIterator[Dict[str, Any]]:
    """Handle server startup and shutdown."""
    global _unreal_connection
    logger.info("UnrealMCP server starting up")
    try:
        _unreal_connection = get_unreal_connection()
        if _unreal_connection:
            logger.info("Connected to Unreal Engine on startup")
        else:
            logger.warning("Could not connect to Unreal Engine on startup")
    except Exception as e:
        logger.error(f"Error connecting to Unreal Engine on startup: {e}")
        _unreal_connection = None
    
    try:
        yield {}
    finally:
        if _unreal_connection:
            _unreal_connection.disconnect()
            _unreal_connection = None
        logger.info("Unreal MCP server shut down")

# Initialize server
mcp = FastMCP(
    "UnrealMCP",
    description="Unreal Engine integration via Model Context Protocol",
    lifespan=server_lifespan
)

# Import and register tools
from tools.editor_tools import register_editor_tools
from tools.blueprint_tools import register_blueprint_tools
from tools.node_tools import register_blueprint_node_tools
from tools.project_tools import register_project_tools
from tools.umg_tools import register_umg_tools
from tools.level_tools import register_level_tools
from tools.material_tools import register_material_tools
from tools.asset_tools import register_asset_tools
from tools.anim_tools import register_anim_tools
from tools.process_tools import register_process_tools

# Register tools
register_editor_tools(mcp)
register_blueprint_tools(mcp)
register_blueprint_node_tools(mcp)
register_project_tools(mcp)
register_umg_tools(mcp)
register_level_tools(mcp)
register_material_tools(mcp)
register_asset_tools(mcp)
register_anim_tools(mcp)
register_process_tools(mcp)

@mcp.prompt()
def info():
    """Information about available Unreal MCP tools and best practices."""
    return """
    # Unreal MCP Server Tools and Best Practices

    ## Level & World Management
    - `new_level(asset_path, template)` - Create new level (optionally from template)
    - `load_level(level_path)` - Load an existing level
    - `save_level()` - Save current level
    - `save_all_levels()` - Save all dirty levels
    - `get_current_level()` - Get current level name and path
    - `play_in_editor()` - Start Play-In-Editor (PIE)
    - `stop_play_in_editor()` - Stop PIE session
    - `is_playing()` - Check if PIE is active
    - `execute_console_command(command)` - Run a console command
    - `build_lighting(quality, with_reflection_captures)` - Build level lighting
    - `set_world_settings(game_mode, kill_z)` - Configure world settings

    ## Editor Tools
    ### Actor Management
    - `get_actors_in_level()` - List all actors
    - `find_actors_by_name(pattern)` - Find actors by name
    - `spawn_actor(name, type, location, rotation)` - Create actors
    - `delete_actor(name)` - Remove actors
    - `set_actor_transform(name, location, rotation, scale)` - Set transform
    - `get_actor_properties(name)` - Get properties
    - `set_actor_property(name, property_name, property_value)` - Set property
    - `spawn_blueprint_actor(blueprint_name, actor_name)` - Spawn BP actor
    ### Selection & Viewport
    - `select_actors(names)` - Select actors by name list
    - `get_selected_actors()` - Get current selection
    - `duplicate_actor(name, new_name, location)` - Duplicate an actor
    - `set_viewport_camera(location, rotation)` - Set viewport camera
    - `get_viewport_camera()` - Get viewport camera position
    - `set_actor_mobility(name, mobility)` - Set Static/Stationary/Movable
    - `set_actor_material(name, material_path, slot)` - Apply material to actor

    ## Material System
    - `create_material(name, path)` - Create new material
    - `create_material_instance(name, parent_material, path)` - Create material instance
    - `set_material_scalar_param(material_name, param_name, value)` - Set scalar param
    - `set_material_vector_param(material_name, param_name, value)` - Set color param
    - `set_material_texture_param(material_name, param_name, texture_path)` - Set texture param
    - `add_material_expression(material_name, expression_type, ...)` - Add graph node
    - `connect_material_expressions(material_name, from_index, to_index, ...)` - Connect nodes
    - `connect_material_property(material_name, from_index, property)` - Connect to output
    - `apply_material_to_actor(actor_name, material_path, slot_index)` - Apply to mesh
    - `recompile_material(material_name)` - Recompile after changes

    ## Asset Management
    - `list_assets(path, class_filter, recursive)` - List assets in folder
    - `find_asset(name)` - Search assets by name
    - `does_asset_exist(path)` - Check if asset exists
    - `duplicate_asset(source_path, dest_path)` - Duplicate asset
    - `delete_asset(path)` - Delete asset
    - `rename_asset(source_path, dest_path)` - Rename/move asset
    - `create_folder(path)` - Create content folder
    - `import_asset(source_file, dest_path, dest_name)` - Import external file
    - `save_asset(path)` - Save specific asset

    ## Blueprint Management
    - `create_blueprint(name, parent_class)` - Create Blueprint class
    - `add_component_to_blueprint(blueprint_name, component_type, component_name)` - Add components
    - `set_static_mesh_properties(blueprint_name, component_name, static_mesh)` - Set mesh
    - `set_physics_properties(blueprint_name, component_name)` - Set physics
    - `compile_blueprint(blueprint_name)` - Compile changes
    - `set_blueprint_property(blueprint_name, property_name, property_value)` - Set property

    ## Blueprint Node Management
    ### Basic Nodes
    - `add_blueprint_event_node(blueprint_name, event_name)` - Add event (ReceiveBeginPlay, ReceiveTick)
    - `add_blueprint_input_action_node(blueprint_name, action_name)` - Input action event
    - `add_blueprint_function_node(blueprint_name, target, function_name)` - Function call
    - `connect_blueprint_nodes(blueprint_name, source_node_id, source_pin, target_node_id, target_pin)` - Wire nodes
    - `add_blueprint_variable(blueprint_name, variable_name, variable_type)` - Add variable
    - `add_blueprint_get_self_component_reference(blueprint_name, component_name)` - Get component ref
    - `add_blueprint_self_reference(blueprint_name)` - Get self ref
    - `find_blueprint_nodes(blueprint_name, node_type, event_type)` - Find nodes
    ### Advanced Nodes
    - `add_blueprint_branch_node(blueprint_name)` - If/Else branch
    - `add_blueprint_for_loop_node(blueprint_name, first_index, last_index)` - For loop
    - `add_blueprint_delay_node(blueprint_name, duration)` - Delay timer
    - `add_blueprint_print_string_node(blueprint_name, text)` - Debug print
    - `add_blueprint_set_timer_node(blueprint_name, function_name, time, looping)` - Timer
    - `add_blueprint_custom_event_node(blueprint_name, event_name)` - Custom event
    - `add_blueprint_variable_get_node(blueprint_name, variable_name)` - Get variable
    - `add_blueprint_variable_set_node(blueprint_name, variable_name)` - Set variable
    - `set_node_pin_default_value(blueprint_name, node_id, pin_name, value)` - Set pin default
    - `add_blueprint_math_node(blueprint_name, operation)` - Math operations (+,-,*,/,>,<,==,!=)

    ## UMG Widgets
    - `create_umg_widget_blueprint(widget_name)` - Create widget BP
    - `add_text_block_to_widget(widget_name, text_block_name)` - Add text
    - `add_button_to_widget(widget_name, button_name)` - Add button
    - `bind_widget_event(widget_name, widget_component_name, event_name)` - Bind event
    - `add_widget_to_viewport(widget_name)` - Show widget
    - `set_text_block_binding(widget_name, text_block_name, binding_property)` - Bind text

    ## Project & Input Settings
    - `create_input_mapping(action_name, key, input_type)` - Legacy input mapping
    - `set_default_game_mode(game_mode_class)` - Set game mode
    - `set_default_map(map_path, map_type)` - Set default map
    - `create_enhanced_input_action(name, value_type)` - Create Enhanced Input action
    - `create_input_mapping_context(name, mappings)` - Create input mapping context
    - `set_project_setting(section, key, value)` - Set config value
    - `get_project_setting(section, key)` - Get config value
    """

# Run the server
if __name__ == "__main__":
    logger.info("Starting MCP server with stdio transport")
    mcp.run(transport='stdio') 