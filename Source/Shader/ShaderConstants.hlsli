#ifndef SHADER_CONSTANTS_HLSLI
#define SHADER_CONSTANTS_HLSLI


#define MAX_BONES        256 // スキニングのボーン数の上限
#define MAX_POINT_LIGHTS 64  // ポイントライトの数の上限

#define SHADOW_CASCADE_COUNT 3   // カスケードシャドウの段数

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
    
    float alphaCutoff;     // Mask素材のクリップしきい値
    float metallicFactor;  // 金属度の係数
    float roughnessFactor; // 粗さの係数
    float objectPadding0;  // 16バイトアライメント用
    
    float4 baseColorFactor; // マテリアルの基本色

    float3 emissiveFactor; // 発光の係数
    float objectPadding1;  // 16バイトアライメント用
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
    float iblIntensity;   // IBLの強さ
    float2 lightsPadding; // 16バイトアラインメント用
};

// b3：スキニング
cbuffer CbSkinning : register(b3)
{
    row_major float4x4 boneTransforms[MAX_BONES];
};

// b4：ポストエフェクト
cbuffer CbPostProcess : register(b4)
{
    float exposure;     // 露出
    int tonemapEnabled; // 1 なら ACES、0 なら切り捨て
    float contrast;     // コントラスト
    float saturation;   // 彩度

    float3 colorFilter;      // カラーフィルタ
    float vignetteIntensity; // ビネットの強さ
    
    float3 vignetteColor;     // ビネットの色
    float vignetteSmoothness; // ビネットの境目のなだらかさ
    
    float aspectRatio;    // 画面の縦横比
    float bloomThreshold; // ブルームのしきい値
    int debugView;        // デバッグ表示の種類
    float bloomScale;     // ブルームの強さ
    
    float radialBlurStrength; // ラジアルブラー
    float radialBlurRadius;   // 境目
    float radialBlurSoftness; // 滑らかさ
    
    float postPadding; // 16バイトアライメント用
};

// b5：カスケードシャドウマップ（影を読む側）
cbuffer CbShadow : register(b5)
{
    row_major float4x4 shadowViewProjections[SHADOW_CASCADE_COUNT]; // 各段のライト ビュー×プロジェクション
    
    float4 cascadeSplits; // カメラからの距離
    
    float shadowStrength; // 影の中で環境光をどれだけ暗くするか
    float3 shadowPadding; // 16バイトアライメント用
};

// b6：カスケードシャドウマップ（影を描く側）
cbuffer CbShadowCaster : register(b6)
{
    row_major float4x4 lightViewProjection; // いま描いている段のライト
};

#endif