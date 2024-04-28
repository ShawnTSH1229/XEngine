cbuffer cbMaterial
{
	float3 ConstantColor;
    float ConstantRoughness;
    float ConstantMetatllic;
};

struct ParametersIn
{
    float2 TexCoords;
    float3 TangentToWorld0;
    float3 TangentToWorld1;
};

struct GBufferdataOutput
{
	float3  BaseColor;
    float   Metallic;
	float   Roughness;	
    float3  Normal;
	uint    ShadingModel;
};


Texture2D    BaseColorMap;
Texture2D    NormalMap;

SamplerState gsamPointWarp  : register(s0,space1000);
SamplerState gsamLinearWarp  : register(s4,space1000);

float3 NormalSampleToWorldSpace(float3 normalMapSample, float3 unitNormalW, float3 tangentW)
{
	float3 normalT = 2.0f*normalMapSample - 1.0f;// Uncompress each component from [0,1] to [-1,1].

	// Build orthonormal basis.
	float3 N = unitNormalW;
	float3 T = normalize(tangentW - dot(tangentW, N)*N);
	float3 B = cross(N, T);
	float3x3 TBN = float3x3(T, B, N);
	
	float3 bumpedNormalW = mul(normalT, TBN);// Transform from tangent space to world space.
	return bumpedNormalW;
}

void CalcMaterialParameters(ParametersIn Parameters,inout GBufferdataOutput OutputGbufferData)
{
    float4 BaseColorSample = BaseColorMap.Sample(gsamLinearWarp, Parameters.TexCoords.xy);
    float4 SampleNormalValue = NormalMap.Sample(gsamPointWarp, Parameters.TexCoords.xy);
    float3 DecodeNormal=NormalSampleToWorldSpace(SampleNormalValue.xyz,normalize(Parameters.TangentToWorld1),Parameters.TangentToWorld0);

    OutputGbufferData.BaseColor= BaseColorSample.xyz * ConstantColor.xyz;
    OutputGbufferData.Metallic= ConstantMetatllic;
    OutputGbufferData.Roughness= ConstantRoughness;
    OutputGbufferData.Normal= DecodeNormal.xyz;
    OutputGbufferData.ShadingModel=1;
}


//used for reflection
void Empty_PS(out float4 OutTarget0 : SV_Target0)
{
    ParametersIn Parameters=(ParametersIn)0;
    GBufferdataOutput OutputGbufferData;
    CalcMaterialParameters(Parameters,OutputGbufferData);
    float TempValue=OutputGbufferData.BaseColor.x+OutputGbufferData.Metallic+OutputGbufferData.Roughness+OutputGbufferData.Normal.x;
    OutTarget0=float4(TempValue,0,0,0);
}