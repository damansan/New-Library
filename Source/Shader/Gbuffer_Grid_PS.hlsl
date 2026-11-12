SamplerState samplerState : register(s0);

cbuffer CbObject : register(b2)
{
    row_major float4x4 world;
    float alphaCutoff;
    float3 padding;
    float4 baseColorFactor; // 格子の下地の色（リニア空間）
};

// 格子の設定（ワールド単位）
static const float GRID_SIZE = 1.0f; // 線の間隔
static const float LINE_WIDTH = 0.02f; // 線の太さ
static const float LINE_DARKNESS = 0.66f; // 線の暗さ（下地の色に掛ける、UEの細線に合わせた値）
static const float3 GRID_BASE_COLOR = float3(0.18f, 0.18f, 0.18f); // 下地の色（リニア、UEのグレーに合わせた値）

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

    float3 N = normalize(pin.normal);

    // 面の向きから投影する2軸を選ぶ（一番大きい法線成分の軸を捨てる）
    float3 an = abs(N);
    float2 uv;
    if (an.x >= an.y && an.x >= an.z)
        uv = pin.worldPos.zy; // 左右を向いた面
    else if (an.y >= an.z)
        uv = pin.worldPos.xz; // 上下を向いた面
    else
        uv = pin.worldPos.xy; // 前後を向いた面

    // マス単位の座標
    float2 g = uv / GRID_SIZE;

    // 一番近い線までの距離（マス単位、0〜0.5）
    float2 d = abs(frac(g - 0.5f) - 0.5f);

    // 1ピクセルあたりの変化量（遠くで線がちらつかないようにぼかし幅に使う）
    float2 w = max(fwidth(g), 1e-5f);

    // 線の上なら1、線の外なら0（境界はwの幅でなめらかに）
    float halfWidth = (LINE_WIDTH / GRID_SIZE) * 0.5f;
    float2 lineMask = 1.0f - saturate((d - halfWidth) / w);
    float onLine = max(lineMask.x, lineMask.y);

    // 色（どちらもリニアなので掛け合わせる）
    float3 baseCol = GRID_BASE_COLOR * baseColorFactor.rgb;
    float3 lineCol = baseCol * LINE_DARKNESS;
    float4 albedo = float4(lerp(baseCol, lineCol, onLine), baseColorFactor.a);

    // Maskクリップ
    clip(albedo.a - alphaCutoff);

    pout.albedo = albedo;
    pout.normal = float4(N * 0.5f + 0.5f, 1.0f);
    pout.position = float4(pin.worldPos, 1.0f);
    pout.roughnessMetallic = float4(1.0f, 0.0f, 0.0f, 1.0f);
    pout.emissive = float4(0.0f, 0.0f, 0.0f, 1.0f);
    pout.occlusion = 1.0f;

    return pout;
}