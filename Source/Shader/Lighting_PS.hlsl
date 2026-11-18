#include "LightingCommon.hlsli"

// Gバッファ
Texture2D gbAlbedo            : register(t0);
Texture2D gbNormal            : register(t1);
Texture2D gbPosition          : register(t2);
Texture2D gbRoughnessMetallic : register(t3);
Texture2D gbEmissive          : register(t4);
Texture2D gbOcclusion         : register(t5);

// IBL用テクスチャ
TextureCube diffuseIem    : register(t6);
TextureCube specularPmrem : register(t7);
Texture2D lutGgx          : register(t8);

SamplerState samplerState : register(s0);

struct VS_OUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float4 main(VS_OUT pin) : SV_TARGET
{    
    float2 uv = pin.texcoord;

    // Gバッファから材料を読み出す
    float4 albedo = gbAlbedo.Sample(samplerState, uv);
    float4 roughnessMetallic = gbRoughnessMetallic.Sample(samplerState, uv);

    SurfaceData s;
    s.albedo = albedo.rgb;
    s.normal = normalize(gbNormal.Sample(samplerState, uv).xyz * 2.0f - 1.0f);
    s.worldPos = gbPosition.Sample(samplerState, uv).xyz;
    s.roughness = roughnessMetallic.r;
    s.metalness = roughnessMetallic.g;
    s.occlusion = gbOcclusion.Sample(samplerState, uv).r;
    s.emissive = gbEmissive.Sample(samplerState, uv).rgb;

    // 光を当てる
    float3 color = ShadeSurface(s, diffuseIem, specularPmrem, lutGgx, samplerState);

    // ガンマ補正
    color = pow(max(color, 0.0001f), 1.0f / 2.2f);

    return float4(color, albedo.a);
}