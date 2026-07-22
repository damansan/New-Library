cbuffer CbShadowScene : register(b0)
{
    row_major float4x4 lightViewProjection;
};

cbuffer CbObject : register(b1)
{
    row_major float4x4 world;
};

#define MAX_BONES 256
cbuffer CbSkinning : register(b2)
{
    row_major float4x4 boneTransforms[MAX_BONES];
};

struct VS_IN
{
    float3 position : POSITION;
    float4 boneWeight : WEIGHTS;
    uint4 boneIndex : BONES;
};

float4 main(VS_IN vin) : SV_POSITION
{
    float4 pos = float4(vin.position, 1.0f);
    float4 skinnedPos = float4(0, 0, 0, 0);

    for (int i = 0; i < 4; i++)
    {
        float weight = vin.boneWeight[i];
        uint index = vin.boneIndex[i];
        skinnedPos += weight * mul(pos, boneTransforms[index]);
    }

    float4 worldPos = mul(skinnedPos, world);
    return mul(worldPos, lightViewProjection);
}