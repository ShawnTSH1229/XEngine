#include "TestSceneCreate.h"
#include "Runtime/Render/VirtualShadowMapArray.h"

void CreateVSMTestScene(std::vector<std::shared_ptr<GGeomertry>>& RenderGeos, XBoundSphere& BoundSphere)
{
    float VSM_MAX_MIP_RESOLUTION = int(VSM_MIN_LEVEL_DISTANCE * 2.4) << VSM_MIP_NUM;
    XVector3 BoundMin(-VSM_MAX_MIP_RESOLUTION * 0.5, -10, -VSM_MAX_MIP_RESOLUTION * 0.5);
    XVector3 BoundMax(+VSM_MAX_MIP_RESOLUTION * 0.5, +10, +VSM_MAX_MIP_RESOLUTION * 0.5);
    
    BoundSphere.Center = (BoundMin + BoundMax) * 0.5;
    BoundSphere.Radius = XVector3(BoundMax - BoundSphere.Center).Length();

    std::shared_ptr<GGeomertry> DefaultCube = TempCreateCubeGeoWithMat();
    std::shared_ptr<GGeomertry> DefaultQuad = TempCreateQuadGeoWithMat();

    DefaultCube->GetMaterialInstance()->SetMaterialValueFloat("ConstantMetatllic", 0.8);
    DefaultCube->GetMaterialInstance()->SetMaterialValueFloat("ConstantRoughness", 0.6);
    std::shared_ptr<GGeomertry> CenterCube = DefaultCube->CreateGeoInstancewithMat();
    CenterCube->SetWorldTranslate(XVector3(1, 1.5, 0));
    RenderGeos.push_back(CenterCube);


    {
        float Mip0CubeNumPerDimension = VSM_TILE_MAX_MIP_NUM_XY;
        float Mip0Stride = int(VSM_MIN_LEVEL_DISTANCE * 2.8) / Mip0CubeNumPerDimension;
        for (int Mip0X = 0; Mip0X < Mip0CubeNumPerDimension; Mip0X++)
        {
            for (int Mip0Z = 0; Mip0Z < Mip0CubeNumPerDimension; Mip0Z++)
            {
                std::shared_ptr<GGeomertry> CubeInstance = DefaultCube->CreateGeoInstancewithMat();
                std::shared_ptr<GGeomertry> QuadInstance = DefaultQuad->CreateGeoInstancewithMat();
                float XPos = Mip0X * Mip0Stride + (-VSM_MIN_LEVEL_DISTANCE * 1.4);
                float ZPos = Mip0Z * Mip0Stride + (-VSM_MIN_LEVEL_DISTANCE * 1.4);
                CubeInstance->SetWorldTranslate(XVector3(XPos, 1.0, ZPos));
                QuadInstance->SetWorldTranslate(XVector3(XPos, 0, ZPos));
                QuadInstance->SetWorldScale(XVector3(0.25, 0.25, 0.25));
                RenderGeos.push_back(CubeInstance);
                RenderGeos.push_back(QuadInstance);
            }
        }
    }

    for (int MipIndex = 1; MipIndex < VSM_MIP_NUM; MipIndex++)
    {
        float MipCubeNumPerDimension = VSM_TILE_MAX_MIP_NUM_XY;
        
        float PreMipRange = (int(VSM_MIN_LEVEL_DISTANCE * 2.8) << (MipIndex - 1)) * 2.0;
        float CurrentMipRange = (int(VSM_MIN_LEVEL_DISTANCE * 2.8) << MipIndex) * 2.0;
       
        float MipStride = CurrentMipRange / MipCubeNumPerDimension;
    
        for (int MipX = 0; MipX < MipCubeNumPerDimension; MipX++)
        {
            for (int MipZ = 0; MipZ < MipCubeNumPerDimension; MipZ++)
            {
                std::shared_ptr<GGeomertry> CubeInstance = DefaultCube->CreateGeoInstancewithMat();
                std::shared_ptr<GGeomertry> QuadInstance = DefaultQuad->CreateGeoInstancewithMat();
                float XPos = MipX * MipStride + (-CurrentMipRange * 0.5);
                float ZPos = MipZ * MipStride + (-CurrentMipRange * 0.5);
                if (!((std::abs(XPos) < PreMipRange * 0.5) && (std::abs(ZPos) < PreMipRange * 0.5)))
                {
                    CubeInstance->SetWorldTranslate(XVector3(XPos, 1.0, ZPos));
                    QuadInstance->SetWorldTranslate(XVector3(XPos, 0, ZPos));
                    QuadInstance->SetWorldScale(XVector3(0.25, 0.25, 0.25));
                    RenderGeos.push_back(CubeInstance);
                    RenderGeos.push_back(QuadInstance);
                }
            }
        }
    }



    for (auto& t : RenderGeos)
    {
        t->GetGVertexBuffer()->CreateRHIBufferChecked();
        t->GetGIndexBuffer()->CreateRHIBufferChecked();
    }
}