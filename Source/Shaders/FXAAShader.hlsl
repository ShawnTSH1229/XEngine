struct VertexOut
{
    float4 PosH    : SV_POSITION;
    float2 TexC    : TEXCOORD;
};

Texture2D    FullScreenMap;
SamplerState gsamLinearWarp  : register(s4,space1000);

#define FXAA_PRESET 5

#if FXAA_PRESET == 0
	#define FXAA_QUALITY__PRESET 10
	#define FXAA_PC_CONSOLE 1
#elif FXAA_PRESET == 1
	#define FXAA_QUALITY__PRESET 10
	#define FXAA_PC 1
#elif FXAA_PRESET == 2
	#define FXAA_QUALITY__PRESET 13
	#define FXAA_PC 1
#elif FXAA_PRESET == 3
	#define FXAA_QUALITY__PRESET 15
	#define FXAA_PC 1
#elif FXAA_PRESET == 4
	#define FXAA_QUALITY__PRESET 29
	#define FXAA_PC 1
#elif FXAA_PRESET == 5
	#define FXAA_QUALITY__PRESET 39
	#define FXAA_PC 1
#endif

#ifndef FXAA_360_OPT
	#define FXAA_360_OPT 0
#endif

#define FXAA_HLSL_4 1

#include "Fxaa3_11.hlsl"

cbuffer cbPostprocessFXAA
{
    float2 Input_ExtentInverse;
}

float4 FXAA_PS(VertexOut pin) : SV_Target
{
    //unused variables in fxaaQualit version
    float4 fxaaConsoleRcpFrameOpt = float4(0,0,0,0);
	float4 fxaaConsoleRcpFrameOpt2 = float4(0,0,0,0);
	float4 fxaaConsole360RcpFrameOpt2 = float4(0,0,0,0);
	
    float fxaaQualitySubpix = 0.75f;
	float fxaaQualityEdgeThreshold = 0.166f;;
	float fxaaQualityEdgeThresholdMin = 0.0833f;
	
    //unused variables in fxaaQualit version
    float fxaaConsoleEdgeSharpness = 0;
	float fxaaConsoleEdgeThreshold = 0;
	float fxaaConsoleEdgeThresholdMin = 0;
	float4 fxaaConsole360ConstDir = float4(0,0,0,0);
    float4 fxaaConsolePosPos = float4(0,0,0,0);

    FxaaTex TextureAndSampler;
	TextureAndSampler.tex = FullScreenMap;
	TextureAndSampler.smpl = gsamLinearWarp;
	TextureAndSampler.UVMinMax = float4(0.0,0.0,1.0,1.0);

    float2 TexCenter = pin.TexC;
    return FxaaPixelShader(
			TexCenter, fxaaConsolePosPos,
			TextureAndSampler,
			TextureAndSampler,
			TextureAndSampler,
			Input_ExtentInverse,
			fxaaConsoleRcpFrameOpt,
			fxaaConsoleRcpFrameOpt2,
			fxaaConsole360RcpFrameOpt2,
			fxaaQualitySubpix,
			fxaaQualityEdgeThreshold,
			fxaaQualityEdgeThresholdMin,
			fxaaConsoleEdgeSharpness,
			fxaaConsoleEdgeThreshold,
			fxaaConsoleEdgeThresholdMin,
			fxaaConsole360ConstDir);
}

