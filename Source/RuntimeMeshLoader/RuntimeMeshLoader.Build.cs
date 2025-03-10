// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class RuntimeMeshLoader : ModuleRules
{
	public RuntimeMeshLoader(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        var thirdPartyPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", "ThirdParty"));
        
        // Enable C++ exceptions for Assimp
        bEnableExceptions = true;
        
        // Add UE version compatibility defines
        PublicDefinitions.Add("WITH_UE5=1");
        
        // For UE version-specific code
        if (Target.Version.MajorVersion >= 5)
        {
            PublicDefinitions.Add("WITH_UE_5_0=1");
            
            if (Target.Version.MinorVersion >= 1)
                PublicDefinitions.Add("WITH_UE_5_1=1");
            else
                PublicDefinitions.Add("WITH_UE_5_1=0");
            
            if (Target.Version.MinorVersion >= 2)
                PublicDefinitions.Add("WITH_UE_5_2=1");
            else
                PublicDefinitions.Add("WITH_UE_5_2=0");
                
            if (Target.Version.MinorVersion >= 3)
                PublicDefinitions.Add("WITH_UE_5_3=1");
            else
                PublicDefinitions.Add("WITH_UE_5_3=0");
                
            if (Target.Version.MinorVersion >= 4)
                PublicDefinitions.Add("WITH_UE_5_4=1");
            else
                PublicDefinitions.Add("WITH_UE_5_4=0");
                
            if (Target.Version.MinorVersion >= 5)
                PublicDefinitions.Add("WITH_UE_5_5=1");
            else
                PublicDefinitions.Add("WITH_UE_5_5=0");
        }
        else
        {
            PublicDefinitions.Add("WITH_UE_5_0=0");
            PublicDefinitions.Add("WITH_UE_5_1=0");
            PublicDefinitions.Add("WITH_UE_5_2=0");
            PublicDefinitions.Add("WITH_UE_5_3=0");
            PublicDefinitions.Add("WITH_UE_5_4=0");
            PublicDefinitions.Add("WITH_UE_5_5=0");
        }

		PublicIncludePaths.AddRange(
			new string[] {
                Path.Combine(thirdPartyPath, "assimp", "include")
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "ProceduralMeshComponent",
                "RenderCore",
                "Projects"  // Required for IPluginManager
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"Projects",
				"ImageWrapper",
                "RHI"
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);

		// Add the Assimp DLL delay-load dependency
		PublicDelayLoadDLLs.Add("assimp-vc142-mt.dll");

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
			string AssimpLibPath = Path.Combine(thirdPartyPath, "assimp", "lib", "assimp-vc142-mt.lib");
			string AssimpDllPath = Path.Combine(thirdPartyPath, "assimp", "bin", "assimp-vc142-mt.dll");
			
			// Log information about the path
			System.Console.WriteLine("Assimp Library Path: " + AssimpLibPath);
			System.Console.WriteLine("Assimp DLL Path: " + AssimpDllPath);
			
			// Check if files exist
			if (File.Exists(AssimpLibPath))
			{
				System.Console.WriteLine("Assimp LIB file exists");
				PublicAdditionalLibraries.Add(AssimpLibPath);
			}
			else
			{
				System.Console.WriteLine("WARNING: Assimp LIB file not found at path: " + AssimpLibPath);
			}
			
			if (File.Exists(AssimpDllPath))
			{
				System.Console.WriteLine("Assimp DLL file exists");
				
				// Add the DLL as a runtime dependency so it gets copied to the output directory
				RuntimeDependencies.Add(AssimpDllPath);
				
				// Also copy to the binaries directory
				string BinariesDllPath = Path.Combine("$(PluginDir)", "Binaries", "ThirdParty", "assimp", "bin", "assimp-vc142-mt.dll");
				RuntimeDependencies.Add(BinariesDllPath, AssimpDllPath, StagedFileType.NonUFS);
                
                // Also copy to the project binaries directory as a fallback
                string ProjectBinPath = Path.Combine("$(ProjectDir)", "Binaries", "Win64", "assimp-vc142-mt.dll");
                RuntimeDependencies.Add(ProjectBinPath, AssimpDllPath, StagedFileType.NonUFS);
			}
			else
			{
				System.Console.WriteLine("WARNING: Assimp DLL file not found at path: " + AssimpDllPath);
			}
        }
		
		if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			var AssimpLib = Path.Combine(ModuleDirectory, "..", "..", "ThirdParty", "assimp", "lib", "libassimp.so");

			if (File.Exists(AssimpLib))
			{
				System.Console.WriteLine("Assimp Linux library exists");
				PublicAdditionalLibraries.Add(AssimpLib);
				RuntimeDependencies.Add(AssimpLib);
			}
			else
			{
				System.Console.WriteLine("WARNING: Assimp Linux library not found at path: " + AssimpLib);
			}
		}
	}
}
