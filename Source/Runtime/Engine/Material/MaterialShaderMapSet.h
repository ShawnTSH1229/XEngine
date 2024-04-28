#pragma once
#include <vector>
class XShaderInfo;
struct XMaterialShaderParameters_ForIndex;
class XMaterialShaderMapSet
{
public:
	std::vector<XShaderInfo*>ShaderInfos;
};

//Only Has Base Pass Shader Map Set
const XMaterialShaderMapSet& GetMaterialShaderMapSet();