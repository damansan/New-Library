
// ブルーム：9タップのガウスぼかし

#ifndef BLOOM_BLUR_HLSLI
#define BLOOM_BLUR_HLSLI

// direction : 横なら (1,0)、縦なら (0,1)
float3 GaussianBlur9(Texture2D tex, SamplerState samp, float2 uv, float2 direction)
{
    // 1ピクセルぶんの uv の大きさ
    uint width, height;
    tex.GetDimensions(width, height);
    float2 texel = direction / float2(width, height);

    // 9個のピクセルの重みを、真ん中1回 ＋ 左右2回ずつの5回で取る
    const float weight0 = 0.2270270270f; // 真ん中
    const float weight1 = 0.3162162162f; // 1・2個目をまとめた重み
    const float weight2 = 0.0702702703f; // 3・4個目をまとめた重み
    const float offset1 = 1.3846153846f; // 1・2個目の間
    const float offset2 = 3.2307692308f; // 3・4個目の間

    float3 color = tex.Sample(samp, uv).rgb * weight0;
    color += tex.Sample(samp, uv + texel * offset1).rgb * weight1;
    color += tex.Sample(samp, uv - texel * offset1).rgb * weight1;
    color += tex.Sample(samp, uv + texel * offset2).rgb * weight2;
    color += tex.Sample(samp, uv - texel * offset2).rgb * weight2;
    return color;
}

#endif