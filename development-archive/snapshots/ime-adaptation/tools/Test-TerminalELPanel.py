"""Non-compiling panel-v1 transport and independent output-invariant checks."""
from pathlib import Path
import math
import random

root = Path(__file__).resolve().parents[1]

def source(path):
    return (root / path).read_text(encoding="utf-8-sig")

# Catch omissions in the hand-maintained WinRT/model/preview chain.
for path in (
    "src/cascadia/inc/ControlProperties.h",
    "src/cascadia/TerminalControl/IControlSettings.idl",
    "src/cascadia/TerminalSettingsModel/GlobalAppSettings.idl",
    "src/cascadia/TerminalSettingsModel/Profile.idl",
    "src/cascadia/TerminalSettingsEditor/GlobalAppearanceViewModel.idl",
    "src/cascadia/TerminalSettingsEditor/ProfileViewModel.idl",
    "src/cascadia/TerminalSettingsEditor/GlobalAppearanceViewModel.h",
    "src/cascadia/TerminalSettingsEditor/ProfileViewModel.h",
    "src/cascadia/TerminalSettingsAppAdapterLib/TerminalSettings.cpp",
    "src/cascadia/TerminalApp/SettingsPaneContent.cpp",
):
    assert "TFELPanelGrid" in source(path), path
assert source("src/cascadia/TerminalSettingsModel/MTSMSettings.h").count('"terminalEL.panelGrid"') == 2
for path in ("src/cascadia/TerminalApp/TFELPreview.h", "src/cascadia/TerminalControl/ControlCore.h", "src/cascadia/TerminalControl/ControlCore.cpp"):
    assert "std::array<float, 18>" in source(path), path
core = source("src/cascadia/TerminalControl/ControlCore.cpp")
assert "values.size() != 18" in core and "v[15], v[16], v[17], 0.0f" in core
assert "clamp(settings.glow.z, 0.0f, 0.15f, 0.0f)" in source("src/renderer/atlas/AtlasEngine.api.cpp")
backend = source("src/renderer/atlas/BackendD3D.cpp")
assert "_tfelParameters.phosphorColor.w = settings.glow.z;" in backend
shader = source("src/renderer/atlas/tfel_native_composite_ps.hlsl")
assert shader.count("return panelV1(") == 3  # zero bloom, zero halo, composed
assert "uint2(position) % 4" in shader
assert "color.rgb *= 1 - phosphorColor.w * separator;" in shader
assert "color.a" not in shader  # alpha/coverage remains unchanged
# Color preset schema remains separate from display adjustment.
presets = source("src/cascadia/TerminalSettingsEditor/TFELPresets.h")
assert 'if (name == L"TFELPanelGrid") return true;' in presets
assert 'L"terminalEL.panelGrid"' not in presets

def panel(rgba, x, y, strength):
    scale = 1 - strength if x % 4 == 0 or y % 4 == 0 else 1
    return tuple(c * scale for c in rgba[:3]) + (rgba[3],)

rng = random.Random(42)
for _ in range(1000):
    alpha = rng.random()
    rgba = tuple(rng.random() * alpha for _ in range(3)) + (alpha,)
    x, y = rng.randrange(4096), rng.randrange(4096)
    assert panel(rgba, x, y, 0) == rgba
    strength = rng.uniform(0, .15)
    out = panel(rgba, x, y, strength)
    assert out[3] == alpha
    assert all(0 <= after <= before <= alpha for before, after in zip(rgba[:3], out[:3]))
    assert math.isclose(out[0] * rgba[1], out[1] * rgba[0], abs_tol=1e-15)
    assert out == panel(rgba, x + 4, y + 4, strength)
for strength in (0, .05, .10, .15):
    mean = sum(panel((1, 1, 1, 1), x, y, strength)[0] for x in range(4) for y in range(4)) / 16
    assert math.isclose(mean, 1 - 7 * strength / 16)
    assert panel((0, 0, 0, 0), 0, 0, strength) == (0, 0, 0, 0)
print("PASS: panel-v1 settings/preview wiring, output routes, off identity, attenuation, RGB ratios, alpha and cell mean.")
print("Arithmetic/source model only; does not compile or execute HLSL or validate GUI behavior.")
