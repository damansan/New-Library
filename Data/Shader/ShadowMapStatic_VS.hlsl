cbuffer CbShadowScene : register(b0)
{
    row_major float4x4 lightViewProjection;
};

cbuffer CbObject : register(b1)
{
    row_major float4x4 world;
};

struct VS_IN
{
    float3 position : POSITION;
};

float4 main(VS_IN vin) : SV_POSITION
{
    float4 worldPos = mul(float4(vin.position, 1.0f), world);
    return mul(worldPos, lightViewProjection);
}