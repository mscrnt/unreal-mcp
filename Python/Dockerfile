FROM python:3.12-slim

# Install uv for fast dependency management
RUN pip install --no-cache-dir uv

WORKDIR /app

# Copy dependency manifest first (layer caching)
COPY pyproject.toml ./

# Install dependencies
RUN uv pip install --system -e .

# Copy application code
COPY . .

# SSE transport on port 8000
ENV MCP_TRANSPORT=sse
ENV UNREAL_HOST=host.docker.internal
ENV UNREAL_PORT=55557
EXPOSE 8000

CMD ["python", "unreal_mcp_server.py"]
