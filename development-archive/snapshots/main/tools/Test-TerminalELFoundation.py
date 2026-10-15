"""Read-only v1 preset/shader regression guard. Does not compile or run a GPU test."""
from pathlib import Path
import hashlib
import re
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1]
hashes = {
    "tfel_shader_ps.hlsl": "B0EF852DA5BF8F27097C3AA3D3F98F240E111C36E9C74E5862EF4960F128BAA0",
    "tfel_shader_vs.hlsl": "52293BBEFD98D44C086C6982BC6D6B92A9D615BF1859FA30440068DC5842B742",
    "tfel_bloom_extract_ps.hlsl": "FFFC1B018F419F1AD89EC286DCA55E6284801B85C7A68D7EA4EE52372350F750",
    "tfel_bloom_blur_ps.hlsl": "BC94172E0453D5F1C7DC935146F1A17E87A7D40944468564C5AE347561930F95",
    "tfel_composite_ps.hlsl": "F9D53AFAD03940DCAD748C0CD50D49D94A9F9A30740B2DF72C802079BBF223E7",
}
for name, expected in hashes.items():
    # Normalize checkout line endings to avoid false failures after git checkout.
    data = (root / "src/renderer/atlas" / name).read_bytes().replace(b"\r\n", b"\n")
    assert hashlib.sha256(data).hexdigest().upper() == expected, f"v1 shader changed: {name}; retain a versioned v1 path before changing the algorithm"

editor = root / "src/cascadia/TerminalSettingsEditor"
text = (editor / "TFELPresets.h").read_text(encoding="utf-8")
presets = re.findall(r'TFELPreset\{ L"([^"]+)", \{ ([^}]+) \}', text)
assert len(presets) == 4
values = [[float(v.strip().removesuffix("f")) for v in row.split(",")] for _, row in presets]
assert all(len(v) == 16 for v in values)
assert values[0] == [2, 1, 0, 1, .6, .3, 8, .1, 1, .82, .04, 0, 1, 18, 1, 1]
assert values[1] == [1, 1, 0, 1, .6, .3, 8, .1, 1, .68, .12, 0, 1, 18, 1, 0]
ET.parse(editor / "GlobalAppearance.xaml")
idl = (editor / "GlobalAppearanceViewModel.idl").read_text(encoding="utf-8")
assert "IObservableVector<IInspectable> TFELPresetNames" in idl, "XAML preset source must expose an object collection, not vector<String>"
view_model = (editor / "GlobalAppearanceViewModel.cpp").read_text(encoding="utf-8")
assert "names.Append(box_value(hstring{ preset.name }))" in view_model
page = (editor / "GlobalAppearance.cpp").read_text(encoding="utf-8")
assert page.index("TFELPresetPicker().ItemsSource(") < page.index("TFELPresetPicker().SelectedIndex(0)")
print("PASS: Foundation/Classic snapshots, four complete presets, frozen v1 shaders, XAML XML syntax.")
print("C++/WinRT and XAML compilation and visual matching still require a manual build/run.")
