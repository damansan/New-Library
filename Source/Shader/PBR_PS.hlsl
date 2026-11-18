#include "LightingCommon.hlsli"

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
    emissive *= emissiveFactor; // glTF：発光 = 係数 × テクスチャ
    
    // MetallicRoughness取得（glTF：値 = 係数 × テクスチャ）
    float4 mr = metallicRoughnessMap.Sample(samplerState, pin.texcoord);
    float roughness = max(mr.g * roughnessFactor, 0.01f);
    float metalness = mr.b * metallicFactor;
    
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

    // 材料を詰める
    SurfaceData s;
    s.albedo = baseColor.rgb;
    s.normal = N;
    s.worldPos = pin.worldPos;
    s.roughness = roughness;
    s.metalness = metalness;
    s.occlusion = occlusion;
    s.emissive = emissive;

    // 光を当てる
    float3 color = ShadeSurface(s, diffuseIem, specularPmrem, lutGgx, samplerState);

    // ガンマ補正
    color = pow(max(color, 0.0001f), 1.0f / GAMMA);

    return float4(color, baseColor.a);
}