#include "BloomBlur.hlsli"

Texture2D sourceTexture : register(t0);
SamplerState linearSampler : register(s0);

struct VS_OUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

// ƒuƒ‹[ƒ€Fc‚É‚Ú‚©‚·
float4 main(VS_OUT pin) : SV_TARGET
{
    return float4(GaussianBlur9(sourceTexture, linearSampler, pin.texcoord, float2(0.0f, 1.0f)), 1.0f);
}