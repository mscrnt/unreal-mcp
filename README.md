<div align="center">

# Model Context Protocol for Unreal Engine
<span style="color: #555555">unreal-mcp</span>

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.5%2B-orange)](https://www.unrealengine.com)
[![Python](https://img.shields.io/badge/Python-3.12%2B-yellow)](https://www.python.org)
[![Status](https://img.shields.io/badge/Status-Experimental-red)](https://github.com/mscrnt/unreal-mcp)

</div>

This project enables AI assistant clients like Cursor, Windsurf, Claude Desktop, and Claude Code to control Unreal Engine through natural language using the Model Context Protocol (MCP).

> **Fork note:** This is a fork of [chongdashu/unreal-mcp](https://github.com/chongdashu/unreal-mcp) with significant expansions — from ~35 tools to **110 tools** — covering materials, assets, levels, animation blueprints, PIE testing, RL agent support, visual feedback via screenshots, Editor Utility Widgets, and more.

## Warning: Experimental Status

This project is currently in an **EXPERIMENTAL** state. The API, functionality, and implementation details are subject to significant changes. While we encourage testing and feedback, please be aware that:

- Breaking changes may occur without notice
- Features may be incomplete or unstable
- Documentation may be outdated or missing
- Production use is not recommended at this time

## Overview

The Unreal MCP integration provides **110 tools** across 10 categories for controlling Unreal Engine through natural language:

| Category | Tools | Capabilities |
|----------|:-----:|-------------|
| **Editor** | 26 | Actor CRUD, transforms, properties, selection, duplication, viewport camera, material assignment (StaticMesh + SkeletalMesh), actor tags, PIE movement input, pawn actions (jump/crouch/launch), viewport screenshots with grid sequences, Editor Utility Widget tab management |
| **Blueprints** | 9 | Create Blueprint classes, add/configure/reparent/remove components, set properties, physics, compile with error reporting |
| **Blueprint Nodes** | 21 | Events, functions, branches, loops, delays, timers, custom events, math ops, variables (get/set/add/remove/change type), pin defaults, self/component references, node connections, node deletion |
| **Level** | 11 | Create/load/save levels, Play-In-Editor (start/stop/query), console commands, build lighting, world settings |
| **Materials** | 12 | Create materials and instances, scalar/vector/texture parameters, 30+ material expression types, expression property editing, node connections, apply to actors, recompile |
| **Assets** | 10 | List/find/duplicate/delete/rename/import/save/open assets, create folders, existence checks |
| **Project** | 7 | Game mode, default maps, Enhanced Input actions and mapping contexts, project settings (read/write) |
| **UMG Widgets** | 6 | Create widget blueprints, text blocks, buttons, event bindings, viewport display, property bindings |
| **Animation** | 7 | Create AnimBPs, state machines, states, transitions, animation assignment, transition rules |
| **Process** | 3 | Stop/start Unreal Editor process, check editor status with MCP connection readiness |

All capabilities are accessible through natural language commands via AI assistants.

## Architecture

```
AI Client (Claude/Cursor/Windsurf)
        |
        | stdio (MCP protocol)
        v
Python MCP Server (FastMCP)           Port 55557 (TCP/JSON)
  tools/editor_tools.py        <---->  C++ Plugin (UnrealMCP)
  tools/blueprint_tools.py               |
  tools/node_tools.py                    +-- UnrealMCPBridge (router)
  tools/level_tools.py                   +-- UnrealMCPEditorCommands
  tools/material_tools.py               +-- UnrealMCPBlueprintCommands
  tools/asset_tools.py                  +-- UnrealMCPBlueprintNodeCommands
  tools/project_tools.py               +-- UnrealMCPLevelCommands
  tools/umg_tools.py                   +-- UnrealMCPMaterialCommands
  tools/anim_tools.py                  +-- UnrealMCPAssetCommands
  tools/process_tools.py              +-- UnrealMCPGameplayCommands
                                        +-- UnrealMCPGameplayCommands
                                        +-- UnrealMCPAnimBlueprintCommands
                                        +-- UnrealMCPCommonUtils (shared)
```

- **C++ Plugin**: Native TCP server running inside Unreal Editor, executes all UE operations on the game thread
- **Python Server**: FastMCP-based stdio server that translates MCP tool calls to JSON commands over TCP
- **JSON Protocol**: `{"type": "command_name", "params": {...}}` / `{"status": "success", "result": {...}}`

## Components

### Sample Project (MCPGameProject)
Based on the Blank Project with the UnrealMCP plugin pre-configured.

### Plugin (UnrealMCP) `MCPGameProject/Plugins/UnrealMCP`
- Native TCP server for MCP communication (port 55557)
- Integrates with Unreal Editor subsystems
- PIE-aware: actor tools auto-detect Play-In-Editor world vs editor world
- Handles command execution and response handling

### Python MCP Server `Python/unreal_mcp_server.py`
- Manages TCP socket connections to the C++ plugin
- Handles command serialization and response parsing
- Loads and registers tool modules from the `tools/` directory
- Uses the FastMCP library to implement the Model Context Protocol

## Directory Structure

```
MCPGameProject/                          # Example Unreal project
  Plugins/UnrealMCP/                     # C++ plugin
    Source/UnrealMCP/
      Public/Commands/                   # Header files
        UnrealMCPEditorCommands.h
        UnrealMCPBlueprintCommands.h
        UnrealMCPBlueprintNodeCommands.h
        UnrealMCPLevelCommands.h
        UnrealMCPMaterialCommands.h
        UnrealMCPAssetCommands.h
        UnrealMCPGameplayCommands.h
        UnrealMCPAnimBlueprintCommands.h
        UnrealMCPCommonUtils.h
      Private/Commands/                  # Implementation files
        (matching .cpp files)
    UnrealMCP.uplugin

Python/                                  # Python MCP server
  unreal_mcp_server.py                   # Main server entry point
  tools/
    editor_tools.py                      # Actor management, viewport, PIE/RL
    blueprint_tools.py                   # Blueprint class creation
    node_tools.py                        # Blueprint node graph authoring
    level_tools.py                       # Level management, PIE
    material_tools.py                    # Material creation and editing
    asset_tools.py                       # Asset management
    project_tools.py                     # Project settings, Enhanced Input
    umg_tools.py                         # UMG Widget Blueprints
    anim_tools.py                        # Animation Blueprints
    process_tools.py                     # Editor process lifecycle (stop/start)

Docs/                                    # Documentation
```

## Quick Start Guide

### Prerequisites
- Unreal Engine 5.5+ (tested on 5.6)
- Python 3.12+
- [uv](https://github.com/astral-sh/uv) (Python package manager)
- MCP Client (Claude Desktop, Claude Code, Cursor, or Windsurf)

### Sample Project

For getting started quickly, use the starter project in `MCPGameProject`:

1. **Prepare the project**
   - Right-click your `.uproject` file
   - Generate Visual Studio project files
2. **Build the project (including the plugin)**
   - Open solution (`.sln`)
   - Choose `Development Editor` as your target
   - Build

### Using the Plugin in Your Own Project

1. **Copy the plugin**
   - Copy `MCPGameProject/Plugins/UnrealMCP` to your project's `Plugins/` folder

2. **Enable the plugin**
   - Edit > Plugins
   - Find "UnrealMCP" in the Editor category
   - Enable and restart the editor

3. **Build**
   - Right-click your `.uproject` file
   - Generate Visual Studio project files
   - Open solution and build

### Python Server Setup

See [Python/README.md](Python/README.md) for detailed setup instructions.

### Configuring Your MCP Client

Add the following to your MCP configuration:

```json
{
  "mcpServers": {
    "unrealMCP": {
      "command": "uv",
      "args": [
        "--directory",
        "<absolute/path/to/this/repo/Python>",
        "run",
        "unreal_mcp_server.py"
      ]
    }
  }
}
```

| MCP Client | Configuration File Location |
|------------|------------------------------|
| Claude Desktop | `%USERPROFILE%\.config\claude-desktop\mcp.json` |
| Claude Code | `.claude/settings.local.json` or `.mcp.json` in project root |
| Cursor | `.cursor/mcp.json` in project root |
| Windsurf | `%USERPROFILE%\.config\windsurf\mcp.json` |

## Tool Reference

### Editor Tools (26)

| Tool | Description |
|------|-------------|
| `get_actors_in_level` | Get a list of all actors in the current level |
| `find_actors_by_name` | Find actors by name pattern |
| `spawn_actor` | Create a new actor (StaticMeshActor, PointLight, etc.) |
| `delete_actor` | Delete an actor by name |
| `set_actor_transform` | Set position, rotation, and scale |
| `get_actor_properties` | Get all properties of an actor |
| `set_actor_property` | Set a property on an actor or its components (supports arrays, enums, structs, objects) |
| `spawn_blueprint_actor` | Spawn an actor from a Blueprint |
| `select_actors` | Select actors in the editor by name |
| `get_selected_actors` | Get the currently selected actors |
| `duplicate_actor` | Duplicate an actor with optional new location |
| `set_viewport_camera` | Set editor viewport camera position and rotation |
| `get_viewport_camera` | Get current viewport camera transform |
| `set_actor_mobility` | Set mobility (Static, Stationary, Movable) |
| `set_actor_material` | Set material on StaticMesh or SkeletalMesh components |
| `set_actor_tags` | Set, add, or remove actor tags (for RL identification, etc.) |
| `get_actor_tags` | Get all tags on an actor |
| `add_movement_input` | Inject movement into a Pawn during PIE (RL training) |
| `pawn_action` | Execute pawn actions during PIE (jump, crouch, launch) |
| `take_screenshot` | Capture the viewport (game view during PIE, editor otherwise) and return as an image |
| `take_screenshot_sequence` | Capture N screenshots over time and return as a single grid image |
| `run_editor_utility` | Run an Editor Utility Blueprint or Widget |
| `spawn_editor_utility_tab` | Open an Editor Utility Widget as an editor tab |
| `close_editor_utility_tab` | Close an Editor Utility Widget tab by ID |
| `does_editor_utility_tab_exist` | Check if an Editor Utility Widget tab is open |
| `find_editor_utility_widget` | Find the widget instance from a spawned Editor Utility tab |

### Blueprint Tools (9)

| Tool | Description |
|------|-------------|
| `create_blueprint` | Create a new Blueprint class |
| `add_component_to_blueprint` | Add a component to a Blueprint |
| `set_static_mesh_properties` | Set mesh on a StaticMeshComponent |
| `set_component_property` | Set a property on a Blueprint component |
| `set_physics_properties` | Configure physics (simulate, mass, damping, gravity) |
| `compile_blueprint` | Compile a Blueprint (returns errors and warnings) |
| `set_blueprint_property` | Set a property on the Blueprint class defaults (CDO) |
| `reparent_blueprint_component` | Reparent (attach) a component to a different parent component |
| `remove_blueprint_component` | Remove a component from a Blueprint (optionally promotes children) |

### Blueprint Node Tools (21)

| Tool | Description |
|------|-------------|
| `add_blueprint_event_node` | Add event nodes (BeginPlay, Tick, etc.) |
| `add_blueprint_input_action_node` | Add input action event nodes |
| `add_blueprint_function_node` | Add function call nodes |
| `connect_blueprint_nodes` | Connect two nodes by pin name |
| `add_blueprint_variable` | Add a variable (Boolean, Integer, Float, Vector, etc.) |
| `add_blueprint_get_self_component_reference` | Get reference to an owned component |
| `add_blueprint_self_reference` | Get reference to self (this actor) |
| `find_blueprint_nodes` | Find nodes by type or event name |
| `add_blueprint_branch_node` | Add a Branch (if/else) node |
| `add_blueprint_for_loop_node` | Add a ForLoop macro node |
| `add_blueprint_delay_node` | Add a Delay node |
| `add_blueprint_print_string_node` | Add a PrintString node (debugging) |
| `add_blueprint_set_timer_node` | Add a SetTimerByFunctionName node |
| `add_blueprint_custom_event_node` | Add a Custom Event node |
| `add_blueprint_variable_get_node` | Add a Variable Get node |
| `add_blueprint_variable_set_node` | Add a Variable Set node |
| `set_node_pin_default_value` | Set the default value of a node pin |
| `add_blueprint_math_node` | Add math operations (+, -, *, /, >, <, ==, !=) |
| `remove_blueprint_variable` | Remove a variable from a Blueprint |
| `change_blueprint_variable_type` | Change a variable's type |
| `delete_blueprint_node` | Delete a node from a Blueprint graph by ID |

### Level Tools (11)

| Tool | Description |
|------|-------------|
| `new_level` | Create a new level (optionally from template) |
| `load_level` | Load an existing level |
| `save_level` | Save the current level |
| `save_all_levels` | Save all modified levels |
| `get_current_level` | Get current level name and path |
| `play_in_editor` | Start Play-In-Editor (PIE) |
| `stop_play_in_editor` | Stop the PIE session |
| `is_playing` | Check if PIE is active |
| `execute_console_command` | Run a console command (e.g., `stat fps`) |
| `build_lighting` | Build lighting (Preview, Medium, High, Production) |
| `set_world_settings` | Set game mode, kill Z, etc. |

### Material Tools (12)

| Tool | Description |
|------|-------------|
| `create_material` | Create a new Material asset |
| `create_material_instance` | Create a Material Instance from a parent material |
| `set_material_scalar_param` | Set scalar parameter on a material instance |
| `set_material_vector_param` | Set vector/color parameter on a material instance |
| `set_material_texture_param` | Set texture parameter on a material instance |
| `add_material_expression` | Add expression nodes (30+ types: Constant, Multiply, Lerp, Fresnel, Panner, TextureSample, ComponentMask, If, etc.) |
| `connect_material_expressions` | Connect two expression nodes |
| `connect_material_property` | Connect expression to material output (BaseColor, Roughness, etc.) |
| `apply_material_to_actor` | Apply material to an actor's mesh |
| `recompile_material` | Recompile a material after graph changes |
| `set_material_expression_property` | Set a property on an expression node (texture, atlas, mask flags, etc.) |
| `get_material_expressions` | List all expression nodes in a material with index, name, class, and caption |

### Asset Tools (10)

| Tool | Description |
|------|-------------|
| `list_assets` | List assets in a content browser path |
| `find_asset` | Search for assets by name |
| `does_asset_exist` | Check if an asset exists |
| `duplicate_asset` | Duplicate an asset |
| `delete_asset` | Delete an asset |
| `rename_asset` | Rename/move an asset |
| `create_folder` | Create a content browser folder |
| `import_asset` | Import an external file (textures, meshes, etc.) |
| `save_asset` | Save a specific asset to disk |
| `open_asset` | Open an asset in its editor (Blueprint Editor, Material Editor, etc.) |

### Project & Gameplay Tools (7)

| Tool | Description |
|------|-------------|
| `create_input_mapping` | Create a legacy input mapping |
| `set_default_game_mode` | Set the project's default game mode |
| `set_default_map` | Set the default game/editor map |
| `create_enhanced_input_action` | Create an Enhanced Input Action asset |
| `create_input_mapping_context` | Create an Input Mapping Context with key bindings |
| `set_project_setting` | Set a project config value (INI) |
| `get_project_setting` | Read a project config value (INI) |

### UMG Widget Tools (6)

| Tool | Description |
|------|-------------|
| `create_umg_widget_blueprint` | Create a new Widget Blueprint |
| `add_text_block_to_widget` | Add a Text Block to a widget |
| `add_button_to_widget` | Add a Button to a widget |
| `bind_widget_event` | Bind a widget event (OnClicked, etc.) to a function |
| `add_widget_to_viewport` | Display a widget on screen |
| `set_text_block_binding` | Bind a text block to a property |

### Animation Tools (7)

| Tool | Description |
|------|-------------|
| `create_anim_blueprint` | Create an Animation Blueprint for a skeleton |
| `add_anim_state_machine` | Add a state machine to an AnimBP |
| `add_anim_state` | Add a state to a state machine |
| `set_anim_state_animation` | Set the animation played by a state |
| `add_anim_transition` | Add a transition between states |
| `get_anim_blueprint_info` | Get detailed AnimBP information |
| `set_anim_transition_rule` | Set a condition rule on a transition |

### Process Tools (3)

| Tool | Description |
|------|-------------|
| `stop_unreal_editor` | Stop the Unreal Editor process (optionally saves first), caches paths for restart |
| `start_unreal_editor` | Launch Unreal Editor with auto-discovery of editor and project paths |
| `is_unreal_editor_running` | Check if the editor is running and MCP TCP connection is available |

## PIE and RL Training Support

This fork includes tools specifically designed for reinforcement learning workflows during Play-In-Editor:

- **PIE-aware actor tools**: `get_actors_in_level`, `find_actors_by_name`, `set_actor_transform`, and `get_actor_properties` all automatically detect and operate on the PIE game world
- **Movement injection**: `add_movement_input` sends single-frame movement to any Pawn
- **Discrete actions**: `pawn_action` triggers jump, crouch, uncrouch, or launch on Characters
- **Actor tagging**: `set_actor_tags` / `get_actor_tags` for identifying RL agents by tag

Typical RL loop:
1. **Read state**: `find_actors_by_name` + `get_actor_properties` (position, velocity)
2. **Apply action**: `add_movement_input` (move) + `pawn_action("jump")` (jump)
3. **Check done**: position checks via properties
4. **Reset**: `set_actor_transform` to teleport back to start

## Visual Feedback (Screenshots)

The AI agent can see what's happening in the editor or during gameplay:

- **`take_screenshot`** — captures the current viewport and returns it as an MCP image that the AI can see directly. During PIE, it captures the game camera view instead of the editor viewport.
- **`take_screenshot_sequence(count=12, interval=0.5, columns=4)`** — captures multiple screenshots over time and composites them into a grid. Default: 12 frames at 0.5s intervals = 6 seconds of gameplay in a single 4x3 image.

This enables the AI to visually verify level design, debug gameplay, observe RL agent behavior, and confirm that materials/lighting look correct — all without the user having to describe what they see.

> **Note**: `take_screenshot_sequence` requires [Pillow](https://pillow.readthedocs.io/) (included in dependencies) for grid compositing.

## Property Type Support

`set_actor_property` and `set_blueprint_property` support a wide range of UE property types:

| Type | JSON Value Format |
|------|-------------------|
| Bool | `true` / `false` |
| Int, Float, Double | Number |
| String | `"text"` |
| FName | `"text"` |
| Byte/Enum | Number or `"EnumValueName"` |
| FVector | `[x, y, z]` |
| FRotator | `[pitch, yaw, roll]` |
| FVector2D | `[x, y]` |
| FColor | `[R, G, B]` or `[R, G, B, A]` (0-255) |
| FLinearColor | `[R, G, B]` or `[R, G, B, A]` (0.0-1.0) |
| FTransform | `{"location": [...], "rotation": [...], "scale": [...]}` |
| UObject ref | `"/Game/Path/To/Asset"` |
| UClass ref | `"/Game/Path/To/Blueprint"` or `"/Script/Module.ClassName"` |
| Soft Object/Class | `"/Game/Path/To/Asset"` |
| TArray\<FName\> | `["Tag1", "Tag2"]` |
| TArray\<FString\> | `["str1", "str2"]` |
| TArray\<int/float/double\> | `[1, 2, 3]` |
| TArray\<bool\> | `[true, false]` |
| TArray\<UObject*\> | `["/Game/Path/Mat1", "/Game/Path/Mat2"]` |

## License

MIT

## Credits

- Original project by [@chongdashu](https://www.x.com/chongdashu)
- Fork expansions by [@mscrnt](https://github.com/mscrnt)
