"""
World Building tools for Unreal MCP.

Pure Python composition tools that use existing spawn_actor and set_actor_transform
commands to build complex structures from primitive shapes.

Scope: worldbuilding (inactive by default)
"""

import logging
import math
from typing import Any, Dict, List

logger = logging.getLogger("UnrealMCP")

# Default meshes for building blocks
CUBE_MESH = "/Engine/BasicShapes/Cube.Cube"
CYLINDER_MESH = "/Engine/BasicShapes/Cylinder.Cylinder"
CONE_MESH = "/Engine/BasicShapes/Cone.Cone"
SPHERE_MESH = "/Engine/BasicShapes/Sphere.Sphere"
PLANE_MESH = "/Engine/BasicShapes/Plane.Plane"


def _spawn_block(unreal, name: str, location: List[float], scale: List[float],
                 rotation: List[float] = None, mesh: str = CUBE_MESH,
                 material_path: str = None) -> dict:
    """Spawn a StaticMeshActor block and optionally apply a material."""
    params = {
        "name": name,
        "type": "StaticMeshActor",
        "location": location,
    }
    if rotation:
        params["rotation"] = rotation
    result = unreal.send_command("spawn_actor", params)

    # Set the mesh
    unreal.send_command("set_actor_property", {
        "name": name,
        "property_name": "StaticMeshComponent.StaticMesh",
        "property_value": mesh,
    })

    # Set scale
    unreal.send_command("set_actor_transform", {
        "name": name,
        "scale": scale,
    })

    # Apply material if provided
    if material_path:
        unreal.send_command("set_actor_material", {
            "name": name,
            "material_path": material_path,
        })

    return result


def register_worldbuilding_tools(mcp):
    """Register world building tools with the MCP server."""
    from fastmcp import Context

    @mcp.tool()
    def build_basic_structure(
        ctx: Context,
        structure_type: str,
        location: List[float] = None,
        scale: float = 1.0,
        material_path: str = None,
        height: float = 500,
        width: float = 500,
        levels: int = 5,
        steps: int = 10,
        segments: int = 8,
        name_prefix: str = "Structure"
    ) -> Dict[str, Any]:
        """
        Build a basic geometric structure from primitive shapes.

        Args:
            structure_type: One of: pyramid, wall, tower, staircase, arch, column, pillar_ring
            location: [X, Y, Z] world location (default [0,0,0])
            scale: Overall scale multiplier
            material_path: Content path to material (e.g. "/Game/Materials/M_Brick")
            height: Structure height in units
            width: Structure width in units
            levels: Number of tiers/levels (pyramid, tower)
            steps: Number of steps (staircase)
            segments: Number of segments (arch, pillar_ring)
            name_prefix: Prefix for spawned actor names
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            loc = location or [0, 0, 0]
            st = structure_type.lower()
            spawned = []
            block_size = 100 * scale
            idx = 0

            if st == "pyramid":
                for lvl in range(levels):
                    blocks_per_side = levels - lvl
                    z = loc[2] + lvl * block_size
                    offset = (blocks_per_side - 1) * block_size / 2
                    for row in range(blocks_per_side):
                        for col in range(blocks_per_side):
                            name = f"{name_prefix}_Pyr_{idx}"
                            x = loc[0] + col * block_size - offset
                            y = loc[1] + row * block_size - offset
                            _spawn_block(unreal, name, [x, y, z],
                                         [scale, scale, scale],
                                         material_path=material_path)
                            spawned.append(name)
                            idx += 1

            elif st == "wall":
                cols = max(1, int(width / (block_size)))
                rows = max(1, int(height / (block_size)))
                for row in range(rows):
                    for col in range(cols):
                        name = f"{name_prefix}_Wall_{idx}"
                        x = loc[0] + col * block_size
                        z = loc[2] + row * block_size
                        _spawn_block(unreal, name, [x, loc[1], z],
                                     [scale, scale * 0.2, scale],
                                     material_path=material_path)
                        spawned.append(name)
                        idx += 1

            elif st == "tower":
                for lvl in range(levels):
                    z = loc[2] + lvl * block_size * 2
                    # 4 walls per level
                    for side in range(4):
                        name = f"{name_prefix}_Twr_{idx}"
                        angle = side * 90
                        rad = math.radians(angle)
                        offset = width * scale / 2
                        if side == 0:
                            pos = [loc[0] + offset, loc[1], z]
                        elif side == 1:
                            pos = [loc[0], loc[1] + offset, z]
                        elif side == 2:
                            pos = [loc[0] - offset, loc[1], z]
                        else:
                            pos = [loc[0], loc[1] - offset, z]
                        _spawn_block(unreal, name, pos,
                                     [scale * 0.2, scale * (width / 100), scale * 2],
                                     rotation=[0, angle, 0],
                                     material_path=material_path)
                        spawned.append(name)
                        idx += 1

            elif st == "staircase":
                step_height = height / steps * scale
                step_depth = block_size
                for s in range(steps):
                    name = f"{name_prefix}_Stair_{idx}"
                    x = loc[0] + s * step_depth
                    z = loc[2] + s * step_height
                    _spawn_block(unreal, name, [x, loc[1], z],
                                 [scale, scale * (width / 100), scale * 0.3],
                                 material_path=material_path)
                    spawned.append(name)
                    idx += 1

            elif st == "arch":
                radius = width * scale / 2
                for s in range(segments):
                    angle = math.pi * s / (segments - 1) if segments > 1 else 0
                    name = f"{name_prefix}_Arch_{idx}"
                    x = loc[0] + radius * math.cos(angle)
                    z = loc[2] + radius * math.sin(angle)
                    rot_deg = math.degrees(angle) - 90
                    _spawn_block(unreal, name, [x, loc[1], z],
                                 [scale, scale * 0.3, scale],
                                 rotation=[0, 0, rot_deg],
                                 material_path=material_path)
                    spawned.append(name)
                    idx += 1

            elif st == "column":
                col_height = height * scale
                segs = max(1, int(col_height / block_size))
                for s in range(segs):
                    name = f"{name_prefix}_Col_{idx}"
                    z = loc[2] + s * block_size
                    _spawn_block(unreal, name, [loc[0], loc[1], z],
                                 [scale * 0.5, scale * 0.5, scale],
                                 mesh=CYLINDER_MESH,
                                 material_path=material_path)
                    spawned.append(name)
                    idx += 1

            elif st == "pillar_ring":
                radius = width * scale / 2
                for s in range(segments):
                    angle = 2 * math.pi * s / segments
                    name = f"{name_prefix}_Pillar_{idx}"
                    x = loc[0] + radius * math.cos(angle)
                    y = loc[1] + radius * math.sin(angle)
                    _spawn_block(unreal, name, [x, y, loc[2]],
                                 [scale * 0.4, scale * 0.4, scale * (height / 100)],
                                 mesh=CYLINDER_MESH,
                                 material_path=material_path)
                    spawned.append(name)
                    idx += 1
            else:
                return {"success": False,
                        "message": f"Unknown structure_type '{structure_type}'. "
                                   "Use: pyramid, wall, tower, staircase, arch, column, pillar_ring"}

            return {
                "success": True,
                "structure_type": structure_type,
                "actors_spawned": len(spawned),
                "actor_names": spawned[:20],
                "total_actors": len(spawned),
                "note": "Wait ~2s before taking a screenshot — the viewport needs time to render new geometry.",
            }
        except Exception as e:
            logger.error(f"Error building structure: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def build_building(
        ctx: Context,
        building_type: str,
        location: List[float] = None,
        scale: float = 1.0,
        floors: int = 1,
        width: float = 800,
        depth: float = 600,
        floor_height: float = 300,
        material_path: str = None,
        name_prefix: str = "Building"
    ) -> Dict[str, Any]:
        """
        Build a multi-floor building from primitive shapes.

        Args:
            building_type: One of: house, tower_building, fortress
            location: [X, Y, Z] world location
            scale: Overall scale multiplier
            floors: Number of floors
            width: Building width
            depth: Building depth
            floor_height: Height per floor
            material_path: Material to apply
            name_prefix: Prefix for actor names
        """
        from unreal_mcp_server import get_unreal_connection
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            loc = location or [0, 0, 0]
            bt = building_type.lower()
            spawned = []
            idx = 0
            w = width * scale
            d = depth * scale
            fh = floor_height * scale
            wall_thickness = 20 * scale

            def add_floor_slab(floor_z, prefix):
                nonlocal idx
                name = f"{name_prefix}_{prefix}_Floor_{idx}"
                _spawn_block(unreal, name,
                             [loc[0], loc[1], floor_z],
                             [w / 100, d / 100, wall_thickness / 100],
                             material_path=material_path)
                spawned.append(name)
                idx += 1

            def add_walls(floor_z, prefix):
                nonlocal idx
                wall_z = floor_z + fh / 2
                walls = [
                    ([loc[0] + w / 2, loc[1], wall_z], [wall_thickness / 100, d / 100, fh / 100]),
                    ([loc[0] - w / 2, loc[1], wall_z], [wall_thickness / 100, d / 100, fh / 100]),
                    ([loc[0], loc[1] + d / 2, wall_z], [w / 100, wall_thickness / 100, fh / 100]),
                    ([loc[0], loc[1] - d / 2, wall_z], [w / 100, wall_thickness / 100, fh / 100]),
                ]
                for wall_loc, wall_scale in walls:
                    name = f"{name_prefix}_{prefix}_Wall_{idx}"
                    _spawn_block(unreal, name, wall_loc, wall_scale,
                                 material_path=material_path)
                    spawned.append(name)
                    idx += 1

            if bt == "house":
                for f in range(floors):
                    fz = loc[2] + f * fh
                    add_floor_slab(fz, f"F{f}")
                    add_walls(fz, f"F{f}")
                # Roof slab
                add_floor_slab(loc[2] + floors * fh, "Roof")
                # Simple pitched roof (two angled slabs)
                roof_z = loc[2] + floors * fh + fh * 0.3
                for side, angle in [(1, 20), (-1, -20)]:
                    name = f"{name_prefix}_RoofPanel_{idx}"
                    _spawn_block(unreal, name,
                                 [loc[0], loc[1] + side * d * 0.25, roof_z],
                                 [w / 100 * 1.1, d / 100 * 0.6, wall_thickness / 100],
                                 rotation=[angle, 0, 0],
                                 material_path=material_path)
                    spawned.append(name)
                    idx += 1

            elif bt == "tower_building":
                for f in range(floors):
                    fz = loc[2] + f * fh
                    add_floor_slab(fz, f"F{f}")
                    add_walls(fz, f"F{f}")
                # Flat roof
                add_floor_slab(loc[2] + floors * fh, "Roof")
                # Roof parapet
                parapet_z = loc[2] + floors * fh + wall_thickness
                parapet_h = fh * 0.3
                for wall_loc, wall_scale in [
                    ([loc[0] + w / 2, loc[1], parapet_z], [wall_thickness / 100, d / 100 * 1.05, parapet_h / 100]),
                    ([loc[0] - w / 2, loc[1], parapet_z], [wall_thickness / 100, d / 100 * 1.05, parapet_h / 100]),
                    ([loc[0], loc[1] + d / 2, parapet_z], [w / 100 * 1.05, wall_thickness / 100, parapet_h / 100]),
                    ([loc[0], loc[1] - d / 2, parapet_z], [w / 100 * 1.05, wall_thickness / 100, parapet_h / 100]),
                ]:
                    name = f"{name_prefix}_Parapet_{idx}"
                    _spawn_block(unreal, name, wall_loc, wall_scale,
                                 material_path=material_path)
                    spawned.append(name)
                    idx += 1

            elif bt == "fortress":
                # Main keep
                for f in range(floors):
                    fz = loc[2] + f * fh
                    add_floor_slab(fz, f"Keep_F{f}")
                    add_walls(fz, f"Keep_F{f}")
                add_floor_slab(loc[2] + floors * fh, "Keep_Roof")

                # 4 corner towers
                tower_r = 150 * scale
                for corner, (cx, cy) in enumerate([
                    (w / 2, d / 2), (-w / 2, d / 2),
                    (-w / 2, -d / 2), (w / 2, -d / 2)
                ]):
                    tower_floors = floors + 1
                    for tf in range(tower_floors):
                        name = f"{name_prefix}_Tower{corner}_F{tf}_{idx}"
                        tz = loc[2] + tf * fh
                        _spawn_block(unreal, name,
                                     [loc[0] + cx, loc[1] + cy, tz + fh / 2],
                                     [tower_r / 100, tower_r / 100, fh / 100],
                                     mesh=CYLINDER_MESH,
                                     material_path=material_path)
                        spawned.append(name)
                        idx += 1
            else:
                return {"success": False,
                        "message": f"Unknown building_type '{building_type}'. "
                                   "Use: house, tower_building, fortress"}

            return {
                "success": True,
                "building_type": building_type,
                "floors": floors,
                "actors_spawned": len(spawned),
                "actor_names": spawned[:20],
                "total_actors": len(spawned),
                "note": "Wait ~2s before taking a screenshot — the viewport needs time to render new geometry.",
            }
        except Exception as e:
            logger.error(f"Error building structure: {e}")
            return {"success": False, "message": str(e)}

    @mcp.tool()
    def build_infrastructure(
        ctx: Context,
        infra_type: str,
        location: List[float] = None,
        scale: float = 1.0,
        size: float = 2000,
        complexity: int = 3,
        material_path: str = None,
        name_prefix: str = "Infra"
    ) -> Dict[str, Any]:
        """
        Build infrastructure and terrain features from primitives.

        Args:
            infra_type: One of: maze, bridge, aqueduct, arena, road
            location: [X, Y, Z] world location
            scale: Overall scale multiplier
            size: Overall size in units
            complexity: Detail level (1-5, affects density)
            material_path: Material to apply
            name_prefix: Prefix for actor names
        """
        from unreal_mcp_server import get_unreal_connection
        import random
        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            loc = location or [0, 0, 0]
            it = infra_type.lower()
            spawned = []
            idx = 0
            block = 100 * scale

            if it == "maze":
                # Generate a simple maze using recursive backtracker
                grid_size = 3 + complexity * 2  # 5 to 13
                maze = [[1] * grid_size for _ in range(grid_size)]
                visited = [[False] * grid_size for _ in range(grid_size)]

                def carve(x, y):
                    visited[y][x] = True
                    maze[y][x] = 0
                    dirs = [(0, -1), (0, 1), (-1, 0), (1, 0)]
                    random.shuffle(dirs)
                    for dx, dy in dirs:
                        nx, ny = x + dx * 2, y + dy * 2
                        if 0 <= nx < grid_size and 0 <= ny < grid_size and not visited[ny][nx]:
                            maze[y + dy][x + dx] = 0
                            carve(nx, ny)

                # Start from (1, 1)
                start_x = 1 if grid_size > 1 else 0
                start_y = 1 if grid_size > 1 else 0
                carve(start_x, start_y)

                wall_height = 200 * scale
                for row in range(grid_size):
                    for col in range(grid_size):
                        if maze[row][col] == 1:
                            name = f"{name_prefix}_Maze_{idx}"
                            x = loc[0] + col * block
                            y = loc[1] + row * block
                            _spawn_block(unreal, name,
                                         [x, y, loc[2] + wall_height / 2],
                                         [scale, scale, wall_height / 100],
                                         material_path=material_path)
                            spawned.append(name)
                            idx += 1

            elif it == "bridge":
                span = size * scale
                segments = 5 + complexity * 3
                seg_length = span / segments
                bridge_width = 300 * scale
                rail_height = 100 * scale

                for s in range(segments):
                    x = loc[0] + s * seg_length
                    # Deck
                    name = f"{name_prefix}_BDeck_{idx}"
                    _spawn_block(unreal, name,
                                 [x, loc[1], loc[2]],
                                 [seg_length / 100, bridge_width / 100, scale * 0.2],
                                 material_path=material_path)
                    spawned.append(name)
                    idx += 1

                    # Railings (both sides)
                    for side in [1, -1]:
                        name = f"{name_prefix}_BRail_{idx}"
                        _spawn_block(unreal, name,
                                     [x, loc[1] + side * bridge_width / 2, loc[2] + rail_height],
                                     [seg_length / 100 * 0.1, scale * 0.1, rail_height / 100],
                                     material_path=material_path)
                        spawned.append(name)
                        idx += 1

                # Support pillars
                num_pillars = max(2, segments // 3)
                pillar_spacing = span / (num_pillars - 1) if num_pillars > 1 else 0
                pillar_height = 400 * scale
                for p in range(num_pillars):
                    name = f"{name_prefix}_BPillar_{idx}"
                    x = loc[0] + p * pillar_spacing
                    _spawn_block(unreal, name,
                                 [x, loc[1], loc[2] - pillar_height / 2],
                                 [scale * 0.8, scale * 0.8, pillar_height / 100],
                                 mesh=CYLINDER_MESH,
                                 material_path=material_path)
                    spawned.append(name)
                    idx += 1

            elif it == "aqueduct":
                span = size * scale
                arches = 2 + complexity * 2
                arch_width = span / arches
                pillar_height = 500 * scale
                beam_thickness = 50 * scale

                for a in range(arches + 1):
                    # Pillars
                    name = f"{name_prefix}_AqPillar_{idx}"
                    x = loc[0] + a * arch_width
                    _spawn_block(unreal, name,
                                 [x, loc[1], loc[2] + pillar_height / 2],
                                 [scale * 0.6, scale * 0.6, pillar_height / 100],
                                 mesh=CYLINDER_MESH,
                                 material_path=material_path)
                    spawned.append(name)
                    idx += 1

                # Top beam
                for a in range(arches):
                    name = f"{name_prefix}_AqBeam_{idx}"
                    x = loc[0] + a * arch_width + arch_width / 2
                    _spawn_block(unreal, name,
                                 [x, loc[1], loc[2] + pillar_height + beam_thickness / 2],
                                 [arch_width / 100, scale * 1.5, beam_thickness / 100],
                                 material_path=material_path)
                    spawned.append(name)
                    idx += 1

                # Water channel on top
                name = f"{name_prefix}_AqChannel_{idx}"
                _spawn_block(unreal, name,
                             [loc[0] + span / 2, loc[1], loc[2] + pillar_height + beam_thickness],
                             [span / 100, scale * 0.8, beam_thickness / 100 * 0.5],
                             material_path=material_path)
                spawned.append(name)
                idx += 1

            elif it == "arena":
                radius = size * scale / 2
                wall_height = 300 * scale
                segments_count = 8 + complexity * 4
                angle_step = 2 * math.pi / segments_count
                seg_width = 2 * radius * math.sin(angle_step / 2)

                for s in range(segments_count):
                    angle = s * angle_step
                    x = loc[0] + radius * math.cos(angle)
                    y = loc[1] + radius * math.sin(angle)
                    rot_deg = math.degrees(angle) + 90
                    name = f"{name_prefix}_Arena_{idx}"
                    _spawn_block(unreal, name,
                                 [x, y, loc[2] + wall_height / 2],
                                 [seg_width / 100, scale * 0.3, wall_height / 100],
                                 rotation=[0, rot_deg, 0],
                                 material_path=material_path)
                    spawned.append(name)
                    idx += 1

                # Floor
                name = f"{name_prefix}_ArenaFloor_{idx}"
                _spawn_block(unreal, name,
                             [loc[0], loc[1], loc[2]],
                             [radius / 100 * 2, radius / 100 * 2, scale * 0.1],
                             mesh=CYLINDER_MESH,
                             material_path=material_path)
                spawned.append(name)
                idx += 1

            elif it == "road":
                road_length = size * scale
                road_width = 300 * scale
                segments_count = 5 + complexity * 3
                seg_length = road_length / segments_count

                for s in range(segments_count):
                    name = f"{name_prefix}_Road_{idx}"
                    x = loc[0] + s * seg_length
                    _spawn_block(unreal, name,
                                 [x, loc[1], loc[2]],
                                 [seg_length / 100, road_width / 100, scale * 0.05],
                                 material_path=material_path)
                    spawned.append(name)
                    idx += 1
            else:
                return {"success": False,
                        "message": f"Unknown infra_type '{infra_type}'. "
                                   "Use: maze, bridge, aqueduct, arena, road"}

            return {
                "success": True,
                "infra_type": infra_type,
                "actors_spawned": len(spawned),
                "actor_names": spawned[:20],
                "total_actors": len(spawned),
                "note": "Wait ~2s before taking a screenshot — the viewport needs time to render new geometry.",
            }
        except Exception as e:
            logger.error(f"Error building infrastructure: {e}")
            return {"success": False, "message": str(e)}

    logger.info("World building tools registered successfully")
