// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

Texture2D<float> sharpFrame : register(t0);
Texture2D<float> bloomFrame : register(t1);
SamplerState inputSampler : register(s0);
cbuffer TFELConstants : register(b0)
{
    float4 tone;
    float4 phosphorColor;
    float4 bloom;
    float4 bloomSampling;
    float4 glowColor;
};

float4 main(float4 pos : SV_POSITION, float2 tex : TEXCOORD) : SV_TARGET
{
    float sharp = sharpFrame.Load(int3(int2(pos.xy), 0));
    // Explicit zero-intensity path preserves the V0.2 tone result without
    // introducing halo arithmetic or another clamp into the sharp path.
    float halo = 0.0f;
    if (bloom.x > 0.0f)
    {
        uint sharpWidth, sharpHeight;
        sharpFrame.GetDimensions(sharpWidth, sharpHeight);
        // Map full-resolution pixel centers into the half-resolution Bloom
        // texture. Using the sharp dimensions also handles odd target sizes.
        float2 uv = pos.xy / float2(sharpWidth, sharpHeight);
        float haloCandidate = max(bloom.x * bloomFrame.Sample(inputSampler, uv), 0.0f);

        // Continuous luminance envelope: preserve bright cores without a
        // threshold mask, edge subtraction, or an excavated ring around glyphs.
        static const float CORE_SCALE = 0.25f;
        float coreRatio = max(sharp, 0.0f) / CORE_SCALE;
        float coreRatioSquared = coreRatio * coreRatio;
        float coreContribution = 1.0f / (1.0f + coreRatioSquared * coreRatioSquared);
        // Lift the low end continuously so the broad tail remains visible
        // while the core protection still prevents a blurred glyph copy.
        static const float HALO_LIFT = 0.50f;
        static const float HALO_AMPLITUDE = 3.0f;
        halo = pow(haloCandidate, HALO_LIFT) * coreContribution * HALO_AMPLITUDE;

        // Soft asymptotic peak limit keeps dark counters and reverse-video
        // text from receiving an unrestricted copy of nearby bright emission.
        static const float HALO_MAXIMUM = 0.25f;
        halo = halo / (1.0f + halo / HALO_MAXIMUM);
        // Cap emission before coloring. With glowColor == phosphorColor this
        // exactly preserves the legacy single-color compositing equation.
        halo = min(halo, max(1.0f - sharp, 0.0f));
    }
    return float4(phosphorColor.rgb * sharp + glowColor.rgb * halo, 1.0f);
}
