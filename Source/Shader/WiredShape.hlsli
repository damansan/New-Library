struct VS_OUT
{
    float4 position : SV_POSITION;
    float4 color    : COLOR;
};

cbuffer CbFrame : register(b0)
{
    row_major float4x4 viewProjection;
};