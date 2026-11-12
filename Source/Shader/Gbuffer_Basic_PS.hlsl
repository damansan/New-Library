Texture2D baseMap : register(t0);
Texture2D normalMap : register(t1);

SamplerState samplerState : register(s0);

cbuffer CbObject : register(b2)
{
    row_major float4x4 world;
    float alphaCutoff;
    float3 padding;
    float4 baseColorFactor; // マテリアルの基本色（リニア空間）
};

struct VS_OUT
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 texcoord : TEXCOORD;
    float3 worldPos : WORLD_POSITION;
};

struct PS_OUT
{
    float4 albedo : SV_TARGET0;
    float4 normal : SV_TARGET1;
    float4 position : SV_TARGET2;
    float4 roughnessMetallic : SV_TARGET3;
    float4 emissive : SV_TARGET4;
    float occlusion : SV_TARGET5;
};

PS_OUT main(VS_OUT pin)
{
    PS_OUT pout;

    // Albedo（ガンマ補正でリニア空間へ）
    float4 baseColor = baseMap.Sample(samplerState, pin.texcoord);

    // アルファは係数を掛けてからクリップ判定する
    baseColor.a *= baseColorFactor.a;

    // Maskクリップ
    clip(baseColor.a - alphaCutoff);

    // テクスチャはsRGBなのでリニアに戻してから、リニアの係数を掛ける
    baseColor.rgb = pow(max(baseColor.rgb, 0.0001f), 2.2f);
    baseColor.rgb *= baseColorFactor.rgb;
    pout.albedo = baseColor;
    
    // 法線
    float3 N = normalize(pin.normal);
    pout.normal = float4(N * 0.5f + 0.5f, 1.0f);

    // ワールド座標
    pout.position = float4(pin.worldPos, 1.0f);

    // Roughness/Metallic
    pout.roughnessMetallic = float4(1.0f, 0.0f, 0.0f, 1.0f);

    // Emissive
    pout.emissive = float4(0.0f, 0.0f, 0.0f, 1.0f);

    // Occlusion
    pout.occlusion = 1.0f;

    return pout;
}