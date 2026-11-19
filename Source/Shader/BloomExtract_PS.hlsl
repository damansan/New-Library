#include "ShaderConstants.hlsli"

// ブルーム：明るい所を書き出す(ついでに 1/2 の大きさへ縮小する)

Texture2D sceneTexture : register(t0);
SamplerState linearSampler : register(s0);

struct VS_OUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

// ブルームに入れる明るさの上限
static const float BLOOM_MAX_BRIGHTNESS = 100.0f;

float4 main(VS_OUT pin) : SV_TARGET
{
    // 1/2 の1ピクセル = 元の 2×2 ピクセル。その真ん中をバイリニアで読むと、4つの平均になる
    float3 color = sceneTexture.Sample(linearSampler, pin.texcoord).rgb;
    
    // 画面に出るときと同じ明るさで判定するため、露出を先にかける
    color *= exposure;
 
    // NaN/Inf はブルームに持ち込まない
    if (any((asuint(color) & 0x7F800000) == 0x7F800000))
    {
        color = 0.0f;
    }
    
    // 明るさに上限を付ける
    float brightness = max(color.r, max(color.g, color.b));
    float limitScale = min(1.0f, BLOOM_MAX_BRIGHTNESS / max(brightness, 0.0001f));
    color *= limitScale;
    brightness *= limitScale;
    
    // いちばん明るいチャンネルが、しきい値をどれだけ超えたかの割合を、色全体に掛ける
    float contribution = max(brightness - bloomThreshold, 0.0f) / max(brightness, 0.0001f);

    return float4(color * contribution, 1.0f);
}