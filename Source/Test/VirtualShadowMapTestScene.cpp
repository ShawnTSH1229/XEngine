#include "TestSceneCreate.h"
void CreateVSMTestScene(std::vector<std::shared_ptr<GGeomertry>>& RenderGeos)
{
    std::shared_ptr<GGeomertry> DefaultCube = TempCreateCubeGeoWithMat();
    DefaultCube->SetWorldTranslate(XVector3(-1, 1.5, 0));
    DefaultCube->GetMaterialInstance()->SetMaterialValueFloat("ConstantMetatllic", 0.8);
    DefaultCube->GetMaterialInstance()->SetMaterialValueFloat("ConstantRoughness", 0.6);

    std::shared_ptr<GGeomertry> DefaultCubeRight = DefaultCube->CreateGeoInstancewithMat();
    DefaultCubeRight->SetWorldTranslate(XVector3(1, 1.5, 0));

    std::shared_ptr<GGeomertry> DefaultQuad = TempCreateQuadGeoWithMat();
    DefaultQuad->SetWorldTranslate(XVector3(0.0, 1.0, 0.0));

    RenderGeos.push_back(DefaultCube);
    RenderGeos.push_back(DefaultCubeRight);
    RenderGeos.push_back(DefaultQuad);

    for (auto& t : RenderGeos)
    {
        t->GetGVertexBuffer()->CreateRHIBufferChecked();
        t->GetGIndexBuffer()->CreateRHIBufferChecked();
    }
}