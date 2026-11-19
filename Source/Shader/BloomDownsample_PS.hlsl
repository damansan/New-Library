
// ブルーム：1段小さくする（縦横 1/2）

Texture2D sourceTexture : register(t0);
SamplerState linearSampler : register(s0);

struct VS_OUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float4 main(VS_OUT pin) : SV_TARGET
{
    // 小さい方の1ピクセル = 大きい方の 2×2。真ん中をバイリニアで読むと、4つの平均になる
    return float4(sourceTexture.Sample(linearSampler, pin.texcoord).rgb, 1.0f);
}