// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#include "tfel_native_tone.hlsli"
Texture2D<float4> sharpFrame : register(t0);
Texture2D<float4> bloomFrame : register(t1);
SamplerState inputSampler : register(s0);
cbuffer TFELConstants : register(b0)
{
    float4 tone;
    float4 phosphorColor;
    float4 bloom;
    float4 bloomSampling;
    float4 glowColor;
};
// panel-v1: static 4x4 render-pixel cell, one-pixel separator, no RGB mask.
// Modulate final RGB only: alpha/coverage and bloom extraction stay unchanged.
float4 panelV1(float4 color, float2 position)
{
    if (phosphorColor.w > 0)
    {
        uint2 cell = uint2(position) % 4;
        float separator = (cell.x == 0 || cell.y == 0) ? 1.0f : 0.0f;
        color.rgb *= 1 - phosphorColor.w * separator;
    }
    return color;
}
float4 main(float4 pos : SV_POSITION, float2 tex : TEXCOORD) : SV_TARGET
{
    float4 source = sharpFrame.Load(int3(int2(pos.xy), 0));
    float4 sharp = nativeTone(source, tone);
    if (glowColor.w < 0)
    {
        float level = max(sharp.r, max(sharp.g, sharp.b));
        sharp.rgb = lerp(sharp.rgb, phosphorColor.rgb * level, nativeNeutralWeight(source.rgb));
    }
    if (bloom.x <= 0) return panelV1(sharp, pos.xy);
    uint width, height;
    sharpFrame.GetDimensions(width, height);
    float3 halo = max(bloomFrame.Sample(inputSampler, pos.xy / float2(width, height)).rgb, 0);
    float peak = max(halo.r, max(halo.g, halo.b));
    // Hybrid emission already has its correct source-specific color.
    if (glowColor.w >= 0)
        halo = lerp(halo, glowColor.rgb * peak, glowColor.w);
    // Lift weak, broad tails like monochrome v1, but apply the nonlinear
    // envelope to a scalar peak, never per RGB channel (which would shift hue).
    float tintedPeak = max(halo.r, max(halo.g, halo.b));
    if (tintedPeak <= 0) return panelV1(sharp, pos.xy);
    float sharpPeak = max(sharp.r, max(sharp.g, sharp.b));
    float coreRatio = max(sharpPeak, 0) / 0.25f;
    float coreRatioSquared = coreRatio * coreRatio;
    float coreContribution = 1 / (1 + coreRatioSquared * coreRatioSquared);
    float amplitude = sqrt(max(bloom.x * tintedPeak, 0)) * coreContribution * 3.0f;
    amplitude = amplitude / (1 + amplitude / 0.25f);
    amplitude = min(amplitude, max(1 - sharpPeak, 0));
    halo = (halo / tintedPeak) * amplitude;
    // Scalar headroom preserves halo RGB ratios and protects bright text cores.
    // Raise alpha only as needed to retain premultiplied output.
    float3 rgb = sharp.rgb + halo;
    float alpha = max(sharp.a, max(rgb.r, max(rgb.g, rgb.b)));
    return panelV1(float4(rgb, alpha), pos.xy);
}
