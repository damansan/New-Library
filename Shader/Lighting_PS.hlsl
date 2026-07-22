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
    float4 direction; // xyz=方向 w=未使用
    float4 color; // xyz=色 w=強度
};
struct PointLight
{
    float4 position; // xyz=位置 w=範囲
    float4 color; // xyz=色 w=強度
};

#define MAX_POINT_LIGHTS 64

cbuffer CbLights : register(b1)
{
    DirectionalLight directionalLight;
    PointLight pointLights[MAX_POINT_LIGHTS];
    int pointLightCount;
    float3 cbPadding;
};

// カスケードシャドウマップ
#define CASCADE_COUNT 4

cbuffer CbCascadeShadow : register(b2)
{
    row_major float4x4 cascadeLightViewProjection[CASCADE_COUNT];
    float4 shadowBias; // カスケードごとの深度比較オフセット
    float shadowAttenuation; // 影の暗さ
    float3 shadowPadding;
};

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

// カスケードシャドウマップ
Texture2DArray cascadeShadowMaps : register(t9);

SamplerState samplerState : register(s0);

struct VS_OUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

// カスケードシャドウ判定
float CalcCascadeShadow(float3 worldPos)
{
    for (int i = 0; i < CASCADE_COUNT; i++)
    {
        // 光源から見たNDC座標を算出
        float4 lightClipPos = mul(float4(worldPos, 1.0f), cascadeLightViewProjection[i]);
        lightClipPos /= lightClipPos.w;

        // NDC座標からUV座標へ変換
        float2 shadowUV = lightClipPos.xy * 0.5f + 0.5f;
        shadowUV.y = 1.0f - shadowUV.y;

        // このカスケードの描画範囲内かどうか判定
        bool inside =
            lightClipPos.z >= 0.0f && lightClipPos.z <= 1.0f &&
            shadowUV.x >= 0.0f && shadowUV.x <= 1.0f &&
            shadowUV.y >= 0.0f && shadowUV.y <= 1.0f;

        if (!inside)
            continue;

        // シャドウマップから深度値取得(第3引数でカスケード番号のスライスを選択)
        float storedDepth = cascadeShadowMaps.Sample(samplerState, float3(shadowUV, i)).r;
        
        // 深度値を比較して影かどうか判定
        if (lightClipPos.z - storedDepth > shadowBias[i])
        {
            return shadowAttenuation;
        }
        // 範囲内が見つかった時点でそのカスケードの結果を使う
        return 1.0f;
    }
    // どのカスケードにも属さない場合は影なし扱い
    return 1.0f;
}

float4 main(VS_OUT pin) : SV_TARGET
{
    float2 uv = pin.texcoord;

    // Gバッファから情報を読み出す
    float4 albedo = gbAlbedo.Sample(samplerState, uv);
    float3 N = gbNormal.Sample(samplerState, uv).xyz;
    float3 worldPos = gbPosition.Sample(samplerState, uv).xyz;
    float4 roughnessMetallic = gbRoughnessMetallic.Sample(samplerState, uv);
    float3 emissive = gbEmissive.Sample(samplerState, uv).rgb;
    float  occlusion = gbOcclusion.Sample(samplerState, uv).r;

    // 法線デコード
    N = normalize(N * 2.0f - 1.0f);

    float roughness = roughnessMetallic.r;
    float metalness = roughnessMetallic.g;

    // PBRパラメーター計算
    float3 diffuse_reflectance = lerp(albedo.rgb, 0.0f, metalness);
    float3 F0 = lerp(0.04f, albedo.rgb, metalness);

    // ベクトル計算
    float3 V = normalize(cameraPosition.xyz - worldPos);
    
    // 直接光
    float3 diffuse = 0;
    float3 specular = 0;
    
    // 平行光源
    {
        float3 L = normalize(-directionalLight.direction.xyz);
        float3 LC = directionalLight.color.rgb * directionalLight.color.a;
        float3 d = 0;
        float3 s = 0;
        DirectBRDF(diffuse_reflectance, F0, N, V, L, LC, roughness, d, s);
        
        // カスケードシャドウマップによる減衰
        float shadowFactor = CalcCascadeShadow(worldPos);
        d *= shadowFactor;
        s *= shadowFactor;
        
        diffuse += d;
        specular += s;
    }

    // ポイントライト
    for (int i = 0; i < pointLightCount; i++)
    {
        float3 toLight = pointLights[i].position.xyz - worldPos;
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
    
    //float3 L = normalize(-lightDirection.xyz);
    //float3 LC = lightColor.rgb * lightColor.a;

    //// 直接光のPBRライティング
    //float3 diffuse = 0;
    //float3 specular = 0;
    //DirectBRDF(diffuse_reflectance, F0, N, V, L, LC,
    //           roughness, diffuse, specular);

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