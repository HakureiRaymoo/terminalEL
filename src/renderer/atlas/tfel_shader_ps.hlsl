// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

Texture2D inputFrame : register(t0);
SamplerState inputSampler : register(s0);

cbuffer TFELConstants : register(b0)
{
    // x = brightness, y = contrast, z = blackLevel, w = gammaExponent.
    float4 tone;
    // xyz = phosphor RGB, w = reserved.
    float4 phosphorColor;
    float4 bloom;
    float4 bloomSampling;
    float4 glowColor;
};

static const float3 LUMINANCE_WEIGHTS = float3(0.2126f, 0.7152f, 0.0722f);

float main(float4 pos : SV_POSITION, float2 tex : TEXCOORD) : SV_TARGET
{
    float3 inputColor = inputFrame.Sample(inputSampler, tex).rgb;
    float luminance = dot(inputColor, LUMINANCE_WEIGHTS);

    float level = max(luminance - tone.z, 0.0f);
    level = (level - 0.5f) * tone.y + 0.5f;
    level *= tone.x;

    float emission = pow(saturate(level), tone.w);
    return emission;
}
