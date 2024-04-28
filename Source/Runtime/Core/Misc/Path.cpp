#include "Path.h"


const std::wstring& XPath::ProjectResourceSavedDir()
{
	static std::wstring SaveDir = BOOST_PP_CAT(L, BOOST_PP_CAT(BOOST_PP_STRINGIZE(ROOT_DIR_XENGINE), "/ContentSave"));
	return SaveDir;
}

const std::string& XPath::ProjectResourceSavedDirString()
{
	static std::string SaveDir = BOOST_PP_CAT(BOOST_PP_STRINGIZE(ROOT_DIR_XENGINE), "/ContentSave");
	return SaveDir;
}

const std::wstring& XPath::ProjectMaterialSavedDir()
{
	static std::wstring SaveDir = BOOST_PP_CAT(L, BOOST_PP_CAT(BOOST_PP_STRINGIZE(ROOT_DIR_XENGINE), "/MaterialShaders"));
	return SaveDir;
}




