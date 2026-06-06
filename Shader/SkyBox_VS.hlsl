cbuffer CbSkyBox : register(b0)
{
    row_major float4x4 view;
    row_major float4x4 projection;
    float4 cameraPosition;
    float4 padding;
};

struct VS_IN
{
    float3 position : POSITION;
};

struct VS_OUT
{
    float4 position : SV_POSITION;
    float3 texcoord : TEXCOORD;
};

VS_OUT main(VS_IN vin)
{
    VS_OUT vout;

    // 頂点座標をそのままサンプリング方向として保存
    vout.texcoord = vin.position;

    // カメラ位置に追従（worldは使わない）
    float3 worldPos = vin.position + cameraPosition.xyz;

    float4 viewPos = mul(float4(worldPos, 1.0f), view);
    float4 clipPos = mul(viewPos, projection);

    // 深度値を常に最大(1.0)にする
    vout.position = clipPos.xyww;

    return vout;
}