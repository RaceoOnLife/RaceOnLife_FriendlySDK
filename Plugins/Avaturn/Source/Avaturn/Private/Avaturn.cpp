// Copyright © 2023++ Avaturn

#include "Avaturn.h"
#include "AvaturnTypes.h"

DEFINE_LOG_CATEGORY(LogAvaturn);

#define LOCTEXT_NAMESPACE "FAvaturnModule"

void FAvaturnModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FAvaturnModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAvaturnModule, Avaturn)