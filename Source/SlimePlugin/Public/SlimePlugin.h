#pragma once
 
#include "Modules/ModuleManager.h"
 
class SlimePluginImpl : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	void StartupModule();
	void ShutdownModule();
};
