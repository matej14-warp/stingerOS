import os
from mcp.server.fastmcp import FastMCP


mcp = FastMCP("FileOpsServer")

@mcp.tool()
def goob_search_files(root_dir: str, pattern: str) -> list:
    """
    recursively searches for files containing the pattern in their filename.
    returns a list of absolute paths.
    """
    matches = []
    for root, _, files in os.walk(root_dir):
        for file in files:
            if pattern.lower() in file.lower():
                matches.append(os.path.join(root, file))
    return matches

@mcp.tool()
def goob_read_file(path: str) -> str:
    """reads the entire content of a file at the specified path."""
    with open(path, "r") as f:
        return f.read()

@mcp.tool()
def goob_write_file(path: str, content: str) -> str:
    """overwrites or creates a file with new content at the specified path."""
    dir_name = os.path.dirname(os.path.abspath(path))
    if dir_name:
        os.makedirs(dir_name, exist_ok=True)
    with open(path, "w") as f:
        f.write(content)
    return f"successfully wrote to {path}"

@mcp.tool()
def goob_partial_read(path: str, offset: int, length: int) -> str:
    """reads a specific chunk of a file starting at offset."""
    with open(path, "r") as f:
        f.seek(offset)
        return f.read(length)

@mcp.tool()
def goob_partial_write(path: str, offset: int, content: str) -> str:
    """writes content at a specific byte offset without wiping the file."""
    with open(path, "r+") as f:
        f.seek(offset)
        f.write(content)
    return f"updated {path} at offset {offset}"

@mcp.tool()
def goob_search_in_file(path: str, query: str) -> list:
    """searches for a string and returns line numbers."""
    results = []
    with open(path, "r") as f:
        for i, line in enumerate(f, 1):
            if query in line:
                results.append({"line": i, "content": line.strip()})
    return results

if __name__ == "__main__":
    mcp.run()



