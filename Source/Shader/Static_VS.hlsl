#include "ShaderConstants.hlsli"

// í∏ì_ì¸óÕ
struct VS_IN
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 texcoord : TEXCOORD;
    float4 boneWeight : WEIGHTS;
    uint4 boneIndex : BONES;
};

// í∏ì_èoóÕ
struct VS_OUT
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 texcoord : TEXCOORD;
    float3 worldPos : WORLD_POSITION;
};

VS_OUT main(VS_IN vin)
{
    VS_OUT vout;

    float4 pos = float4(vin.position, 1.0f);
    pos = mul(pos, world);
    vout.worldPos = pos.xyz;
    pos = mul(pos, view);
    pos = mul(pos, projection);
    vout.position = pos;

    vout.normal = normalize(mul(vin.normal, (float3x3) worldInverseTranspose));
    vout.tangent.xyz = normalize(mul(vin.tangent.xyz, (float3x3) world));
    vout.tangent.w = vin.tangent.w;
    vout.texcoord = vin.texcoord;

    return vout;
}