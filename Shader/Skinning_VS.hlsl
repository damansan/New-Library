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

// 定数バッファ2：ボーン行列
#define MAX_BONES 256
cbuffer CbSkinning : register(b2)
{
    row_major float4x4 boneTransforms[MAX_BONES];
};

// 頂点入力
struct VS_IN
{
    float3 position   : POSITION;
    float3 normal     : NORMAL;
    float4 tangent    : TANGENT;
    float2 texcoord   : TEXCOORD;
    float4 boneWeight : WEIGHTS;
    uint4 boneIndex   : BONES;
};

// 頂点出力
struct VS_OUT
{
    float4 position : SV_POSITION;
    float3 normal   : NORMAL;
    float4 tangent  : TANGENT;
    float2 texcoord : TEXCOORD;
    float3 worldPos : WORLD_POSITION;
};

VS_OUT main(VS_IN vin)
{
    VS_OUT vout;

    // スキニング計算
    float4 pos = float4(vin.position, 1.0f);
    float4 normal = float4(vin.normal, 0.0f);
    float4 tangent = float4(vin.tangent.xyz, 0.0f);
    
    float4 skinnedPos = float4(0, 0, 0, 0);
    float4 skinnedNormal = float4(0, 0, 0, 0);
    float4 skinnedTangent = float4(0, 0, 0, 0);
    
    // 4ボーン分のブレンド
    for (int i = 0; i < 4; i++)
    {
        float weight = vin.boneWeight[i];
        uint index = vin.boneIndex[i];

        skinnedPos += weight * mul(pos, boneTransforms[index]);
        skinnedNormal += weight * mul(normal, boneTransforms[index]);
        skinnedTangent += weight * mul(tangent, boneTransforms[index]);
    }

    // ワールド変換
    float4 worldPos = mul(skinnedPos, world);
    vout.worldPos = worldPos.xyz;
    vout.position = mul(mul(worldPos, view), projection);
    
    // 法線・接線をワールド空間へ
    vout.normal = normalize(mul(skinnedNormal.xyz, (float3x3) world));
    vout.tangent.xyz = normalize(mul(skinnedTangent.xyz, (float3x3) world));
    vout.tangent.w = vin.tangent.w;
    
    vout.texcoord = vin.texcoord;
    
    return vout;
}