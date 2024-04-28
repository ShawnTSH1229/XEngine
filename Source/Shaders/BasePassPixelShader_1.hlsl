
struct FVertexFactoryInterpolantsVSToPS
{
	float4 TangentToWorld0 : TEXCOORD10; 
    float4 TangentToWorld1 : TEXCOORD11;
	float4 TexCoords : TEXCOORD0;
    float3 TestWorldPosition : TEXCOORD1;
};


#include "Generated/Material.hlsl"

void PS(FVertexFactoryInterpolantsVSToPS Input,
    out float4 OutTargetA : SV_Target0,
    out float4 OutTargetB : SV_Target1,
    out float4 OutTargetC : SV_Target2,
    out float4 OutTargetD : SV_Target3
    )
{
    ParametersIn Parameters=(ParametersIn)0;
    Parameters.TexCoords = Input.TexCoords.xy;
    Parameters.TangentToWorld0 = Input.TangentToWorld0.xyz;
    Parameters.TangentToWorld1 = Input.TangentToWorld1.xyz;

    GBufferdataOutput OutputGbufferData=(GBufferdataOutput)0;
    CalcMaterialParameters(Parameters,OutputGbufferData);
    
    OutTargetA=float4(OutputGbufferData.Normal,1.0f);
    OutTargetB=float4(OutputGbufferData.Metallic,0,OutputGbufferData.Roughness,OutputGbufferData.ShadingModel);
    OutTargetC=float4(OutputGbufferData.BaseColor,1.0f);
    OutTargetD=float4(Input.TestWorldPosition,1.0f);
}