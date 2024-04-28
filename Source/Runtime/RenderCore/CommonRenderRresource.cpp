#include "CommonRenderRresource.h"
#include "Runtime/RHI/RHICommandList.h"
#include <array>
#include "Runtime\Core\Misc\Path.h"

TGlobalResource<RFullScreenQuadVertexLayout> GFullScreenLayout;
TGlobalResource<XScreenQuadVertexBuffer> GFullScreenVertexRHI;
TGlobalResource<XScreenQuadIndexBuffer> GFullScreenIndexRHI;

RFullScreenQuadVS::ShaderInfos RFullScreenQuadVS::StaticShaderInfos(
	"RFullScreenQuadVS", GET_SHADER_PATH("FullScreenVertexShader.hlsl"),
	"VS", EShaderType::SV_Vertex, RFullScreenQuadVS::CustomConstrucFunc,
	RFullScreenQuadVS::ModifyShaderCompileDefines);

void XScreenQuadVertexBuffer::InitRHI()
{
	TResourceVector<XFullScreenQuadVertex>Vertices;
	Vertices.PushBack(XFullScreenQuadVertex(XVector2(-1.0f, -1.0f), XVector2(0.0f, 1.0f)));
	Vertices.PushBack(XFullScreenQuadVertex(XVector2(-1.0f, 1.0f), XVector2(0.0f, 0.0f)));
	Vertices.PushBack(XFullScreenQuadVertex(XVector2(1.0f, 1.0f), XVector2(1.0f, 0.0f)));
	Vertices.PushBack(XFullScreenQuadVertex(XVector2(1.0f, -1.0f), XVector2(1.0f, 1.0f)));
	
	XRHIResourceCreateData CreateData(&Vertices);
	RHIVertexBuffer = RHIcreateVertexBuffer(sizeof(XFullScreenQuadVertex), 4* sizeof(XFullScreenQuadVertex), EBufferUsage::BUF_Static, CreateData);
}

void XScreenQuadIndexBuffer::InitRHI()
{
	TResourceVector<uint16> Indecies;
	Indecies.PushBack(0);
	Indecies.PushBack(1);
	Indecies.PushBack(2);
	Indecies.PushBack(0);
	Indecies.PushBack(2);
	Indecies.PushBack(3);
	XRHIResourceCreateData CreateData(&Indecies);
	RHIIndexBuffer = RHICreateIndexBuffer(sizeof(uint16), 6 * sizeof(uint16), EBufferUsage::BUF_Static, CreateData);
}
