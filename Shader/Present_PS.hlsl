Texture2D sourceTexture : register(t0);
SamplerState samplerState : register(s0);

struct VS_OUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float4 main(VS_OUT pin) : SV_TARGET
{
    return sourceTexture.Sample(samplerState, pin.texcoord);
}