// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
Texture2D<float4> inputFrame : register(t0);
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
    static const float weights[5] = { 0.2270270270f, 0.1945945946f, 0.1216216216f, 0.0540540541f, 0.0162162162f };
    float2 step = bloomSampling.xy * bloomSampling.zw * clamp(bloom.z, 0, 16) / 8;
    float3 result = inputFrame.Sample(inputSampler, tex).rgb * weights[0];
    [unroll] for (int i = 1; i <= 4; ++i)
    {
        result += weights[i] * inputFrame.Sample(inputSampler, tex + step * i).rgb;
        result += weights[i] * inputFrame.Sample(inputSampler, tex - step * i).rgb;
    }
    return float4(result, 0);
}
