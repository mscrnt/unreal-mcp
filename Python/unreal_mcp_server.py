"""
Unreal Engine MCP Server

A simple MCP server for interacting with Unreal Engine.
"""

import logging
import os
import platform
import socket
import sys
import json
import time
from contextlib import asynccontextmanager
from typing import AsyncIterator, Dict, Any, Optional
from fastmcp import FastMCP, Context

# Configure logging with more detailed format
logging.basicConfig(
    level=logging.DEBUG,  # Change to DEBUG level for more details
    format='%(asctime)s - %(name)s - %(levelname)s - [%(filename)s:%(lineno)d] - %(message)s',
    handlers=[
        logging.FileHandler('unreal_mcp.log'),
        # logging.StreamHandler(sys.stdout) # Remove this handler to unexpected non-whitespace characters in JSON
    ]
)
if os.environ.get("MCP_TRANSPORT") == "sse":
    logging.getLogger().addHandler(logging.StreamHandler(sys.stderr))

logger = logging.getLogger("UnrealMCP")

# Configuration — override via environment variables for Docker
UNREAL_HOST = os.environ.get("UNREAL_HOST", "127.0.0.1")
UNREAL_PORT = int(os.environ.get("UNREAL_PORT", "55557"))

# Crash detection state
_last_successful_connection: float = 0.0  # timestamp of last successful TCP connect
_last_crash_check: float = 0.0
_last_crash_info: Optional[Dict[str, Any]] = None


def check_for_editor_crash() -> Optional[Dict[str, Any]]:
    """Check if the Unreal Editor has crashed recently.

    Looks for crash dump files and determines if the editor process
    disappeared unexpectedly (crash vs intentional shutdown).

    Returns crash info dict if crash detected, None otherwise.
    """
    global _last_crash_check, _last_crash_info, _last_successful_connection

    now = time.time()
    # Don't re-check more than once per 5 seconds
    if now - _last_crash_check < 5.0:
        return _last_crash_info
    _last_crash_check = now

    # Only consider it a crash if we had a recent connection
    if _last_successful_connection == 0.0:
        _last_crash_info = None
        return None

    # Check for UE crash logs
    crash_info = _find_recent_crash_log()
    if crash_info:
        _last_crash_info = crash_info
        return crash_info

    # If we had a connection recently (last 60s) and now TCP is dead and process is gone,
    # it's likely a crash even without a crash log
    if now - _last_successful_connection < 60.0:
        if not _is_port_open_quick() and not _is_process_alive():
            _last_crash_info = {
                "crashed": True,
                "message": "Unreal Editor process disappeared unexpectedly (possible crash). "
                           "No crash log found — the editor may have been killed or encountered a silent fatal error.",
                "last_connection": _last_successful_connection,
                "seconds_since_connection": round(now - _last_successful_connection, 1)
            }
            return _last_crash_info

    _last_crash_info = None
    return None


def clear_crash_state():
    """Clear crash detection state after editor restarts successfully."""
    global _last_crash_info
    _last_crash_info = None


def _is_port_open_quick(timeout: float = 1.0) -> bool:
    """Quick TCP port check."""
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(timeout)
            s.connect((UNREAL_HOST, UNREAL_PORT))
            return True
    except (socket.timeout, ConnectionRefusedError, OSError):
        return False


def _is_process_alive() -> bool:
    """Check if UnrealEditor.exe process exists."""
    if platform.system() != "Windows":
        return False
    try:
        import subprocess
        result = subprocess.run(
            ['tasklist', '/FI', 'IMAGENAME eq UnrealEditor.exe', '/NH'],
            capture_output=True, text=True, timeout=5,
            creationflags=0x08000000  # CREATE_NO_WINDOW
        )
        return 'UnrealEditor.exe' in result.stdout
    except Exception:
        return False


def _find_recent_crash_log() -> Optional[Dict[str, Any]]:
    """Look for crash dump files created in the last 2 minutes."""
    import glob
    from pathlib import Path

    crash_dirs = []

    # Project-level crashes
    project_dir = os.environ.get("UNREAL_PROJECT_DIR")
    if not project_dir:
        # Try to infer from cached paths
        server_dir = Path(__file__).resolve().parent.parent
        for uproject in server_dir.glob("*/*.uproject"):
            crash_dirs.append(uproject.parent / "Saved" / "Crashes")
            break
    else:
        crash_dirs.append(Path(project_dir) / "Saved" / "Crashes")

    # Engine-level crashes
    local_app_data = os.environ.get("LOCALAPPDATA", "")
    if local_app_data:
        crash_dirs.append(Path(local_app_data) / "UnrealEngine" / "5.6" / "Saved" / "Crashes")

    now = time.time()
    max_age = 120  # 2 minutes

    for crash_dir in crash_dirs:
        if not crash_dir.exists():
            continue
        try:
            # Look for crash folders (named with timestamps)
            for entry in sorted(crash_dir.iterdir(), reverse=True):
                if not entry.is_dir():
                    continue
                # Check modification time
                mtime = entry.stat().st_mtime
                if now - mtime > max_age:
                    break  # Sorted newest first, so stop early

                # Found a recent crash folder — try to read the log
                crash_log = entry / "CrashContext.runtime-xml"
                error_msg = "Unknown crash"

                if crash_log.exists():
                    try:
                        content = crash_log.read_text(encoding='utf-8', errors='replace')
                        # Extract error message from XML
                        import re
                        match = re.search(r'<ErrorMessage>(.*?)</ErrorMessage>', content, re.DOTALL)
                        if match:
                            error_msg = match.group(1).strip()[:500]
                    except Exception:
                        pass

                # Also check for UE crash log txt
                for log_file in entry.glob("*.log"):
                    try:
                        lines = log_file.read_text(encoding='utf-8', errors='replace').splitlines()
                        # Look for the fatal error line
                        for line in lines[-50:]:
                            if 'Fatal error' in line or 'Assertion failed' in line:
                                error_msg = line.strip()[:500]
                                break
                    except Exception:
                        pass

                return {
                    "crashed": True,
                    "message": f"Unreal Editor crashed: {error_msg}",
                    "crash_folder": str(entry),
                    "crash_time": mtime,
                    "seconds_ago": round(now - mtime, 1)
                }
        except PermissionError:
            continue

    return None

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
            global _last_successful_connection
            _last_successful_connection = time.time()
            clear_crash_state()
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
            # Check if this is a crash rather than the editor simply not running
            crash_info = check_for_editor_crash()
            if crash_info:
                logger.error(f"Editor crash detected: {crash_info.get('message', 'Unknown crash')}")
                return {
                    "status": "error",
                    "error": crash_info["message"],
                    "crash": True,
                    "crash_info": crash_info
                }
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

            # Check if the error was caused by a crash
            crash_info = check_for_editor_crash()
            if crash_info:
                return {
                    "status": "error",
                    "error": crash_info["message"],
                    "crash": True,
                    "crash_info": crash_info
                }

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
    instructions="Unreal Engine integration via Model Context Protocol",
    lifespan=server_lifespan
)

# ── Scope-based tool loading ──────────────────────────────────────────────────
# Import tool registrars and the scope manager
import mcp_scopes
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
from tools.worldbuilding_tools import register_worldbuilding_tools

# Load default scopes from config (falls back to all scopes if config missing)
_scope_config = os.path.join(os.path.dirname(__file__), "tool_scopes.json")
_default_scopes = mcp_scopes.load_config(_scope_config)
if not _default_scopes:
    _default_scopes = ["editor", "assets", "level", "process",
                       "blueprint", "blueprint_nodes", "materials",
                       "animation", "umg", "project"]

mcp_scopes.init(mcp, _default_scopes)

# Register all scopes — inactive ones are hidden from tools/list until activated
mcp_scopes.register_scope("editor",           register_editor_tools)
mcp_scopes.register_scope("assets",           register_asset_tools)
mcp_scopes.register_scope("level",            register_level_tools)
mcp_scopes.register_scope("process",          register_process_tools)
mcp_scopes.register_scope("blueprint",        register_blueprint_tools)
mcp_scopes.register_scope("blueprint_nodes",  register_blueprint_node_tools)
mcp_scopes.register_scope("materials",        register_material_tools)
mcp_scopes.register_scope("animation",        register_anim_tools)
mcp_scopes.register_scope("umg",              register_umg_tools)
mcp_scopes.register_scope("project",          register_project_tools)
mcp_scopes.register_scope("worldbuilding",    register_worldbuilding_tools)

# Auto-discover user tools from UserTools/ directory
def _discover_user_tools(mcp_instance):
    """Scan UserTools/ for .py files with register_tools(mcp) functions."""
    user_dir = os.path.join(os.path.dirname(__file__), "UserTools")
    if not os.path.isdir(user_dir):
        logger.info("UserTools/ directory not found, skipping user tool discovery")
        return
    import importlib.util
    loaded = []
    for f in sorted(os.listdir(user_dir)):
        if f.endswith('.py') and not f.startswith('_'):
            filepath = os.path.join(user_dir, f)
            try:
                spec = importlib.util.spec_from_file_location(f"user_{f[:-3]}", filepath)
                mod = importlib.util.module_from_spec(spec)
                spec.loader.exec_module(mod)
                if hasattr(mod, 'register_tools'):
                    mod.register_tools(mcp_instance)
                    loaded.append(f)
                    logger.info(f"UserTools: loaded {f}")
                else:
                    logger.warning(f"UserTools: {f} has no register_tools() function, skipping")
            except Exception as e:
                logger.error(f"UserTools: failed to load {f}: {e}")
    if loaded:
        logger.info(f"UserTools: loaded {len(loaded)} file(s): {loaded}")

mcp_scopes.register_scope("user",             _discover_user_tools)

# Scope management tools — always available regardless of active scopes
@mcp.tool()
def activate_tool_scope(ctx: Context, scope: str) -> dict:
    """
    Activate a tool scope to add more tools to the active listing.

    Available scopes: editor, assets, level, process, blueprint, blueprint_nodes,
                      materials, animation, umg, project, worldbuilding, user

    Call list_tool_scopes() to see what is currently active and how many tools
    each scope contains.
    """
    return mcp_scopes.activate(scope)

@mcp.tool()
def deactivate_tool_scope(ctx: Context, scope: str) -> dict:
    """
    Deactivate a tool scope to remove its tools from the active listing.
    Reduces context window usage when those tools are no longer needed.
    """
    return mcp_scopes.deactivate(scope)

@mcp.tool()
def list_tool_scopes(ctx: Context) -> dict:
    """
    List all tool scopes, their active status, tool count, and tool names.
    Use activate_tool_scope() to enable scopes needed for your current task.
    """
    return mcp_scopes.list_scopes()

# Tell the scope manager how many always-on tools exist (the 3 above)
mcp_scopes.set_always_on_count(3)

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
    - `inspect_blueprint(blueprint_name, include_*)` - Inspect variables, functions, components, interfaces, event graph
    - `analyze_blueprint_graph(blueprint_name, graph_name)` - Deep graph/node/pin/connection analysis
    - `set_blueprint_metadata(blueprint_name, category, description, ...)` - Set metadata fields

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
    ### Function Management
    - `blueprint_function(blueprint_name, action, function_name)` - Create/delete/rename functions
    - `blueprint_function_param(blueprint_name, function_name, direction, param_name, param_type)` - Add input/output params

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
    transport = os.environ.get("MCP_TRANSPORT", "stdio")
    logger.info(f"Starting MCP server with {transport} transport")
    if transport == "sse":
        mcp.run(transport=transport, host="0.0.0.0", port=8000)
    else:
        mcp.run(transport=transport)