"""Non-compiling native-color source-contract and arithmetic checks."""
from pathlib import Path
import math
import random
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1]
atlas = root / "src/renderer/atlas"
project = ET.parse(atlas / "atlas.vcxproj")
ns = {"m": "http://schemas.microsoft.com/developer/msbuild/2003"}
shaders = {n.get("Include") for n in project.findall(".//m:FxCompile", ns)}
for name in ("extract", "blur", "composite"):
    file = f"tfel_native_{name}_ps.hlsl"
    assert file in shaders and (atlas / file).is_file(), file
    assert file in (atlas / "makefile.inc").read_text(), file
assert "std::array<float, 18>" in (root / "src/cascadia/TerminalApp/TFELPreview.h").read_text()
assert "values.size() != 18" in (root / "src/cascadia/TerminalControl/ControlCore.cpp").read_text()

# Independent arithmetic model: saturated primaries keep their hue on extraction;
# zero intensity is identity and bounded composition remains premultiplied.
def extract(rgb, threshold=0.3, knee=0.1):
    peak = max(rgb)
    soft = min(max(peak - threshold + knee, 0), 2 * knee)
    amount = max(peak - threshold, soft * soft / max(4 * knee, 1e-6))
    return [c * amount / max(peak, 1e-6) for c in rgb]

for primary in ([1, 0, 0], [0, 1, 0], [0, 0, 1]):
    result = extract(primary)
    assert all(math.isclose(c, p * 0.7) for c, p in zip(result, primary))
assert extract([0, 0, 0]) == [0, 0, 0]

def neutral_weight(rgb):
    high, low = max(rgb), min(rgb)
    saturation = (high - low) / max(high, 1e-6)
    t = min(max((saturation - 0.04) / 0.08, 0), 1)
    return 1 - t * t * (3 - 2 * t)

def hybrid_map(rgb, target):
    weight = neutral_weight(rgb)
    peak = max(rgb)
    return [c * (1 - weight) + t * peak * weight for c, t in zip(rgb, target)]

yellow, orange = [1, 0.82, 0.04], [1, 0.3, 0]
for gray in (0, 0.05, 0.5, 1):
    assert neutral_weight([gray] * 3) == 1
    assert hybrid_map([gray] * 3, yellow) == [c * gray for c in yellow]
    assert hybrid_map([gray] * 3, orange) == [c * gray for c in orange]
for color in ([1, 0, 0], [0, 1, 0], [0, 0, 1], [1, 1, 0], [0.8, 0.7, 0.7]):
    assert neutral_weight(color) == 0
    assert hybrid_map(color, orange) == list(color)
    for coverage in (0.01, 0.1, 0.5):
        assert neutral_weight([c * coverage for c in color]) == 0
assert 0 < neutral_weight([1, 0.92, 0.92]) < 1
# Gray orange emission is colored BEFORE mixing with red emission, so a red
# neighbor cannot turn the shared halo into a newly classified gray/yellow halo.
assert hybrid_map([0.5] * 3, orange)[1] > 0
assert hybrid_map([0.5, 0, 0], orange)[1] == 0

def native_tone(rgb, alpha, brightness=1, contrast=1, black=0, gamma=1):
    if (brightness, contrast, black, gamma) == (1, 1, 0, 1) or alpha <= 0:
        return rgb, alpha
    color = [min(max(c / alpha, 0), 1) for c in rgb]
    peak = max(color)
    level = (max(peak - black, 0) - 0.5) * contrast + 0.5
    level = min(max(level * brightness, 0), 1) ** gamma
    return [(c * level / peak if peak > 0 else level) * alpha for c in color], alpha

sample = [0.3, 0.15, 0.06]
assert native_tone(sample, 1) == (sample, 1)
boosted, _ = native_tone(sample, 1, brightness=2)
assert all(math.isclose(a, b * 2) for a, b in zip(boosted, sample))
assert max(extract(boosted)) > max(extract(sample))
for kwargs in ({"contrast": 1.5}, {"black": 0.1}, {"gamma": 2}):
    toned, _ = native_tone(sample, 1, **kwargs)
    assert toned != sample
    assert math.isclose(toned[1] / toned[0], 0.5)
assert native_tone(sample, 1, brightness=0) == ([0, 0, 0], 1)
assert native_tone([0, 0, 0], 0, brightness=2) == ([0, 0, 0], 0)
translucent, alpha = native_tone([0.2, 0.1, 0], 0.25, brightness=2)
assert max(translucent) <= alpha
for stage in ("extract", "composite"):
    shader = (atlas / f"tfel_native_{stage}_ps.hlsl").read_text()
    assert '#include "tfel_native_tone.hlsli"' in shader and "nativeTone(" in shader

def composite(sharp, alpha, halo, intensity):
    peak = max(halo)
    if intensity <= 0 or peak <= 0:
        return sharp, alpha
    core = max(sharp)
    protection = 1 / (1 + (core / 0.25) ** 4)
    amplitude = math.sqrt(intensity * peak) * protection * 3
    amplitude = min(amplitude / (1 + amplitude / 0.25), max(1 - core, 0))
    rgb = [s + h / peak * amplitude for s, h in zip(sharp, halo)]
    return rgb, max(alpha, *rgb)

# Weak tails should be visibly lifted without brightening saturated cores.
for signal in (1e-6, 1e-4, 0.01):
    rgb, _ = composite([0, 0, 0], 1, [signal, signal / 2, 0], 0.6)
    old_peak = signal * 0.6 / (1 + signal * 0.6 / 0.25)
    assert rgb[0] > old_peak
    assert math.isclose(rgb[1] / rgb[0], 0.5) and rgb[2] == 0
assert composite([1, 0, 0], 1, [0.1, 0.1, 0.1], 2) == ([1, 0, 0], 1)
assert composite([0.2, 0.1, 0], 0.4, [1, 1, 1], 0) == ([0.2, 0.1, 0], 0.4)
assert composite([0, 0, 0], 0, [0, 0, 0], 2) == ([0, 0, 0], 0)
rng = random.Random(0)
for _ in range(1000):
    alpha = rng.random()
    sharp = [rng.random() * alpha for _ in range(3)]
    halo = [rng.random() * 2 for _ in range(3)]
    rgb, out_alpha = composite(sharp, alpha, halo, rng.random() * 2)
    assert all(0 <= s <= c <= out_alpha <= 1 for s, c in zip(sharp, rgb))
print("PASS: native wiring, primary-color extraction, lifted tails, hue ratios, zero-bloom identity, core protection and bounded premultiplied arithmetic model.")
print("This does not execute HLSL or replace manual compilation/GPU acceptance.")
