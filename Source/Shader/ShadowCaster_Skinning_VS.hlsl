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

float4 main(VS_IN vin) : SV_POSITION
{
    // スキニング
    float4 pos = float4(vin.position, 1.0f);
    float4 skinnedPos = float4(0, 0, 0, 0);

    for (int i = 0; i < 4; i++)
    {
        skinnedPos += vin.boneWeight[i] * mul(pos, boneTransforms[vin.boneIndex[i]]);
    }

    float4 worldPos = mul(skinnedPos, world);
    return mul(worldPos, lightViewProjection);
}