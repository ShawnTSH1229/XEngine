#pragma once
#include "Runtime/Core/Mesh/GeomertyNode.h"
#include "Runtime/Engine/Classes/Texture.h"
#include "Runtime/Engine/Classes/Material.h"

//convert texture
//convert mesh
std::shared_ptr<GTexture2D> CreateTextureFromImageFile(const std::string& FilePath, bool bSRGB = false);
std::shared_ptr<GTexture2D> LoadTextureFromAssetFile(const std::string& FilePath);
std::shared_ptr<GTexture2D> CreateTextureFromImageFileAndSave(const std::string& FilePath, bool bSRGB = false);

std::shared_ptr<GMaterial> CreateMaterialFromCode(const std::wstring& CodePathIn);

std::shared_ptr<GGeomertry> CreateDefualCubeGeo();
std::shared_ptr<GGeomertry> CreateDefualtSphereGeo(float Radius, uint32 SliceCount, uint32 StackCount);
std::shared_ptr<GGeomertry> CreateDefualtQuadGeo();

std::shared_ptr<GGeomertry> TempCreateCubeGeoWithMat();
std::shared_ptr<GGeomertry> TempCreateSphereGeoWithMat();
std::shared_ptr<GGeomertry> TempCreateQuadGeoWithMat();
static std::shared_ptr<GMaterialInstance> GetDefaultmaterialIns();


