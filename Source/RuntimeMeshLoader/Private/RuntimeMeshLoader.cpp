// Copyright Epic Games, Inc. All Rights Reserved.

#include "RuntimeMeshLoader.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "GenericPlatform/GenericPlatformOutputDevices.h"
#include "Logging/LogMacros.h"
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

DEFINE_LOG_CATEGORY(LogRuntimeMeshLoader);

#define LOCTEXT_NAMESPACE "FRuntimeMeshLoaderModule"

void FRuntimeMeshLoaderModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	UE_LOG(LogRuntimeMeshLoader, Log, TEXT("RuntimeMeshLoader: Module startup"));
	
	// Get the base directory of this plugin
	FString BaseDir = IPluginManager::Get().FindPlugin("RuntimeMeshLoader")->GetBaseDir();
	
	// Add on the relative location of the third party dll and load it
	TArray<FString> PossibleDllPaths;

#if PLATFORM_WINDOWS
	PossibleDllPaths.Add(FPaths::Combine(*BaseDir, TEXT("ThirdParty/assimp/bin/assimp-vc142-mt.dll")));
	PossibleDllPaths.Add(FPaths::Combine(*BaseDir, TEXT("Binaries/Win64/assimp-vc142-mt.dll")));
	PossibleDllPaths.Add(FPaths::Combine(FPaths::ProjectDir(), TEXT("Binaries/Win64/assimp-vc142-mt.dll")));

	// Try each possible path
	for (const FString& DllPath : PossibleDllPaths)
	{
		if (FPaths::FileExists(DllPath))
		{
			UE_LOG(LogRuntimeMeshLoader, Log, TEXT("RuntimeMeshLoader: Attempting to load Assimp DLL from: %s"), *DllPath);
			DllHandle = FPlatformProcess::GetDllHandle(*DllPath);
			if (DllHandle)
			{
				UE_LOG(LogRuntimeMeshLoader, Log, TEXT("RuntimeMeshLoader: Successfully loaded Assimp DLL from: %s"), *DllPath);
				break;
			}
			else
			{
				UE_LOG(LogRuntimeMeshLoader, Warning, TEXT("RuntimeMeshLoader: Failed to load Assimp DLL from: %s"), *DllPath);
			}
		}
		else
		{
			UE_LOG(LogRuntimeMeshLoader, Warning, TEXT("RuntimeMeshLoader: DLL path does not exist: %s"), *DllPath);
		}
	}

	if (!DllHandle)
	{
		UE_LOG(LogRuntimeMeshLoader, Error, TEXT("RuntimeMeshLoader: Failed to load Assimp DLL from any location. The plugin may not function correctly."));
	}

	// Set up a search path for the DLL so the system can find it
	for (const FString& DllPath : PossibleDllPaths)
	{
		if (FPaths::FileExists(DllPath))
		{
			FString DllDir = FPaths::GetPath(DllPath);
			FPlatformProcess::AddDllDirectory(*DllDir);
			UE_LOG(LogRuntimeMeshLoader, Log, TEXT("RuntimeMeshLoader: Added DLL directory to search path: %s"), *DllDir);
		}
	}
#endif
}

void FRuntimeMeshLoaderModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	UE_LOG(LogRuntimeMeshLoader, Log, TEXT("RuntimeMeshLoader: Module shutdown"));

	// Free the dll handle
	if (DllHandle)
	{
		FPlatformProcess::FreeDllHandle(DllHandle);
		DllHandle = nullptr;
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FRuntimeMeshLoaderModule, RuntimeMeshLoader)