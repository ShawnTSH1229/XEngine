#pragma once
#include <string>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/wstringize.hpp>

#define GET_SHADER_PATH(SName) BOOST_PP_CAT(BOOST_PP_CAT(L, BOOST_PP_CAT(BOOST_PP_STRINGIZE(ROOT_DIR_XENGINE), "/Source/Shaders/")),SName)

#define GLOBAL_SHADER_PATH  BOOST_PP_CAT(BOOST_PP_STRINGIZE(ROOT_DIR_XENGINE), "/Source/Shaders/")
class XPath
{
public:
	static const std::wstring& ProjectResourceSavedDir();
	static const std::string& ProjectResourceSavedDirString();
	static const std::wstring& ProjectMaterialSavedDir();
};

