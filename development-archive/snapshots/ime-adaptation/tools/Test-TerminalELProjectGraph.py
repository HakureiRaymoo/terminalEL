"""Read-only project-reference cycle check; never invokes a build tool."""
from pathlib import Path
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
NS = {"m": "http://schemas.microsoft.com/developer/msbuild/2003"}
seen = set()
active = []


def visit(project):
    project = project.resolve()
    if project in active:
        raise AssertionError("Project reference cycle: " + " -> ".join(map(str, active + [project])))
    if project in seen:
        return
    active.append(project)
    for ref in ET.parse(project).findall(".//m:ProjectReference", NS):
        path = ref.attrib["Include"]
        for variable in ("$(OpenConsoleDir)", "$(SolutionDir)"):
            path = path.replace(variable, str(ROOT) + "/")
        assert "$(" not in path, f"Unresolved project reference: {path}"
        visit(project.parent / path.replace("\\", "/"))
    active.pop()
    seen.add(project)


if __name__ == "__main__":
    visit(ROOT / "src/cascadia/WindowsTerminal/WindowsTerminal.vcxproj")
    print(f"PASS: {len(seen)} referenced project XML files parsed; no project-reference cycles.")
    print("This checks explicit references only, not MSBuild evaluation or compilation.")
