#include "WiredShape.hlsli"

VS_OUT main(
    float4 position : POSITION,       // slot 0: メッシュの頂点
    row_major float4x4 world : WORLD, // slot 1: インスタンスごとのワールド行列
    float4 color : COLOR1)            // slot 1: インスタンスごとの色
{
    VS_OUT vout;

    float4x4 worldViewProjection = mul(world, viewProjection);
    vout.position = mul(position, worldViewProjection);
    vout.color = color;

    return vout;
}