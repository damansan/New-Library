struct VS_OUT
{
    float4 position : SV_POSITION;
    float3 normal   : NORMAL;
    float4 color    : COLOR;
};

cbuffer CbMesh : register(b0)
{
    row_major float4x4 viewProjection;
    float4 lightDirection;
};
