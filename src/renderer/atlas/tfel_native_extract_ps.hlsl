// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
// native-el-v1: threshold the composed terminal frame, retaining source hue.
#include "tfel_native_tone.hlsli"
Texture2D<float4> inputFrame : register(t0);
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
    uint width, height;
    inputFrame.GetDimensions(width, height);
    int2 base = int2(pos.xy) * 2;
    float3 sum = 0;
    float count = 0;
    float threshold = max(bloom.y, 0.0001f);
    float knee = min(bloom.w, threshold * 0.999f);
    [unroll] for (int y = 0; y < 2; ++y)
    {
        [unroll] for (int x = 0; x < 2; ++x)
        {
            int2 pixel = base + int2(x, y);
            if (pixel.x < int(width) && pixel.y < int(height))
            {
                float4 source = inputFrame.Load(int3(pixel, 0));
                float3 rgb = nativeTone(source, tone).rgb;
                float peak = max(rgb.r, max(rgb.g, rgb.b));
                // Negative glowColor.w selects hybrid mode. Recolor emission
                // before blur so orange neutral halos and colored halos mix
                // spatially without reclassifying their blurred result.
                if (glowColor.w < 0)
                    rgb = lerp(rgb, glowColor.rgb * peak, nativeNeutralWeight(source.rgb));
                float soft = clamp(peak - threshold + knee, 0, 2 * knee);
                soft = soft * soft / max(4 * knee, 0.000001f);
                sum += rgb * (max(peak - threshold, soft) / max(peak, 0.000001f));
                count += 1;
            }
        }
    }
    return float4(sum / max(count, 1), 0);
}
