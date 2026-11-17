//SamplerState samplerState : register(s0);

//cbuffer CbObject : register(b2)
//{
//    row_major float4x4 world;
//    float alphaCutoff;
//    float3 padding;
//    float4 baseColorFactor; // 格子の下地の色（リニア空間）
//};

//// 格子の設定（ワールド単位）
//static const float GRID_SIZE = 1.0f; // 線の間隔
//static const float LINE_WIDTH = 0.02f; // 線の太さ
//static const float LINE_DARKNESS = 0.66f; // 線の暗さ（下地の色に掛ける、UEの細線に合わせた値）
//static const float3 GRID_BASE_COLOR = float3(0.18f, 0.18f, 0.18f); // 下地の色（リニア、UEのグレーに合わせた値）

//struct VS_OUT
//{
//    float4 position : SV_POSITION;
//    float3 normal : NORMAL;
//    float4 tangent : TANGENT;
//    float2 texcoord : TEXCOORD;
//    float3 worldPos : WORLD_POSITION;
//};

//struct PS_OUT
//{
//    float4 albedo : SV_TARGET0;
//    float4 normal : SV_TARGET1;
//    float4 position : SV_TARGET2;
//    float4 roughnessMetallic : SV_TARGET3;
//    float4 emissive : SV_TARGET4;
//    float occlusion : SV_TARGET5;
//};

//PS_OUT main(VS_OUT pin)
//{
//    PS_OUT pout;

//    float3 N = normalize(pin.normal);

//    // 面の向きから投影する2軸を選ぶ（一番大きい法線成分の軸を捨てる）
//    float3 an = abs(N);
//    float2 uv;
//    if (an.x >= an.y && an.x >= an.z)
//        uv = pin.worldPos.zy; // 左右を向いた面
//    else if (an.y >= an.z)
//        uv = pin.worldPos.xz; // 上下を向いた面
//    else
//        uv = pin.worldPos.xy; // 前後を向いた面

//    // マス単位の座標
//    float2 g = uv / GRID_SIZE;

//    // 一番近い線までの距離（マス単位、0〜0.5）
//    float2 d = abs(frac(g - 0.5f) - 0.5f);

//    // 1ピクセルあたりの変化量（遠くで線がちらつかないようにぼかし幅に使う）
//    float2 w = max(fwidth(g), 1e-5f);

//    // 線の上なら1、線の外なら0（境界はwの幅でなめらかに）
//    float halfWidth = (LINE_WIDTH / GRID_SIZE) * 0.5f;
//    float2 lineMask = 1.0f - saturate((d - halfWidth) / w);
//    float onLine = max(lineMask.x, lineMask.y);

//    // 色（どちらもリニアなので掛け合わせる）
//    float3 baseCol = GRID_BASE_COLOR * baseColorFactor.rgb;
//    float3 lineCol = baseCol * LINE_DARKNESS;
//    float4 albedo = float4(lerp(baseCol, lineCol, onLine), baseColorFactor.a);

//    // Maskクリップ
//    clip(albedo.a - alphaCutoff);

//    pout.albedo = albedo;
//    pout.normal = float4(N * 0.5f + 0.5f, 1.0f);
//    pout.position = float4(pin.worldPos, 1.0f);
//    pout.roughnessMetallic = float4(1.0f, 0.0f, 0.0f, 1.0f);
//    pout.emissive = float4(0.0f, 0.0f, 0.0f, 1.0f);
//    pout.occlusion = 1.0f;

//    return pout;
//}



SamplerState samplerState : register(s0);

cbuffer CbObject : register(b2)
{
    row_major float4x4 world;
    float alphaCutoff;
    float3 padding;
    float4 baseColorFactor; // 格子の下地の色（リニア空間）
};

// 市松模様の設定（ワールド単位）
static const float GRID_SIZE = 1.0f; // 市松1マスの大きさ
static const float3 CHECKER_COLOR_A = float3(0.18f, 0.18f, 0.18f); // 明るいマス（リニア、灰色）
static const float3 CHECKER_COLOR_B = float3(0.05f, 0.05f, 0.05f); // 暗いマス（リニア、黒っぽい色）

// 格子線の設定（ワールド単位）
static const float SUB_DIVISION = 4.0f; // 1マスを何分割する細線を引くか
static const float MAJOR_LINE_WIDTH = 0.02f; // 太線（マスの境界）の太さ
static const float MINOR_LINE_WIDTH = 0.01f; // 細線の太さ
static const float3 MAJOR_LINE_COLOR = float3(0.6f, 0.6f, 0.6f); // 太線の色（リニア、白っぽい色）
static const float MINOR_LINE_DARKNESS = 0.8f; // 細線の暗さ（マスの色に掛ける）

struct VS_OUT
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 texcoord : TEXCOORD;
    float3 worldPos : WORLD_POSITION;
};

// 線の上なら1、線の外なら0を返す（境界は1ピクセル幅でなめらかに）
// g    : 線の間隔を1とした座標
// width: 線の間隔を1とした線の太さ
float LineMask(float2 g, float width)
{
    // 一番近い線までの距離（0〜0.5）
    float2 d = abs(frac(g - 0.5f) - 0.5f);

    // 1ピクセルあたりの変化量（遠くで線がちらつかないようにぼかし幅に使う）
    float2 w = max(fwidth(g), 1e-5f);

    float2 m = 1.0f - saturate((d - width * 0.5f) / w);
    return max(m.x, m.y);
}

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

    // 今いるマスの番号（整数）
    float2 cell = floor(g);

    // 番号の和が偶数なら0、奇数なら1（負の座標でも崩れないようにfracを使う）
    float checker = frac((cell.x + cell.y) * 0.5f) * 2.0f;

    // 太線：マスの境界（線の間隔＝1マス）
    float major = LineMask(g, MAJOR_LINE_WIDTH / GRID_SIZE);

    // 細線：1マスをSUB_DIVISION分割（線の間隔＝1マス/SUB_DIVISION）
    float minor = LineMask(g * SUB_DIVISION, MINOR_LINE_WIDTH * SUB_DIVISION / GRID_SIZE);

    // マスの色に細線の暗さを掛ける
    float3 col = lerp(CHECKER_COLOR_A, CHECKER_COLOR_B, checker);
    col *= lerp(1.0f, MINOR_LINE_DARKNESS, minor);

    // 太線は決まった色で上書きする（最後に塗るので、細線と重なる所も太線が優先される）
    col = lerp(col, MAJOR_LINE_COLOR, major);

    // マテリアルの色味を全体に掛ける（どちらもリニア）
    col *= baseColorFactor.rgb;
    float4 albedo = float4(col, baseColorFactor.a);
    
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