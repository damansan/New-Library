// ブルーム：1つ下の段を2倍に広げて、この段に足す

Texture2D    currentTexture : register(t0); // この段のぼかした絵
Texture2D    lowerTexture   : register(t1); // 1つ下の段までを足した絵
SamplerState linearSampler  : register(s0);

struct VS_OUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float4 main(VS_OUT pin) : SV_TARGET
{
    float3 current = currentTexture.Sample(linearSampler, pin.texcoord).rgb;

    // 同じ uv で半分の大きさの絵を読むと、バイリニアで2倍に広げた値になる
    float3 lower = lowerTexture.Sample(linearSampler, pin.texcoord).rgb;

    return float4(current + lower, 1.0f);
}