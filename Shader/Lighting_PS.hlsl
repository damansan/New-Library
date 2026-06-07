#include "PBR_Functions.hlsli"
#include "IBL_Functions.hlsli"

cbuffer CbScene : register(b0)
{
    row_major float4x4 view;
    row_major float4x4 projection;
    float4 lightDirection;
    float4 lightColor;
    float4 ambientColor;
    float4 cameraPosition;
};

// Gバッファ
Texture2D gbAlbedo            : register(t0);
Texture2D gbNormal            : register(t1);
Texture2D gbPosition          : register(t2);
Texture2D gbRoughnessMetallic : register(t3);
Texture2D gbEmissive          : register(t4);

// IBL用テクスチャ
TextureCube diffuseIem    : register(t5);
TextureCube specularPmrem : register(t6);
Texture2D lutGgx          : register(t7);

// Occlusion
Texture2D gbOcclusion : register(t8);

SamplerState samplerState : register(s0);

struct VS_OUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float4 main(VS_OUT pin) : SV_TARGET
{
    float2 uv = pin.texcoord;

    // Gバッファから情報を読み出す
    float4 albedo = gbAlbedo.Sample(samplerState, uv);
    float3 N = gbNormal.Sample(samplerState, uv).xyz;
    float3 worldPos = gbPosition.Sample(samplerState, uv).xyz;
    float4 roughnessMetallic = gbRoughnessMetallic.Sample(samplerState, uv);
    float3 emissive = gbEmissive.Sample(samplerState, uv).rgb;
    //float occlusion = gbOcclusion.Sample(samplerState, uv).r;
    float occlusion = 1.0f;

    // 法線デコード
    N = normalize(N * 2.0f - 1.0f);

    float roughness = roughnessMetallic.r;
    float metalness = roughnessMetallic.g;

    // PBRパラメーター計算
    float3 diffuse_reflectance = lerp(albedo.rgb, 0.0f, metalness);
    float3 F0 = lerp(0.04f, albedo.rgb, metalness);

    // ベクトル計算
    float3 V = normalize(cameraPosition.xyz - worldPos);
    float3 L = normalize(-lightDirection.xyz);
    float3 LC = lightColor.rgb * lightColor.a;

    // 直接光のPBRライティング
    float3 diffuse = 0;
    float3 specular = 0;
    DirectBRDF(diffuse_reflectance, F0, N, V, L, LC,
               roughness, diffuse, specular);

    // 間接光（IBL）
    float3 iblDiffuse = IBL_Diffuse(
        diffuseIem, samplerState, N,
        diffuse_reflectance, F0, V);

    float3 iblSpecular = IBL_Specular(
        specularPmrem, lutGgx, samplerState,
        N, V, F0, roughness);

    float3 ambient = iblDiffuse + iblSpecular;

    // Occlusion適用
    diffuse *= occlusion;
    specular *= occlusion;
    ambient *= occlusion;

    // 最終カラー合成
    float3 color = diffuse + specular + ambient + emissive;

    // ガンマ補正
    color = pow(max(color, 0.0001f), 1.0f / 2.2f);

    return float4(color, albedo.a);
}