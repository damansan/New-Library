// ライティングの共通処理

#ifndef LIGHTING_COMMON_HLSLI
#define LIGHTING_COMMON_HLSLI

#include "ShaderConstants.hlsli"
#include "PBR_Functions.hlsli"
#include "IBL_Functions.hlsli"

// 表面の材料
struct SurfaceData
{
    float3 albedo; // 基本色
    float3 normal; // 法線
    float3 worldPos; // ワールド座標
    float roughness; // 粗さ
    float metalness; // 金属度
    float occlusion; // AO
    float3 emissive; // 発光
};

// 光を当てた色を返す
float3 ShadeSurface(
    SurfaceData s,
    TextureCube diffuseIem,
    TextureCube specularPmrem,
    Texture2D lutGgx,
    SamplerState samplerState)
{
    // PBR パラメーター
    float3 diffuse_reflectance = lerp(s.albedo, 0.0f, s.metalness);
    float3 F0 = lerp(0.04f, s.albedo, s.metalness);

    float3 N = s.normal;
    float3 V = normalize(cameraPosition.xyz - s.worldPos);
    float3 diffuse = 0;
    float3 specular = 0;

    // 平行光源
    {
        float3 L = normalize(-directionalLight.direction.xyz);
        float3 LC = directionalLight.color.rgb * directionalLight.color.a;
        float3 d = 0;
        float3 sp = 0;
        DirectBRDF(diffuse_reflectance, F0, N, V, L, LC, s.roughness, d, sp);
        diffuse += d;
        specular += sp;
    }

    // ポイントライト
    for (int i = 0; i < pointLightCount; i++)
    {
        float3 toLight = pointLights[i].position.xyz - s.worldPos;
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
        float3 sp = 0;
        DirectBRDF(diffuse_reflectance, F0, N, V, L, LC, s.roughness, d, sp);
        diffuse += d;
        specular += sp;
    }

    // 間接光（IBL）
    float3 iblDiffuse = IBL_Diffuse(
        diffuseIem, samplerState, N,
        diffuse_reflectance, F0, V);

    float3 iblSpecular = IBL_Specular(
        specularPmrem, lutGgx, samplerState,
        N, V, F0, s.roughness);

    float3 ambient = (iblDiffuse + iblSpecular) * iblIntensity;
    
    // Occlusion
    ambient *= s.occlusion;
    
    return diffuse + specular + ambient + s.emissive;
}

#endif