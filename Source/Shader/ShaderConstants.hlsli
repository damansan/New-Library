#ifndef SHADER_CONSTANTS_HLSLI
#define SHADER_CONSTANTS_HLSLI


#define MAX_BONES        256 // スキニングのボーン数の上限
#define MAX_POINT_LIGHTS 64  // ポイントライトの数の上限

// b0：シーン
cbuffer CbScene : register(b0)
{
    row_major float4x4 view;
    row_major float4x4 projection;
    float4 cameraPosition; // xyz=位置 w=未使用
};

// b1：オブジェクト
cbuffer CbObject : register(b1)
{
    row_major float4x4 world;
    row_major float4x4 worldInverseTranspose; // 法線用
    float alphaCutoff;      // Mask素材のクリップしきい値
    float3 objectPadding;
    float4 baseColorFactor; // マテリアルの基本色
};

// b2：ライト
struct DirectionalLight
{
    float4 direction; // xyz=方向 w=未使用
    float4 color;     // xyz=色 w=強度
};
struct PointLight
{
    float4 position; // xyz=位置 w=範囲
    float4 color;    // xyz=色 w=強度
};
cbuffer CbLights : register(b2)
{
    DirectionalLight directionalLight;
    PointLight pointLights[MAX_POINT_LIGHTS];
    int pointLightCount;
    float3 lightsPadding;
};

// b3：スキニング
cbuffer CbSkinning : register(b3)
{
    row_major float4x4 boneTransforms[MAX_BONES];
};

// b4：カスケードシャドウマップ

// b5：ポストエフェクト

#endif