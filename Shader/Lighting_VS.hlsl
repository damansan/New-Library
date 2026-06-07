struct VS_OUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

VS_OUT main(uint vertexID : SV_VertexID)
{
    VS_OUT vout;

    // 頂点IDから座標とUVを生成（三角形2枚で画面全体を覆う）
    // vertexID: 0→左下 1→左上 2→右下 3→右上
    float2 pos;
    pos.x = (vertexID & 1) ? 1.0f : -1.0f;
    pos.y = (vertexID & 2) ? 1.0f : -1.0f;

    vout.position = float4(pos, 0.0f, 1.0f);

    // UV座標
    vout.texcoord.x = (pos.x + 1.0f) * 0.5f;
    vout.texcoord.y = (1.0f - pos.y) * 0.5f;

    return vout;
}