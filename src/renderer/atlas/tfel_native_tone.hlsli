// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
// Shared by extraction and composition: both must see the same toned frame.
// Classify BEFORE tone and before downsampling/blur. Relative chroma makes the
// result invariant under coverage scaling against a black background.
float nativeNeutralWeight(float3 source)
{
    float high = max(source.r, max(source.g, source.b));
    float low = min(source.r, min(source.g, source.b));
    float saturation = (high - low) / max(high, 0.000001f);
    return 1.0f - smoothstep(0.04f, 0.12f, saturation);
}

float4 nativeTone(float4 source, float4 tone)
{
    // Initialize the full result and use a single return: FXC can report X4000
    // for early returns when this function is inlined into the extraction loop.
    float4 outputColor = source;
    if (any(tone != float4(1, 1, 0, 1)) && source.a > 0)
    {
        float safeAlpha = source.a > 0 ? source.a : 1.0f;
        float3 color = saturate(source.rgb / safeAlpha);
        float peak = max(color.r, max(color.g, color.b));
        float level = max(peak - tone.z, 0);
        level = (level - 0.5f) * tone.y + 0.5f;
        level = pow(saturate(level * tone.x), tone.w);
        // Safe even if the compiler flattens the conditional expression.
        float safePeak = peak > 0 ? peak : 1.0f;
        float3 result = peak > 0 ? color * (level / safePeak) : float3(level, level, level);
        outputColor = float4(result * source.a, source.a);
    }
    // Neutral tone preserves exact source RGBA, including antialiased edges.
    return outputColor;
}
