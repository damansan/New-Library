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

// テクスチャ
Texture2D baseMap              : register(t0); // Albedo
Texture2D normalMap            : register(t1); // 法線マップ
Texture2D metallicRoughnessMap : register(t2); // G:Roughness B:Metallic
Texture2D occlusionMap         : register(t3); // R:Occlusion
Texture2D emissiveMap          : register(t4); // Emissive
// IBL用テクスチャ
TextureCube diffuseIem    : register(t5); // Diffuse IBL
TextureCube specularPmrem : register(t6); // Specular IBL
Texture2D lutGgx          : register(t7); // BRDF LUT

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
    // Albedo取得 + ガンマ補正(sRGB -> Linear)
    float4 baseColor = baseMap.Sample(samplerState, pin.texcoord);
    baseColor.rgb = pow(max(baseColor.rgb, 0.0001f), GAMMA);
    
    // Emissive取得 + ガンマ補正(sRGB -> Linear)
    float3 emissive = emissiveMap.Sample(samplerState, pin.texcoord).rgb;
    emissive = pow(max(emissive, 0.0001f), GAMMA);
    
    // MetallicRoughness取得
    // glTF準拠: G=Roughness, B=Metallic
    float4 mr = metallicRoughnessMap.Sample(samplerState, pin.texcoord);
    float roughness = max(mr.g, 0.01f); // 最小値を設ける
    float metalness = mr.b;

    // Occlusion取得
    float occlusion = occlusionMap.Sample(samplerState, pin.texcoord).r;
    
    // 法線マッピング
    float3 N = normalize(pin.normal);
    float3 T = normalize(pin.tangent.xyz);
    float sigma = pin.tangent.w; // 接線の符号

    // グラムシュミット直交化
    T = normalize(T - N * dot(N, T));
    float3 B = normalize(cross(N, T) * sigma); // 従法線
    
    // 法線マップをサンプリングして接線空間 -> ワールド空間へ
    float3 normalSample = normalMap.Sample(samplerState, pin.texcoord).xyz;
    normalSample = normalSample * 2.0f - 1.0f; // [0,1] -> [-1,1]
    N = normalize(normalSample.x * T
                + normalSample.y * B
                + normalSample.z * N);
    
    // PBRパラメーター計算
    // 拡散反射率（金属は拡散反射しない）
    float3 diffuse_reflectance = lerp(baseColor.rgb, 0.0f, metalness);

    // 垂直入射時のフレネル反射率
    // 非金属でも最低4%は鏡面反射する
    float3 F0 = lerp(0.04f, baseColor.rgb, metalness);

    // ベクトル計算
    float3 V = normalize(cameraPosition.xyz - pin.worldPos); // 視線方向
    float3 L = normalize(-lightDirection.xyz); // 光源方向
    float3 LC = lightColor.rgb * lightColor.a; // ライトカラー
    
    // 直接光のPBRライティング
    float3 diffuse = 0;
    float3 specular = 0;
    DirectBRDF(diffuse_reflectance, F0, N, V, L, LC, roughness, diffuse, specular);
    
    
    // 間接光（アンビエント）
    float3 ambient = 0;
    float3 iblDiffuse = IBL_Diffuse(
    diffuseIem, samplerState, N,
    diffuse_reflectance, F0, V);

    float3 iblSpecular = IBL_Specular(
    specularPmrem, lutGgx, samplerState,
    N, V, F0, roughness);

    ambient = iblDiffuse + iblSpecular;
    
    
    // Occlusion適用
    diffuse *= occlusion;
    specular *= occlusion;
    ambient *= occlusion;

    // 最終カラー合成
    float3 color = diffuse + specular + ambient + emissive;
    
    // ガンマ補正(Linear -> sRGB)
    color = pow(max(color, 0.0001f), 1.0f / GAMMA);

    return float4(color, baseColor.a);
    
}