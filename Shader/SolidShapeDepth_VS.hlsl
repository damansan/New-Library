#include "SolidShape.hlsli"

// 深度プリパス用:座標だけを出力し、色の計算は一切行わない
float4 main(
    float4 position          : POSITION, // slot 0: メッシュの頂点(NORMALは使わないが、Solidメッシュと同じ頂点バッファを使うため受け取る必要がある)
    float4 normal            : NORMAL,   // slot 0: 未使用
    row_major float4x4 world : WORLD,    // slot 1: インスタンスごとのワールド行列
    float4 color             : COLOR1    // slot 1: 未使用
) : SV_POSITION
{
    return mul(position, mul(world, viewProjection));
}