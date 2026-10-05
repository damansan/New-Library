Texture2D baseMap : register(t0);
SamplerState samplerState : register(s0);

struct VS_OUT
{
    float4 position : SV_POSITION;
    float3 normal   : NORMAL;
    float4 tangent  : TANGENT;
    float2 texcoord : TEXCOORD;
    float3 worldPos : WORLD_POSITION;
};

float4 main(VS_OUT pin) : SV_TARGET
{
    // ベースカラーのみ返す
    return baseMap.Sample(samplerState, pin.texcoord);
}