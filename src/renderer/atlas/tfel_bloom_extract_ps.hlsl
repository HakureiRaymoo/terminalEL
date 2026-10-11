// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

Texture2D<float> emissionFrame : register(t0);
cbuffer TFELConstants : register(b0)
{
    float4 tone;
    float4 phosphorColor;
    float4 bloom;
    float4 bloomSampling;
    float4 glowColor;
};

float extract(float emission)
{
    float threshold = clamp(bloom.y, 0.0001f, 1.0f);
    float knee = clamp(bloom.w, 0.0f, threshold * 0.999f);
    float soft = clamp(emission - threshold + knee, 0.0f, 2.0f * knee);
    // At zero knee the numerator is zero: this becomes the hard threshold.
    soft = soft * soft / max(4.0f * knee, 0.000001f);
    return max(emission - threshold, soft);
}

float main(float4 pos : SV_POSITION, float2 tex : TEXCOORD) : SV_TARGET
{
    uint width, height;
    emissionFrame.GetDimensions(width, height);
    int2 base = int2(pos.xy) * 2;
    float sum = 0.0f;
    float count = 0.0f;
    [unroll] for (int y = 0; y < 2; ++y)
    {
        [unroll] for (int x = 0; x < 2; ++x)
        {
            int2 pixel = base + int2(x, y);
            if (pixel.x < int(width) && pixel.y < int(height))
            {
                sum += extract(emissionFrame.Load(int3(pixel, 0)));
                count += 1.0f;
            }
        }
    }
    return sum / max(count, 1.0f);
}
