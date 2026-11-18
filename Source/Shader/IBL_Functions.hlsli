#ifndef IBL_FUNCTIONS_HLSLI
#define IBL_FUNCTIONS_HLSLI

// Diffuse IBL
// irradianceMap: 事前畳み込み済みのDiffuse環境マップ
// N            : 法線ベクトル（ワールド空間）
float3 IBL_Diffuse(
    TextureCube irradianceMap,
    SamplerState samplerState,
    float3 N,
    float3 diffuse_reflectance,
    float3 F0,
    float3 V)
{
    // フレネル項
    float NdotV = max(0.0001f, dot(N, V));
    float3 F = F0 + (1.0f - F0) * pow(clamp(1.0f - NdotV, 0.0f, 1.0f), 5.0f);
    
    // kd
    float3 kd = (1.0f - F);

    // 法線方向でIrradianceマップをサンプリング
    float3 irradiance = irradianceMap.Sample(samplerState, N).rgb;

    return kd * diffuse_reflectance * irradiance;
}

// Specular IBL
// prefilteredMap: roughnessごとにMipがある事前フィルタリング済みマップ
// brdfLut       : GGX BRDFのLookUpTable
// N             : 法線ベクトル
// V             : 視線ベクトル
// F0            : 垂直入射時のフレネル反射率
// roughness     : 粗さ
float3 IBL_Specular(
    TextureCube prefilteredMap,
    Texture2D brdfLut,
    SamplerState samplerState,
    float3 N,
    float3 V,
    float3 F0,
    float roughness)
{
    float NdotV = max(0.0001f, dot(N, V));

    // 反射方向
    float3 R = reflect(-V, N);

    // roughnessに応じてMipレベルを選択
    uint width, height, mipCount;
    prefilteredMap.GetDimensions(0, width, height, mipCount);
    float mipLevel = roughness * (float) (mipCount - 1);

    // 事前フィルタリング済み環境マップをサンプリング
    float3 prefilteredColor = prefilteredMap.SampleLevel(
        samplerState, R, mipLevel).rgb;

    // BRDF LUTをサンプリング
    float2 brdf = brdfLut.Sample(samplerState,
        float2(NdotV, roughness)).rg;

    // フレネル項
    float3 F = F0 + (1.0f - F0) * pow(clamp(1.0f - NdotV, 0.0f, 1.0f), 5.0f);
    
    return prefilteredColor * (F * brdf.x + brdf.y);
}

#endif