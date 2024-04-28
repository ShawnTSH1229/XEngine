struct VertexOut
{
	float4 PosH    : SV_POSITION;
    float2 TexC    : TEXCOORD;
};

Texture2D    FullScreenMap;
SamplerState gsamPointWarp  : register(s0,space1000);

float4 ToneMapping_PS(VertexOut pin) : SV_Target
{

    float4 FullScreenSample = FullScreenMap.Sample(gsamPointWarp, pin.TexC);
    FullScreenSample = float4(saturate(pow(FullScreenSample.rgb, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2))), 1.0);
    FullScreenSample.a = dot(FullScreenSample.xyz, float3(0.3, 0.59, 0.11));
    return FullScreenSample;
}