"""Compare a settings.json scheme with the checked-in clean palette; read only."""
import json
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
expected = json.loads((root / "doc/EL-Amber-Code.json").read_text(encoding="utf-8-sig"))
settings = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8-sig"))
matches = [s for s in settings.get("schemes", []) if s.get("name") == expected["name"]]
assert len(matches) == 1, "Expected exactly one EL Amber Code scheme"
assert matches[0] == expected, "Scheme differs from the clean palette"
print("PASS: unique EL Amber Code scheme exactly matches doc/EL-Amber-Code.json")
print("Default scheme:", settings.get("profiles", {}).get("defaults", {}).get("colorScheme"))
