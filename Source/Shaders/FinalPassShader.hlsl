
//struct VertexIn
//{
//	float2 PosIn    : POSITION;
//	float2 TexC    : TEXCOORD;
//};


struct VertexOut
{
	float4 PosH    : SV_POSITION;
    float2 TexC    : TEXCOORD;
};

//VertexOut VS(VertexIn vin)
//{
//	VertexOut vout = (VertexOut)0.0f;
//    vout.PosH = float4(vin.PosIn,0.0f,1.0f);
//    vout.TexC = vin.TexC;
//
//    return vout;
//}

Texture2D    FullScreenMap;
SamplerState gsamPointWarp  : register(s0,space1000);

float4 PS(VertexOut pin) : SV_Target
{
    
    float4 FullScreenSample = FullScreenMap.Sample(gsamPointWarp, pin.TexC);
    //FullScreenSample=float4(saturate(pow(FullScreenSample.rgb, float3(1.0/2.2,1.0/2.2,1.0/2.2))),1.0f);
    return FullScreenSample;
}