// 定数バッファ0：シーン情報
cbuffer CbScene : register(b0)
{
    row_major float4x4 view;
    row_major float4x4 projection;
    float4 lightDirection;
    float4 lightColor;
    float4 ambientColor;
    float4 cameraPosition;
};

// 定数バッファ1：オブジェクト情報
cbuffer CbObject : register(b1)
{
    row_major float4x4 world;
};

// 頂点入力
struct VS_IN
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 texcoord : TEXCOORD;
    float4 boneWeight : WEIGHTS;
    uint4 boneIndex : BONES;
};

// 頂点出力
struct VS_OUT
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
    float3 worldPos : WORLD_POSITION;
};

VS_OUT main(VS_IN vin)
{
    VS_OUT vout;

    float4 pos = float4(vin.position, 1.0f);
    pos = mul(pos, world);
    vout.worldPos = pos.xyz;
    pos = mul(pos, view);
    pos = mul(pos, projection);
    vout.position = pos;

    // 法線のワールド変換
    vout.normal = normalize(mul(vin.normal, (float3x3) world));
    vout.texcoord = vin.texcoord;

    return vout;
}