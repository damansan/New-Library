Texture2D baseMap              : register(t0); // Albedo
Texture2D normalMap            : register(t1); // 法線マップ
Texture2D metallicRoughnessMap : register(t2); // G:Roughness B:Metallic
Texture2D occlusionMap         : register(t3); // R:Occlusion
Texture2D emissiveMap          : register(t4); // Emissive

SamplerState samplerState      : register(s0);

cbuffer CbObject : register(b2)
{
    row_major float4x4 world;
    float alphaCutoff;
    float3 padding;
};

struct VS_OUT
{
    float4 position : SV_POSITION;
    float3 normal   : NORMAL;
    float4 tangent  : TANGENT;
    float2 texcoord : TEXCOORD;
    float3 worldPos : WORLD_POSITION;
};

// 出力構造体
struct PS_OUT
{
    float4 albedo            : SV_TARGET0; // Albedo
    float4 normal            : SV_TARGET1; // Normal（ワールド空間）
    float4 position          : SV_TARGET2; // WorldPosition
    float4 roughnessMetallic : SV_TARGET3; // R:Roughness G:Metallic
    float4 emissive          : SV_TARGET4; // Emissive
    float occlusion          : SV_TARGET5; // Occlusion
};

PS_OUT main(VS_OUT pin)
{
    PS_OUT pout;

    // Albedo
    float4 baseColor = baseMap.Sample(samplerState, pin.texcoord);

    // Maskクリップ
    clip(baseColor.a - alphaCutoff);

    baseColor.rgb = pow(max(baseColor.rgb, 0.0001f), 2.2f);
    pout.albedo = baseColor;
    
    // 法線マッピング
    float3 N = normalize(pin.normal);
    float3 T = normalize(pin.tangent.xyz);
    float sigma = pin.tangent.w;

    T = normalize(T - N * dot(N, T));
    float3 B = normalize(cross(N, T) * sigma);

    float3 normalSample = normalMap.Sample(samplerState, pin.texcoord).xyz;
    normalSample = normalSample * 2.0f - 1.0f;
    N = normalize(normalSample.x * T
                + normalSample.y * B
                + normalSample.z * N);

    pout.normal = float4(N * 0.5f + 0.5f, 1.0f);

    // ワールド座標
    pout.position = float4(pin.worldPos, 1.0f);

    // MetallicRoughness
    float4 mr = metallicRoughnessMap.Sample(samplerState, pin.texcoord);
    pout.roughnessMetallic = float4(
        max(mr.g, 0.01f), // R: Roughness
        mr.b, // G: Metallic
        0.0f,
        1.0f
    );

    // Emissive
    float3 emissive = emissiveMap.Sample(samplerState, pin.texcoord).rgb;
    emissive = pow(max(emissive, 0.0001f), 2.2f);
    pout.emissive = float4(emissive, 1.0f);
    pout.occlusion = occlusionMap.Sample(samplerState, pin.texcoord).r;

    return pout;
}