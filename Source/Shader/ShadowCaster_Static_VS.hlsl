#include "ShaderConstants.hlsli"

// 頂点入力
struct VS_IN
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 texcoord : TEXCOORD;
    float4 boneWeight : WEIGHTS;
    uint4 boneIndex : BONES;
};

// 影マップには深度だけ書くので、出力は位置のみ
float4 main(VS_IN vin) : SV_POSITION
{
    float4 worldPos = mul(float4(vin.position, 1.0f), world);
    return mul(worldPos, lightViewProjection);
}