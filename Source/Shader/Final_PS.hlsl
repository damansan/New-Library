
Texture2D sceneTexture : register(t0);

struct VS_OUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float4 main(VS_OUT pin) : SV_TARGET
{
    // 読む側と書く側が同じ大きさなので、ピクセルの位置で直接読む
    float3 color = sceneTexture.Load(int3(pin.position.xy, 0)).rgb;

    return float4(color, 1.0f);
}