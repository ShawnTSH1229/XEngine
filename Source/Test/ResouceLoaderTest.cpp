#include "ResouceLoaderTest.h"
#include "ThirdParty/tiny_obj_loader.h"

#include "Runtime/Engine/ResourcecConverter.h"

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/wstringize.hpp>

void OBjLoaderTest(std::vector<std::shared_ptr<GGeomertry>>& RenderGeosOut)
{
    std::shared_ptr<GTexture2D> TexBaseColor =
        CreateTextureFromImageFile("E:/XEngine/XEnigine/ContentSave/TextureNoAsset/T_Metal_Gold_D.TGA", true);

    std::shared_ptr<GTexture2D> TexRouhness =
        CreateTextureFromImageFile("E:/XEngine/XEnigine/ContentSave/TextureNoAsset/T_MacroVariation.TGA", false);

    std::shared_ptr<GTexture2D> TexNormal =
        CreateTextureFromImageFile("E:/XEngine/XEnigine/ContentSave/TextureNoAsset/T_Metal_Gold_N.TGA", false);

    TexBaseColor->CreateRHITexture();
    TexRouhness->CreateRHITexture();
    TexNormal->CreateRHITexture();

    std::shared_ptr<GMaterial> MaterialPtr = CreateMaterialFromCode(L"E:/XEngine/XEnigine/MaterialShaders/Material.hlsl");
    std::shared_ptr<GMaterialInstance> MaterialIns = std::make_shared<GMaterialInstance>(MaterialPtr);
    MaterialIns->SetMaterialValueFloat("Metallic", 0.5f);
    MaterialIns->SetMaterialValueFloat("Roughness", 0.5f);
    MaterialIns->SetMaterialValueFloat("TextureScale", 1.0f);
    MaterialIns->SetMaterialTexture2D("BaseColorMap", TexBaseColor);
    MaterialIns->SetMaterialTexture2D("RoughnessMap", TexRouhness);
    MaterialIns->SetMaterialTexture2D("NormalMap", TexNormal);

	std::string InputFile = BOOST_PP_CAT(BOOST_PP_STRINGIZE(ROOT_DIR_XENGINE), "/ContentSave/Sponza/sponza.obj");
	
    tinyobj::ObjReader       reader;
    tinyobj::ObjReaderConfig reader_config;
    reader_config.vertex_color = false;
    if (!reader.ParseFromFile(InputFile, reader_config))
    {
        if (!reader.Error().empty())
        {
            std::cerr << "TinyObjReader: " << reader.Error();
        }
        assert(0);
    }

    if (!reader.Warning().empty())
    {
        std::cout << "TinyObjReader: " << reader.Warning();
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();



    for (size_t s = 0; s < shapes.size(); s++)
    {
        XVector3 BoundingBoxMaxVar((std::numeric_limits<float>::min)(), (std::numeric_limits<float>::min)(), (std::numeric_limits<float>::min)());
        XVector3 BoundingBoxMinVar((std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)());

        std::vector<XVector4>Positions;
        std::vector<XVector3>TangentX;
        std::vector<XVector4>TangentY;
        std::vector<XVector2>TextureCoords;
        std::vector<uint16>Indices;

        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
        {
            size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

            bool with_normal = true;
            bool with_texcoord = true;

            XVector3 vertex[3];
            XVector3 normal[3];
            XVector2 uv[3];

            if (fv != 3)
            {
                continue;
            }

            for (size_t v = 0; v < fv; v++)
            {
                auto idx = shapes[s].mesh.indices[index_offset + v];
                auto vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
                auto vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
                auto vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

                vertex[v].x = static_cast<float>(vx);
                vertex[v].y = static_cast<float>(vy);
                vertex[v].z = static_cast<float>(vz);

                BoundingBoxMaxVar = XVector3::Max(BoundingBoxMaxVar, vertex[v]);
                BoundingBoxMinVar = XVector3::Min(BoundingBoxMinVar, vertex[v]);

                if (idx.normal_index >= 0)
                {
                    auto nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
                    auto ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
                    auto nz = attrib.normals[3 * size_t(idx.normal_index) + 2];

                    normal[v].x = static_cast<float>(nx);
                    normal[v].y = static_cast<float>(ny);
                    normal[v].z = static_cast<float>(nz);
                }
                else
                {
                    with_normal = false;
                }

                if (idx.texcoord_index >= 0)
                {
                    auto tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
                    auto ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];

                    uv[v].x = static_cast<float>(tx);
                    uv[v].y = static_cast<float>(ty);
                }
                else
                {
                    with_texcoord = false;
                }
            }

            

            if (!with_normal)
            {
                XVector3 v0 = vertex[1] - vertex[0];
                XVector3 v1 = vertex[2] - vertex[1];
                normal[0] = v0.Cross(v1).NormalizeRet();
                normal[1] = normal[0];
                normal[2] = normal[0];
            }

            if (!with_texcoord)
            {
                uv[0] = XVector2(0.5f, 0.5f);
                uv[1] = XVector2(0.5f, 0.5f);
                uv[2] = XVector2(0.5f, 0.5f);
            }

            XVector3 tangent{ 1, 0, 0 };
            {
                XVector3 edge1 = vertex[1] - vertex[0];
                XVector3 edge2 = vertex[2] - vertex[1];
                XVector2 deltaUV1 = uv[1] - uv[0];
                XVector2 deltaUV2 = uv[2] - uv[1];

                auto divide = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;
                if (divide >= 0.0f && divide < 0.000001f)
                    divide = 0.000001f;
                else if (divide < 0.0f && divide > -0.000001f)
                    divide = -0.000001f;

                float df = 1.0f / divide;
                tangent.x = df * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
                tangent.y = df * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
                tangent.z = df * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
                tangent = (tangent).NormalizeRet();
            }

            for (size_t i = 0; i < 3; i++)
            {
                Positions.push_back(XVector4(vertex[i].x, vertex[i].y, vertex[i].z, 1.0));
                TangentX.push_back(tangent);
                TangentY.push_back(XVector4(normal[i].x, normal[i].y, normal[i].z, 1.0));
                TextureCoords.push_back(uv[i]);
                Indices.push_back(static_cast<uint16>(index_offset + i));
            }

            index_offset += fv;
        }

        std::shared_ptr<GDataBuffer> PositionDataBuffer = std::make_shared<GDataBuffer>();
        PositionDataBuffer->SetData((uint8*)Positions.data(), Positions.size(), EVertexElementType::VET_Float4);

        std::shared_ptr<GDataBuffer> TangentXDataBuffer = std::make_shared<GDataBuffer>();
        TangentXDataBuffer->SetData((uint8*)TangentX.data(), TangentX.size(), EVertexElementType::VET_Float3);

        std::shared_ptr<GDataBuffer> TangentYDataBuffer = std::make_shared<GDataBuffer>();
        TangentYDataBuffer->SetData((uint8*)TangentY.data(), TangentY.size(), EVertexElementType::VET_Float4);

        std::shared_ptr<GDataBuffer> TextureCoordsDataBuffer = std::make_shared<GDataBuffer>();
        TextureCoordsDataBuffer->SetData((uint8*)TextureCoords.data(), TextureCoords.size(), EVertexElementType::VET_Float2);

        std::shared_ptr<GDataBuffer> IndexDataBuffer = std::make_shared<GDataBuffer>();
        IndexDataBuffer->SetData((uint8*)Indices.data(), Indices.size(), EVertexElementType::VET_UINT16);

        std::shared_ptr<GVertexBuffer>VertexBuffer = std::make_shared<GVertexBuffer>();
        VertexBuffer->SetData(PositionDataBuffer, EVertexAttributeType::VAT_POSITION);
        VertexBuffer->SetData(TangentXDataBuffer, EVertexAttributeType::VAT_TANGENT);
        VertexBuffer->SetData(TangentYDataBuffer, EVertexAttributeType::VAT_NORMAL);
        VertexBuffer->SetData(TextureCoordsDataBuffer, EVertexAttributeType::VAT_TEXCOORD);

        std::shared_ptr<GIndexBuffer> IndexBuffer = std::make_shared<GIndexBuffer>();
        IndexBuffer->SetData(IndexDataBuffer);

        std::shared_ptr<GMeshData>MeshData = std::make_shared<GMeshData>();
        MeshData->SetVertexBuffer(VertexBuffer);
        MeshData->SetIndexBuffer(IndexBuffer);

        std::shared_ptr<GGeomertry>Geomertry = std::make_shared<GGeomertry>();
        Geomertry->SetMeshData(MeshData);

        XVector3 Center = (BoundingBoxMaxVar + BoundingBoxMinVar) * 0.5;
        XVector3 Extent = BoundingBoxMaxVar - Center;
        Geomertry->SetBoundingBox(Center, Extent);




        Geomertry->SetMaterialPtr(MaterialIns);

        RenderGeosOut.push_back(Geomertry);
    }
}
