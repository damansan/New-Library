// 影を読む側の共通処理

#ifndef SHADOW_HLSLI
#define SHADOW_HLSLI

#include "ShaderConstants.hlsli"

Texture2DArray shadowMap : register(t9); // 影マップ（段ごとの配列）
SamplerComparisonState shadowSampler : register(s1); // 比較サンプラー

static const float SHADOW_NORMAL_OFFSET = 1.5f; // 法線方向へ浮かせる量
static const float SHADOW_DEPTH_BIAS = 0.0005f; // 深度を手前に寄せる量
static const float SHADOW_FADE_RATIO = 0.1f;    // 最後の段の奥

// 指定した段の影マップを読む。光が届いていれば 1、影なら 0
float SampleShadowCascade(int cascade, float3 worldPos, float3 normal)
{
    float4x4 m = shadowViewProjections[cascade];
    
    // 影マップ1ピクセルがワールドで何mか
    uint width, height, elements;
    shadowMap.GetDimensions(width, height, elements);
    float texelWorld = 2.0f / (length(float3(m._11, m._21, m._31)) * width);
    
    // 表面を法線の方向へ少し浮かせてから調べる
    float3 offsetPos = worldPos + normal * texelWorld * SHADOW_NORMAL_OFFSET;
    
    // ライトから見た位置へ
    float4 lightPos = mul(float4(offsetPos, 1.0f), m);
    float3 ndc = lightPos.xyz / lightPos.w;
    
    // NDC から テクスチャ座標 へ変換
    float2 uv = float2(ndc.x * 0.5f + 0.5f, -ndc.y * 0.5f + 0.5f);
    float depth = ndc.z;
    
    // ライトの範囲より奥は、影マップに情報がないので影なしにする
    if (depth > 1.0f)
    {
        return 1.0f;
    }
    
    // 「自分の深度 <= 影マップの深度」なら 光が届く
    return shadowMap.SampleCmpLevelZero(shadowSampler, float3(uv, (float) cascade), depth - SHADOW_DEPTH_BIAS);
}

// 光が届いていれば 1、影なら 0 を返す
float CalcShadow(float3 worldPos, float3 normal)
{
    // カメラからの距離
    float viewDepth = mul(float4(worldPos, 1.0f), view).z;
    
    // 影を出す範囲より奥は、影なし
    float shadowEnd = cascadeSplits[SHADOW_CASCADE_COUNT - 1];
    if (viewDepth >= shadowEnd)
    {
        return 1.0f;
    }
    
    // どの段を読むか
    int cascade = 0;
    [unroll]
    for (int i = 0; i < SHADOW_CASCADE_COUNT - 1; i++)
    {
        if (viewDepth > cascadeSplits[i])
        {
            cascade = i + 1;
        }
    }
    
    float shadow = SampleShadowCascade(cascade, worldPos, normal);

     // いちばん奥の手前で、影を少しずつ消す
    float fadeStart = shadowEnd * (1.0f - SHADOW_FADE_RATIO);
    float fade = saturate((viewDepth - fadeStart) / (shadowEnd - fadeStart));
    return lerp(shadow, 1.0f, fade);
}

#endif