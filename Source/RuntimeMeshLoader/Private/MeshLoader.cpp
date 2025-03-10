// Fill out your copyright notice in the Description page of Project Settings.


#include "MeshLoader.h"
#include "RuntimeMeshLoader.h"
#include "Interfaces/IPluginManager.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Modules/ModuleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "HAL/FileManager.h"
#include "HAL/FileManagerGeneric.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureDefines.h"
#include "RHI.h"
#include "RenderCore.h"

// TexturePlatformData.h has moved in UE 5.5
#if WITH_UE_5_5
#include "TextureResource.h"
#include "RenderUtils.h"
#else
#if WITH_UE_5_0
#include "TextureResource.h"
#include "RenderUtils.h"
#include "Engine/TexturePlatformData.h"
#endif
#endif

FMeshData ProcessMesh(aiMesh* Mesh, const aiScene* Scene)
{
    FMeshData MeshData;

	for (uint32 j = 0; j < Mesh->mNumVertices; ++j)
	{
		// Create vectors with the appropriate type based on UE version
		FVector Vertex(Mesh->mVertices[j].x, Mesh->mVertices[j].y, Mesh->mVertices[j].z);
		MeshData.Vertices.Add(Vertex);
		
		FVector Normal = FVector::ZeroVector;
		if (Mesh->HasNormals())
		{
		    Normal = FVector(Mesh->mNormals[j].x, Mesh->mNormals[j].y, Mesh->mNormals[j].z);
		}
		MeshData.Normals.Add(Normal);
		
		if (Mesh->mTextureCoords[0])
		{
		#if WITH_UE_5_0
			// UE 5.0+ uses double precision
		    MeshData.UVs.Add(FVector2D(static_cast<double>(Mesh->mTextureCoords[0][j].x), 1.0-static_cast<double>(Mesh->mTextureCoords[0][j].y)));
		#else
			// UE 4.x uses single precision
		    MeshData.UVs.Add(FVector2D(static_cast<float>(Mesh->mTextureCoords[0][j].x), 1.f-static_cast<float>(Mesh->mTextureCoords[0][j].y)));
		#endif
		}

		if (Mesh->HasTangentsAndBitangents())
		{
			FProcMeshTangent Tangent = FProcMeshTangent(Mesh->mTangents[j].x, Mesh->mTangents[j].y, Mesh->mTangents[j].z);
			MeshData.Tangents.Add(Tangent);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("mNumFaces: %d"), Mesh->mNumFaces);
	for (uint32 i = 0; i < Mesh->mNumFaces; i++)
	{
		aiFace Face = Mesh->mFaces[i];
		for (uint32 f = 0; f < Face.mNumIndices; f++)
		{
		    MeshData.Triangles.Add(Face.mIndices[f]);
		}
	}

	return MeshData;
}

void ProcessNode(aiNode* Node, const aiScene* Scene, int ParentNodeIndex, int* CurrentIndex, FFinalReturnData* FinalReturnData)
{
    FNodeData NodeData;
	NodeData.NodeParentIndex = ParentNodeIndex;
	

	aiMatrix4x4 TempTrans = Node->mTransformation;
	FMatrix tempMatrix;
	tempMatrix.M[0][0] = TempTrans.a1; tempMatrix.M[0][1] = TempTrans.b1; tempMatrix.M[0][2] = TempTrans.c1; tempMatrix.M[0][3] = TempTrans.d1;
	tempMatrix.M[1][0] = TempTrans.a2; tempMatrix.M[1][1] = TempTrans.b2; tempMatrix.M[1][2] = TempTrans.c2; tempMatrix.M[1][3] = TempTrans.d2;
	tempMatrix.M[2][0] = TempTrans.a3; tempMatrix.M[2][1] = TempTrans.b3; tempMatrix.M[2][2] = TempTrans.c3; tempMatrix.M[2][3] = TempTrans.d3;
	tempMatrix.M[3][0] = TempTrans.a4; tempMatrix.M[3][1] = TempTrans.b4; tempMatrix.M[3][2] = TempTrans.c4; tempMatrix.M[3][3] = TempTrans.d4;
	NodeData.RelativeTransformTransform = FTransform(tempMatrix);

    for (uint32 n = 0; n < Node->mNumMeshes; n++)
    {
		uint32 MeshIndex = Node->mMeshes[n];
		UE_LOG(LogTemp, Log, TEXT("Loading Mesh at index: %d"), MeshIndex);
        aiMesh* Mesh = Scene->mMeshes[MeshIndex];
		NodeData.Meshes.Add(ProcessMesh(Mesh, Scene));
    }

	FinalReturnData->Nodes.Add(NodeData);

	UE_LOG(LogTemp, Log, TEXT("mNumMeshes: %d, mNumChildren of Node: %d"), Node->mNumMeshes, Node->mNumChildren);
	int CurrentParentIndex = *CurrentIndex;
	for (uint32 n = 0; n < Node->mNumChildren; n++)
	{
		(*CurrentIndex)++;
	    ProcessNode(Node->mChildren[n], Scene, CurrentParentIndex, CurrentIndex, FinalReturnData);
	}
}

FFinalReturnData UMeshLoader::LoadMeshFromFile(FString FilePath, EPathType type)
{
    FFinalReturnData ReturnData;
	ReturnData.Success = false;

	if (FilePath.IsEmpty())
	{
		UE_LOG(LogRuntimeMeshLoader, Warning, TEXT("Runtime Mesh Loader: filepath is empty.\n"));
		return ReturnData;
	}

	// Check if the Assimp module is properly loaded
	FRuntimeMeshLoaderModule& Module = FModuleManager::GetModuleChecked<FRuntimeMeshLoaderModule>("RuntimeMeshLoader");
	if (!Module.DllHandle)
	{
		UE_LOG(LogRuntimeMeshLoader, Error, TEXT("Runtime Mesh Loader: Assimp DLL not loaded! Mesh loading will fail."));
		
		// Try to manually load the DLL from various locations as a last resort
		FString PluginDir = IPluginManager::Get().FindPlugin("RuntimeMeshLoader")->GetBaseDir();
		TArray<FString> PossiblePaths;
		
		// Add potential DLL locations to check
		PossiblePaths.Add(FPaths::Combine(PluginDir, TEXT("ThirdParty/assimp/bin/assimp-vc142-mt.dll")));
		PossiblePaths.Add(FPaths::Combine(PluginDir, TEXT("Binaries/Win64/assimp-vc142-mt.dll")));
		PossiblePaths.Add(FPaths::Combine(FPaths::ProjectDir(), TEXT("Binaries/Win64/assimp-vc142-mt.dll")));
		
		for (const FString& Path : PossiblePaths)
		{
			if (FPaths::FileExists(Path))
			{
				UE_LOG(LogRuntimeMeshLoader, Log, TEXT("Found Assimp DLL at %s, attempting to load..."), *Path);
				Module.DllHandle = FPlatformProcess::GetDllHandle(*Path);
				if (Module.DllHandle)
				{
					UE_LOG(LogRuntimeMeshLoader, Log, TEXT("Successfully loaded Assimp DLL from %s"), *Path);
					break;
				}
			}
		}
		
		// Still no DLL loaded? Return with error
		if (!Module.DllHandle)
		{
			UE_LOG(LogRuntimeMeshLoader, Error, TEXT("Could not find or load Assimp DLL from any location. Mesh loading will fail."));
			return ReturnData;
		}
	}

	UE_LOG(LogRuntimeMeshLoader, Log, TEXT("Runtime Mesh Loader: Loading mesh from %s"), *FilePath);

	if(type == EPathType::Relative){
		FString ProjectContentDir = FPaths::ProjectContentDir();
		FilePath = FPaths::Combine(ProjectContentDir, FilePath);
	}

	UE_LOG(LogRuntimeMeshLoader, Log, TEXT("Runtime Mesh Loader: Absolute path is %s"), *FilePath);

	if (!FPaths::FileExists(FilePath))
	{
		UE_LOG(LogRuntimeMeshLoader, Error, TEXT("Runtime Mesh Loader: File does not exist: %s"), *FilePath);
		return ReturnData;
	}

	try
	{
		Assimp::Importer Importer;
		const aiScene* Scene = Importer.ReadFile(TCHAR_TO_ANSI(*FilePath), 
			aiProcess_Triangulate | 
			aiProcess_MakeLeftHanded |
			aiProcess_CalcTangentSpace |
			aiProcess_FlipUVs |
			aiProcess_FlipWindingOrder);

		if (!Scene || !Scene->HasMeshes())
		{
			UE_LOG(LogRuntimeMeshLoader, Error, TEXT("Runtime Mesh Loader: Failed to load mesh: %s. Error: %s"), 
				*FilePath, 
				UTF8_TO_TCHAR(Importer.GetErrorString()));
			return ReturnData;
		}

		int CurrentIndex = 0;

		ProcessNode(Scene->mRootNode, Scene, -1, &CurrentIndex, &ReturnData);

		ReturnData.Success = true;
	}
	catch (const std::exception& e)
	{
		UE_LOG(LogRuntimeMeshLoader, Error, TEXT("Runtime Mesh Loader: Exception while loading mesh: %s. Error: %s"), 
			*FilePath, 
			UTF8_TO_TCHAR(e.what()));
	}

	return ReturnData;
}

bool UMeshLoader::DirectoryExists(FString DirectoryPath)
{
	return FPaths::DirectoryExists(DirectoryPath);
}

bool UMeshLoader::CreateDirectory(FString DirectoryPath)
{
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	if (!PlatformFile.DirectoryExists(*DirectoryPath))
	{
	    return PlatformFile.CreateDirectoryTree(*DirectoryPath);
	}

	return true;
}

TArray<FString> UMeshLoader::ListFolders(FString DirectoryPath)
{
	TArray<FString> Folders;
	FFileManagerGeneric::Get().FindFilesRecursive(Folders, *DirectoryPath, TEXT("*"), false, true, true);
	return Folders;
}

UTexture2D* UMeshLoader::LoadTexture2DFromFile(const FString& FullFilePath, bool& IsValid, int32& Width, int32& Height)
{
	IsValid = false;
	UTexture2D* LoadedT2D = NULL;
	
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
	
	TArray<uint8> RawFileData;
	if (!FFileHelper::LoadFileToArray(RawFileData, *FullFilePath)) return NULL;
	
	if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(RawFileData.GetData(), RawFileData.Num()))
	{
		TArray<uint8> UncompressedBGRA;
		if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBGRA))
		{
			LoadedT2D = UTexture2D::CreateTransient(ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), PF_B8G8R8A8);
			if (!LoadedT2D) return NULL;
			
			//Out params
			Width = ImageWrapper->GetWidth();
			Height = ImageWrapper->GetHeight();
			
#if WITH_UE_5_5
			// UE 5.5 uses a different API for texture update
			FRHIResourceCreateInfo CreateInfo(TEXT("LoadTexture2DFromFile"));
            FUpdateTextureRegion2D Region(0, 0, 0, 0, Width, Height);
            
            // Create an array with just one region
            TArray<FUpdateTextureRegion2D> Regions;
            Regions.Add(Region);
            
            // Update texture regions
            LoadedT2D->UpdateTextureRegions(
                0,                                  // MipIndex
                1,                                  // Number of regions
                Regions.GetData(),                  // Regions array
                Width * 4,                          // Source pixel data pitch
                4,                                  // Bytes per pixel
                UncompressedBGRA.GetData(),         // Source data
                nullptr                             // Completion callback
            );
#else
  #if WITH_UE_5_0
			// In UE 5.0-5.4, we need to use the GetPlatformData API
			FTexture2DMipMap& Mip = LoadedT2D->GetPlatformData()->Mips[0];
			void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
			FMemory::Memcpy(Data, UncompressedBGRA.GetData(), UncompressedBGRA.Num());
			Mip.BulkData.Unlock();
  #else
			// Legacy UE 4.x code
			void* TextureData = LoadedT2D->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
			FMemory::Memcpy(TextureData, UncompressedBGRA.GetData(), UncompressedBGRA.Num());
			LoadedT2D->PlatformData->Mips[0].BulkData.Unlock();
  #endif
			// Update the texture
			LoadedT2D->UpdateResource();
#endif
		}
	}
	
	// Success!
	IsValid = true;
	return LoadedT2D;
}
