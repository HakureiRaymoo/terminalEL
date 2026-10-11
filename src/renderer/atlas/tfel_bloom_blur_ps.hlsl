// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

Texture2D<float> bloomFrame : register(t0);
SamplerState inputSampler : register(s0);
cbuffer TFELConstants : register(b0)
{
    float4 tone;
    float4 phosphorColor;
    float4 bloom;
    float4 bloomSampling;
    float4 glowColor;
};

float main(float4 pos : SV_POSITION, float2 tex : TEXCOORD) : SV_TARGET
{
    // Nine normalized Gaussian-style taps; radius is the full-resolution support.
    static const float weights[5] = { 0.2270270270f, 0.1945945946f, 0.1216216216f, 0.0540540541f, 0.0162162162f };
    // Four taps on either side, at half resolution: spacing = radius / (4 * 2).
    // Match the 0..16 px range exposed by both settings pages.
    float spacing = clamp(bloom.z, 0.0f, 16.0f) / 8.0f;
    float2 step = bloomSampling.xy * bloomSampling.zw * spacing;
    float result = bloomFrame.Sample(inputSampler, tex) * weights[0];
    // Explicit taps preserve the kernel at fractional spacing; paired bilinear
    // optimization would only be exact for adjacent texel-centered taps.
    [unroll] for (int i = 1; i <= 4; ++i)
    {
        result += weights[i] * bloomFrame.Sample(inputSampler, tex + step * i);
        result += weights[i] * bloomFrame.Sample(inputSampler, tex - step * i);
    }
    return result;
}
