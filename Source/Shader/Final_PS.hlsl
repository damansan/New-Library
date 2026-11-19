#include "ShaderConstants.hlsli"

Texture2D sceneTexture : register(t0);

struct VS_OUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

// カラー調整
float3 ColorAdjust(float3 color)
{
    // コントラスト
    const float middleGray = 0.18f;
    color = middleGray * pow(max(color, 0.0f) / middleGray, contrast);

    // カラーフィルタ
    color *= colorFilter;

    // 彩度
    float luma = dot(color, float3(0.2126f, 0.7152f, 0.0722f));
    color = lerp(luma.xxx, color, saturation);

    return max(color, 0.0f);
}

// ビネット
float3 ApplyVignette(float3 color, float2 uv)
{
    // 中心からの距離に強さを掛ける
    float2 d = abs(uv - 0.5f) * vignetteIntensity;

    // 縦横比で補正して、横長の画面でも円にする
    d.x *= aspectRatio;

    // 中心ほど 1、隅ほど 0 になる
    float factor = pow(saturate(1.0f - dot(d, d)), vignetteSmoothness);

    // 隅ほどビネットの色に近づける
    return color * lerp(vignetteColor, 1.0f, factor);
}

// ACES トーンマップ
float3 TonemapACES(float3 x)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

// リニア から sRGB
float3 LinearToSRGB(float3 c)
{
    c = saturate(c);
    float3 low = c * 12.92f;
    float3 high = 1.055f * pow(c, 1.0f / 2.4f) - 0.055f;
    return (c <= 0.0031308f) ? low : high; // 成分ごとに選ぶ
}

float4 main(VS_OUT pin) : SV_TARGET
{
    // 読む側と書く側が同じ大きさなので、ピクセルの位置で直接読む
    float3 color = sceneTexture.Load(int3(pin.position.xy, 0)).rgb;

    // 露出設定
    color *= exposure;
    
    // カラー調整
    color = ColorAdjust(color);
    
    // ビネット
    color = ApplyVignette(color, pin.texcoord);

    // トーンマップ
    if (tonemapEnabled != 0)
    {
        color = TonemapACES(color);
    }
    
     // 出口：リニア → モニター向けの値（ガンマ補正はここ1か所だけ）
    color = pow(max(color, 0.0001f), 1.0f / 2.2f);
    
    return float4(color, 1.0f);
}