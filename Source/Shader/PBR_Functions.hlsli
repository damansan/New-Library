#ifndef PBR_FUNCTIONS_HLSLI
#define PBR_FUNCTIONS_HLSLI

#ifndef PI
#define PI 3.14159265358979323846f
#endif

#ifndef GAMMA
#define GAMMA 2.2f
#endif

// フレネル項 (Schlickの近似式)

// F0    : 垂直入射時の反射率
// VdotH : 視線ベクトルとハーフベクトルの内積
float3 CalcFresnel(float3 F0, float VdotH)
{
    return F0 + (1.0f - F0) * pow(clamp(1.0f - VdotH, 0.0f, 1.0f), 5.0f);
}

// 拡散反射BRDF (正規化ランバート)

// VdotH              : 視線とハーフベクトルの内積
// F0                 : 垂直入射時のフレネル反射率
// diffuse_reflectance: 拡散反射率
float3 DiffuseBRDF(float VdotH, float3 F0, float3 diffuse_reflectance)
{
    return (1.0f - CalcFresnel(F0, VdotH)) * (diffuse_reflectance / PI);
}

// D項: 法線分布関数 (GGX)

// NdotH    : 法線とハーフベクトルの内積
// roughness: 粗さ
float CalcNDF(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float d = (NdotH * NdotH) * (a2 - 1.0f) + 1.0f;
    return a2 / (PI * d * d);
}

// G項: 幾何減衰項 (Smith-Schlick)

// NdotL    : 法線と光源ベクトルの内積
// NdotV    : 法線と視線ベクトルの内積
// roughness: 粗さ
float CalcGeometry(float NdotL, float NdotV, float roughness)
{
    float r = roughness * 0.5f;
    float shadowing = NdotL / (NdotL * (1.0f - r) + r);
    float masking = NdotV / (NdotV * (1.0f - r) + r);
    return shadowing * masking;
}

// 鏡面反射BRDF (Cook-Torranceモデル)

// NdotV    : 法線と視線ベクトルの内積
// NdotL    : 法線と光源ベクトルの内積
// NdotH    : 法線とハーフベクトルの内積
// VdotH    : 視線とハーフベクトルの内積
// F0       : 垂直入射時のフレネル反射率
// roughness: 粗さ
float3 SpecularBRDF(float NdotV, float NdotL, float NdotH, float VdotH,
                    float3 F0, float roughness)
{
    float D = CalcNDF(NdotH, roughness);
    float G = CalcGeometry(NdotL, NdotV, roughness);
    float3 F = CalcFresnel(F0, VdotH);
    return (D * G * F) / (NdotL * NdotV * 4.0f);
}

// 直接光のPBRライティング

// diffuse_reflectance: 拡散反射率
// F0                 : 垂直入射時のフレネル反射率
// N                  : 法線 (正規化済み)
// V                  : 視線ベクトル (カメラ方向, 正規化済み)
// L                  : 光源方向ベクトル (光源へ向かう, 正規化済み)
// light_color        : ライトカラー
// roughness          : 粗さ
void DirectBRDF(
    float3 diffuse_reflectance,
    float3 F0,
    float3 N, float3 V, float3 L,
    float3 light_color,
    float roughness,
    out float3 out_diffuse,
    out float3 out_specular)
{
    float3 H = normalize(L + V);
    float NdotV = max(0.0001f, dot(N, V));
    float NdotL = max(0.0001f, dot(N, L));
    float NdotH = max(0.0001f, dot(N, H));
    float VdotH = max(0.0001f, dot(V, H));

    float3 irradiance = light_color * NdotL;

    out_diffuse = DiffuseBRDF(VdotH, F0, diffuse_reflectance) * irradiance;
    out_specular = SpecularBRDF(NdotV, NdotL, NdotH, VdotH, F0, roughness) * irradiance;
}

#endif