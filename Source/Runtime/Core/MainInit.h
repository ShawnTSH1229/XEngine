#pragma once
#include <vector>
class XReflectionInfo;

typedef void (*InitPropertyFunPtr)(XReflectionInfo*);

class MainInit
{
public:
	static void PushToInitPropertyFunArray(InitPropertyFunPtr);
	static void Init();
	static void InitAfterRHI();
	static void Destroy();
};