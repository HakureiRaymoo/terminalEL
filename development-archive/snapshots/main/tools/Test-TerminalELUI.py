"""Read-only resource/binding checks; does not compile or launch Terminal."""
from pathlib import Path
import re
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1]
editor = root / "src/cascadia/TerminalSettingsEditor"
resources = {}
for language in ("en-US", "zh-CN"):
    entries = ET.parse(editor / "Resources" / language / "Resources.resw").getroot().findall("data")
    keys = [entry.attrib["name"] for entry in entries]
    assert len(keys) == len(set(keys)), f"Duplicate resources: {language}"
    resources[language] = set(keys)

xaml = ET.parse(editor / "GlobalAppearance.xaml").getroot()
uid_key = "{http://schemas.microsoft.com/winfx/2006/xaml}Uid"
for element in xaml.iter():
    uid = element.get(uid_key, "")
    if uid.startswith("TFEL") or uid.startswith("Globals_TFEL") or "TerminalEL" in uid:
        for language, keys in resources.items():
            assert any(key.startswith(uid + ".") for key in keys), (language, uid)

for filename in ("GlobalAppearance.cpp", "GlobalAppearanceViewModel.cpp"):
    text = (editor / filename).read_text(encoding="utf-8-sig")
    for key in re.findall(r'RS_\(L"(TFEL[^"\n]+)"\)', text):
        for language, keys in resources.items():
            assert key in keys, (filename, language, key)

sliders = {}
numbers = {}
for element in xaml.iter():
    value = element.get("Value", "")
    match = re.search(r"ViewModel\.(TFEL\w+)", value)
    if not match:
        continue
    name = match[1]
    if element.tag.endswith("}Slider"):
        sliders[name] = element
    elif element.tag.endswith("}NumberBox"):
        numbers[name] = element
        assert "Mode=OneWay" in value, name
        assert element.get("Tag") == name, name
        assert element.get("ValueChanged") == "TFELNumber_ValueChanged", name
assert len(numbers) == 13 and numbers.keys() == sliders.keys()
for name, number in numbers.items():
    for bound in ("Minimum", "Maximum"):
        assert float(number.get(bound)) == float(sliders[name].get(bound)), (name, bound)

profile = ET.parse(editor / "Profiles_Appearance.xaml").getroot()
profile_numbers = {}
profile_sliders = {}
reset_names = set()
for element in profile.iter():
    uid = element.get(uid_key, "")
    if uid.startswith("TFEL"):
        for language, keys in resources.items():
            assert any(key.startswith(uid + ".") for key in keys), (language, uid)
    if element.tag.endswith("}SettingResetButton"):
        reset_names.add(element.get("SettingName"))
    match = re.search(r"Profile\.(TFEL\w+)", element.get("Value", ""))
    if not match:
        continue
    name = match[1]
    if element.tag.endswith("}Slider"):
        profile_sliders[name] = element
    elif element.tag.endswith("}NumberBox"):
        profile_numbers[name] = element
        assert "Mode=OneWay" in element.get("Value"), name
        assert element.get("Tag") == name, name
        assert element.get("ValueChanged") == "TFELNumber_ValueChanged", name
assert set(profile_numbers) == {"TFELHue", "TFELSaturation", "TFELGlowHue", "TFELGlowSaturation", "TFELGlowMix"}
for name, number in profile_numbers.items():
    assert name in reset_names, name
    for bound in ("Minimum", "Maximum"):
        assert float(number.get(bound)) == float(profile_sliders[name].get(bound)) == float(numbers[name].get(bound)), (name, bound)
assert len([e for e in profile.iter() if e.get("SettingName") == "TFELGlowMix"]) == 2

print("PASS: unique resources, en-US/zh-CN TFEL coverage, 13 global numeric pairs, 5 profile color pairs, matching bounds and reset targets.")
print("C++/WinRT compilation, XAML resource loading, focus/validation and Save/Reset still require GUI acceptance.")
