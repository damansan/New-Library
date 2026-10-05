TextureCube skyMap : register(t10); // 他のテクスチャと被らないスロット
SamplerState samplerState : register(s0);

struct VS_OUT
{
    float4 position : SV_POSITION;
    float3 texcoord : TEXCOORD;
};

float4 main(VS_OUT pin) : SV_TARGET
{
    float3 color = skyMap.Sample(samplerState, pin.texcoord).rgb;

    // ガンマ補正
    color = pow(max(color, 0.0001f), 1.0f / 2.2f);

    return float4(color, 1.0f);
}