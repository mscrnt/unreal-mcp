# UserTools — Custom MCP Tool Extensions

Drop `.py` files here to add custom tools to the Unreal MCP server.

## Convention

Each file must define a `register_tools(mcp)` function that registers tools
using the `@mcp.tool()` decorator:

```python
# my_tools.py
import logging
from typing import Any, Dict
logger = logging.getLogger("UnrealMCP")

def register_tools(mcp):
    from fastmcp import Context

    @mcp.tool()
    def my_custom_tool(ctx: Context, param: str) -> Dict[str, Any]:
        """Description of what this tool does."""
        from unreal_mcp_server import get_unreal_connection
        unreal = get_unreal_connection()
        if not unreal:
            return {"success": False, "message": "Not connected"}
        # Use unreal.send_command("command_name", {...}) to talk to UE
        return {"success": True, "result": "done"}

    logger.info("My custom tools registered")
```

## Activation

User tools are loaded into the `user` scope (inactive by default).
Activate with: `activate_tool_scope("user")`

## Notes

- Files starting with `_` are ignored
- Files are loaded in alphabetical order
- The `unreal` module from `get_unreal_connection()` provides `send_command()`
- All existing C++ commands are available via `send_command()`
