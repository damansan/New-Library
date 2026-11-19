TextureCube skyMap : register(t10); // 他のテクスチャと被らないスロット
SamplerState samplerState : register(s0);

struct VS_OUT
{
    float4 position : SV_POSITION;
    float3 texcoord : TEXCOORD;
};

float4 main(VS_OUT pin) : SV_TARGET
{
    // 空のテクスチャ（HDR）はリニアの値なので、そのまま出す。ガンマ補正は最終パスで行う
    float3 color = skyMap.Sample(samplerState, pin.texcoord).rgb;

    return float4(color, 1.0f);
}