#include "SolidShape.hlsli"

VS_OUT main(
    float4 position : POSITION,       // slot 0: メッシュの頂点
    float4 normal : NORMAL,           // slot 0: メッシュの頂点
    row_major float4x4 world : WORLD, // slot 1: インスタンスごとのワールド行列
    float4 color : COLOR1)            // slot 1: インスタンスごとの色
{
    normal.w = 0;

    VS_OUT vout;
    vout.position = mul(position, mul(world, viewProjection));
    vout.normal = mul(normal, world).xyz;
    vout.color = color; // ピクセルシェーダーへそのまま橋渡し

    return vout;
}