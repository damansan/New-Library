#include "PBR_Functions.hlsli"
#include "IBL_Functions.hlsli" 

cbuffer CbScene : register(b0)
{
    row_major float4x4 view;
    row_major float4x4 projection;
    float4 cameraPosition;
};

struct DirectionalLight
{
    float4 direction;
    float4 color;
};
struct PointLight
{
    float4 position;
    float4 color;
};

#define MAX_POINT_LIGHTS 64

cbuffer CbLights : register(b1)
{
    DirectionalLight directionalLight;
    PointLight pointLights[MAX_POINT_LIGHTS];
    int pointLightCount;
    float3 cbPadding;
};

cbuffer CbObject : register(b2)
{
    row_major float4x4 world;
    float alphaCutoff;
    float3 padding;
    float4 baseColorFactor; // マテリアルの基本色（リニア空間）
};

// テクスチャ
Texture2D baseMap              : register(t0); // Albedo
Texture2D normalMap            : register(t1); // 法線マップ
Texture2D metallicRoughnessMap : register(t2); // G:Roughness B:Metallic
Texture2D occlusionMap         : register(t3); // R:Occlusion
Texture2D emissiveMap          : register(t4); // Emissive
// IBL用テクスチャ
TextureCube diffuseIem    : register(t5); // Diffuse IBL
TextureCube specularPmrem : register(t6); // Specular IBL
Texture2D   lutGgx        : register(t7); // BRDF LUT

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
    // Albedo取得
    float4 baseColor = baseMap.Sample(samplerState, pin.texcoord);
    baseColor.rgb = pow(max(baseColor.rgb, 0.0001f), GAMMA);

    // マテリアルの基本色を掛ける（リニア同士）
    baseColor *= baseColorFactor;
    
    // Emissive取得
    float3 emissive = emissiveMap.Sample(samplerState, pin.texcoord).rgb;
    emissive = pow(max(emissive, 0.0001f), GAMMA);
    
    // MetallicRoughness取得
    float4 mr = metallicRoughnessMap.Sample(samplerState, pin.texcoord);
    float roughness = max(mr.g, 0.01f);
    float metalness = mr.b;

    // Occlusion取得
    float occlusion = occlusionMap.Sample(samplerState, pin.texcoord).r;
    
    // 法線マッピング
    float3 N = normalize(pin.normal);
    float3 T = normalize(pin.tangent.xyz);
    float sigma = pin.tangent.w;

    T = normalize(T - N * dot(N, T));
    float3 B = normalize(cross(N, T) * sigma);
    
    float3 normalSample = normalMap.Sample(samplerState, pin.texcoord).xyz;
    normalSample = normalSample * 2.0f - 1.0f;
    N = normalize(normalSample.x * T
                + normalSample.y * B
                + normalSample.z * N);
    
    // PBRパラメーター計算
    float3 diffuse_reflectance = lerp(baseColor.rgb, 0.0f, metalness);
    float3 F0 = lerp(0.04f, baseColor.rgb, metalness);

    // ベクトル計算
    float3 V = normalize(cameraPosition.xyz - pin.worldPos);
    float3 diffuse = 0;
    float3 specular = 0;

    // 平行光源
    {
        float3 L = normalize(-directionalLight.direction.xyz);
        float3 LC = directionalLight.color.rgb * directionalLight.color.a;
        float3 d = 0;
        float3 s = 0;
        DirectBRDF(diffuse_reflectance, F0, N, V, L, LC, roughness, d, s);
        diffuse += d;
        specular += s;
    }

    // ポイントライト
    for (int i = 0; i < pointLightCount; i++)
    {
        float3 toLight = pointLights[i].position.xyz - pin.worldPos;
        float dist = length(toLight);
        float range = pointLights[i].position.w;

        if (dist > range)
            continue;

        float attenuation = 1.0f - saturate(dist / range);
        attenuation *= attenuation;

        float3 L = normalize(toLight);
        float3 LC = pointLights[i].color.rgb
              * pointLights[i].color.a
              * attenuation;

        float3 d = 0;
        float3 s = 0;
        DirectBRDF(diffuse_reflectance, F0, N, V, L, LC, roughness, d, s);
        diffuse += d;
        specular += s;
    }

    // 間接光
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
    color = pow(max(color, 0.0001f), 1.0f / GAMMA);

    return float4(color, baseColor.a);
}