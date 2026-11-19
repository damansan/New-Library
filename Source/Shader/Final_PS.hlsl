
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

     // 出口：リニア → モニター向けの値（ガンマ補正はここ1か所だけ）
    color = pow(max(color, 0.0001f), 1.0f / 2.2f);
    
    return float4(color, 1.0f);
}