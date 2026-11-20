#include "ShaderConstants.hlsli"

Texture2D sceneTexture : register(t0);
SamplerState linearSampler : register(s0);

struct VS_OUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

static const int RADIAL_BLUR_SAMPLES = 16; // 線の上で読む点の数
static const float2 RADIAL_BLUR_CENTER = float2(0.5f, 0.5f); // 中心

// ピクセルの位置から 0から1 のばらけた値を作る
float InterleavedGradientNoise(float2 pixel)
{
    return frac(52.9829189f * frac(dot(pixel, float2(0.06711056f, 0.00583715f))));
}

float4 main(VS_OUT pin) : SV_TARGET
{
    float2 uv = pin.texcoord;
    
    // 中心からの距離
    float2 d = (uv - RADIAL_BLUR_CENTER) * 2.0f;
    d.x *= aspectRatio;
    float dist = length(d);
    
    // 滑らかにする
    float mask = smoothstep(radialBlurRadius, radialBlurRadius + radialBlurSoftness, dist);

    // 線の全長
    float2 blurVector = (RADIAL_BLUR_CENTER - uv) * radialBlurStrength * mask;
    
    // 1点ずつ進む幅
    float2 stepUV = blurVector / RADIAL_BLUR_SAMPLES;
    
    // 読み始めを区間の中でずらす量
    float jitter = InterleavedGradientNoise(pin.position.xy);

    float3 color = 0.0f;
    [unroll]
    for (int i = 0; i < RADIAL_BLUR_SAMPLES; i++)
    {
        color += sceneTexture.SampleLevel(linearSampler, uv + stepUV * (i + jitter), 0).rgb;
    }
    color /= RADIAL_BLUR_SAMPLES;

    return float4(color, 1.0f);
}