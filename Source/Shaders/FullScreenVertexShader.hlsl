struct VertexIn
{
	float2 PosIn    : ATTRIBUTE0;
	float2 Tex    : ATTRIBUTE1;
};

void VS(VertexIn vin,
    out float4 PosH: SV_POSITION,
    out float2 Tex: TEXCOORD0
)
{
    PosH = float4(vin.PosIn, 0.0f, 1.0f);
    Tex = vin.Tex;
}