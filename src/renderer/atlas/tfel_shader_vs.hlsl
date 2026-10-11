// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD;
};

VS_OUTPUT main(uint id : SV_VERTEXID)
{
    VS_OUTPUT output;
    output.tex = float2(id % 2, id % 4 / 2);
    output.pos = float4((output.tex.x - 0.5f) * 2.0f, -(output.tex.y - 0.5f) * 2.0f, 0, 1);
    return output;
}
