cbuffer CbScene : register(b0)
{
    row_major float4x4 view;
    row_major float4x4 projection;
    float4 lightDirection;
    float4 lightColor;
    float4 ambientColor;
    float4 cameraPosition;
};

Texture2D baseMap : register(t0);
SamplerState samplerState : register(s0);

struct VS_OUT
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
    float3 worldPos : WORLD_POSITION;
};

float4 main(VS_OUT pin) : SV_TARGET
{
    // ベースカラー取得
    float4 baseColor = baseMap.Sample(samplerState, pin.texcoord);

    // 法線正規化
    float3 N = normalize(pin.normal);

    // ライト方向正規化（逆方向）
    float3 L = normalize(-lightDirection.xyz);

    // ランバート拡散光
    float diffuse = max(dot(N, L), 0.0f);

    // 最終カラー計算
    float3 color =
        baseColor.rgb * lightColor.rgb * diffuse +
        baseColor.rgb * ambientColor.rgb;

    return float4(color, baseColor.a);
}