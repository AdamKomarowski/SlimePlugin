#pragma once
 
#include "ModuleManager.h"
 
class SlimePluginImpl : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	void StartupModule();
	void ShutdownModule();
};
