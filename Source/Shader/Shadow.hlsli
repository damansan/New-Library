// 影を読む側の共通処理

#ifndef SHADOW_HLSLI
#define SHADOW_HLSLI

#include "ShaderConstants.hlsli"

Texture2DArray shadowMap : register(t9); // 影マップ（段ごとの配列）
SamplerComparisonState shadowSampler : register(s1); // 比較サンプラー

static const float SHADOW_NORMAL_OFFSET = 1.5f; // 法線方向へ浮かせる量
static const float SHADOW_DEPTH_BIAS = 0.0005f; // 深度を手前に寄せる量
static const float SHADOW_FADE_RATIO = 0.1f;    // 最後の段の奥
static const float CASCADE_BLEND_RATIO = 0.2f;  // 各段の奥、何割の区間で次の段と混ぜるか
static const int SHADOW_PCF_RADIUS = 1;         // 周りを何ピクセルまで読むか

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
    
    // 少し手前に寄せた、自分の深度
    float compareDepth = depth - SHADOW_DEPTH_BIAS;

    // 影マップ1ピクセルぶんの UV の大きさ
    float2 texelUV = 1.0f / float2(width, height);

    // 周りのピクセルも読んで平均する
    float sum = 0.0f;
    [unroll]
    for (int y = -SHADOW_PCF_RADIUS; y <= SHADOW_PCF_RADIUS; y++)
    {
        [unroll]
        for (int x = -SHADOW_PCF_RADIUS; x <= SHADOW_PCF_RADIUS; x++)
        {
            float2 offsetUV = uv + float2(x, y) * texelUV;
            sum += shadowMap.SampleCmpLevelZero(shadowSampler, float3(offsetUV, (float) cascade), compareDepth);
        }
    }

    const float sampleCount = (SHADOW_PCF_RADIUS * 2 + 1) * (SHADOW_PCF_RADIUS * 2 + 1);
    return sum / sampleCount;
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

    // 段の境目
    if (cascade < SHADOW_CASCADE_COUNT - 1)
    {
        float cascadeStart = (cascade == 0) ? 0.0f : cascadeSplits[cascade - 1];
        float cascadeEnd = cascadeSplits[cascade];
        float blendStart = cascadeEnd - (cascadeEnd - cascadeStart) * CASCADE_BLEND_RATIO;
        float blend = saturate((viewDepth - blendStart) / (cascadeEnd - blendStart));

        // 混ぜる区間に入った所だけ、次の段も読む
        if (blend > 0.0f)
        {
            float nextShadow = SampleShadowCascade(cascade + 1, worldPos, normal);
            shadow = lerp(shadow, nextShadow, blend);
        }
    }
    
    // いちばん奥の手前で、影を少しずつ消す
    float fadeStart = shadowEnd * (1.0f - SHADOW_FADE_RATIO);
    float fade = saturate((viewDepth - fadeStart) / (shadowEnd - fadeStart));
    return lerp(shadow, 1.0f, fade);
}

#endif