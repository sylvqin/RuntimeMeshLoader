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
	
	// Vertices
	for(unsigned int i = 0; i < Mesh->mNumVertices; i++)
	{	
		// Process vertex positions, normals, and UVs
		FVector Vertex;
		Vertex.X = Mesh->mVertices[i].x;
		Vertex.Y = Mesh->mVertices[i].y;
		Vertex.Z = Mesh->mVertices[i].z;
		MeshData.Vertices.Add(Vertex);
		
		// Normals
		if(Mesh->HasNormals())
		{
			FVector Normal;
			Normal.X = Mesh->mNormals[i].x;
			Normal.Y = Mesh->mNormals[i].y;
			Normal.Z = Mesh->mNormals[i].z;
			MeshData.Normals.Add(Normal);
		}
		else 
		{
            // Add a default normal if none exists
            MeshData.Normals.Add(FVector(0.0f, 0.0f, 1.0f));
		}
		
		// Texture Coordinates
		if(Mesh->HasTextureCoords(0))  // Check if the mesh contains texture coordinates
		{
			// UVs might need adjusting depending on how the texture looks
			FVector2D UV;
			
			// Standard UV mapping from Assimp
			UV.X = Mesh->mTextureCoords[0][i].x;
			UV.Y = 1.0f - Mesh->mTextureCoords[0][i].y; // Flip V coordinate for UE (1.0 - v)
			
			// Ensure UVs are properly normalized to 0-1 range
			if (UV.X < 0.0f) UV.X = 0.0f;
			if (UV.X > 1.0f) UV.X = 1.0f;
			if (UV.Y < 0.0f) UV.Y = 0.0f;
			if (UV.Y > 1.0f) UV.Y = 1.0f;
			
			MeshData.UVs.Add(UV);
		}
		else
		{
			// If no texture coordinates are available, use a default UV mapping
			MeshData.UVs.Add(FVector2D(0.0f, 0.0f));
		}
		
		// Tangents
		if(Mesh->HasTangentsAndBitangents())
		{
			FVector Tangent;
			Tangent.X = Mesh->mTangents[i].x;
			Tangent.Y = Mesh->mTangents[i].y;
			Tangent.Z = Mesh->mTangents[i].z;
			
			FVector Bitangent;
			Bitangent.X = Mesh->mBitangents[i].x;
			Bitangent.Y = Mesh->mBitangents[i].y;
			Bitangent.Z = Mesh->mBitangents[i].z;
			
			// Calculate the handedness value for the tangent by determining the sign of the cross product
			float TangentW = (FVector::CrossProduct(Tangent, Bitangent).Dot(FVector(Mesh->mNormals[i].x, Mesh->mNormals[i].y, Mesh->mNormals[i].z)) < 0.0f) ? -1.0f : 1.0f;
			
			MeshData.Tangents.Add(FProcMeshTangent(Tangent, TangentW < 0.0f));
		}
		else
		{
			// Add default tangent if none exists
			MeshData.Tangents.Add(FProcMeshTangent(FVector(1.0f, 0.0f, 0.0f), false));
		}
	}
	
	// Process indices (faces)
	for(unsigned int i = 0; i < Mesh->mNumFaces; i++)
	{
		aiFace Face = Mesh->mFaces[i];
		for(unsigned int j = 0; j < Face.mNumIndices; j++)
		{
			MeshData.Triangles.Add(Face.mIndices[j]);
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

	// Check for texture files
	FString BaseName = FPaths::GetBaseFilename(FilePath);
	FString Directory = FPaths::GetPath(FilePath);
	
	FString TexturePath = FPaths::Combine(Directory, BaseName + TEXT("_T.png"));
	FString NormalPath = FPaths::Combine(Directory, BaseName + TEXT("_N.png"));
	
	UE_LOG(LogRuntimeMeshLoader, Log, TEXT("Looking for texture at: %s (Exists: %s)"), 
		*TexturePath, FPaths::FileExists(TexturePath) ? TEXT("Yes") : TEXT("No"));
	UE_LOG(LogRuntimeMeshLoader, Log, TEXT("Looking for normal map at: %s (Exists: %s)"), 
		*NormalPath, FPaths::FileExists(NormalPath) ? TEXT("Yes") : TEXT("No"));

	// Continue with existing code for loading the mesh
	try
	{
		Assimp::Importer Importer;
		
		// Configure importer for better UV handling
		Importer.SetPropertyInteger(AI_CONFIG_PP_PTV_NORMALIZE, 1); // Normalize UVs
		
		// Modified processing flags to fix texture mapping issues
		unsigned int Flags = 
			aiProcess_Triangulate | 		// Convert all shapes to triangles
			aiProcess_MakeLeftHanded |  	// Convert to UE coordinate system
			aiProcess_CalcTangentSpace | 	// Create tangents
			aiProcess_GenSmoothNormals | 	// Generate smooth normals
			aiProcess_OptimizeMeshes | 		// Join similar meshes
			aiProcess_ImproveCacheLocality | // Improve memory access for vertices
			aiProcess_RemoveRedundantMaterials | // Remove duplicate materials
			aiProcess_FixInfacingNormals;    // Fix normals pointing inward
			
		// Don't flip UVs if the texture appears incorrect
		// aiProcess_FlipUVs |
		
		// Load the scene
		const aiScene* Scene = Importer.ReadFile(TCHAR_TO_ANSI(*FilePath), Flags);

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
	UTexture2D* LoadedTexture = nullptr;

	// Check if file exists
	if (!FPaths::FileExists(FullFilePath))
	{
		UE_LOG(LogRuntimeMeshLoader, Error, TEXT("Texture file not found: %s"), *FullFilePath);
		return nullptr;
	}

	UE_LOG(LogRuntimeMeshLoader, Log, TEXT("Attempting to load texture from: %s"), *FullFilePath);

	// Load the compressed bytes
	TArray<uint8> FileData;
	if (!FFileHelper::LoadFileToArray(FileData, *FullFilePath))
	{
		UE_LOG(LogRuntimeMeshLoader, Error, TEXT("Failed to load texture file to array: %s"), *FullFilePath);
		return nullptr;
	}

	// Get an image wrapper for the file type
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	// Detect the image type using the file extension
	EImageFormat DetectedFormat = EImageFormat::Invalid;
	
	if (FullFilePath.EndsWith(".png", ESearchCase::IgnoreCase))
	{
		DetectedFormat = EImageFormat::PNG;
	}
	else if (FullFilePath.EndsWith(".jpg", ESearchCase::IgnoreCase) || FullFilePath.EndsWith(".jpeg", ESearchCase::IgnoreCase))
	{
		DetectedFormat = EImageFormat::JPEG;
	}
	else if (FullFilePath.EndsWith(".bmp", ESearchCase::IgnoreCase))
	{
		DetectedFormat = EImageFormat::BMP;
	}
	
	if (DetectedFormat == EImageFormat::Invalid)
	{
		UE_LOG(LogRuntimeMeshLoader, Error, TEXT("Unsupported texture format for file: %s"), *FullFilePath);
		return nullptr;
	}

	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(DetectedFormat);

	if (!ImageWrapper.IsValid())
	{
		UE_LOG(LogRuntimeMeshLoader, Error, TEXT("Failed to create image wrapper for format: %d"), (int32)DetectedFormat);
		return nullptr;
	}

	if (!ImageWrapper->SetCompressed(FileData.GetData(), FileData.Num()))
	{
		UE_LOG(LogRuntimeMeshLoader, Error, TEXT("Failed to set compressed data for image: %s"), *FullFilePath);
		return nullptr;
	}

	// Get the raw RGB data
	TArray<uint8> RawData;
	if (!ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, RawData))
	{
		UE_LOG(LogRuntimeMeshLoader, Error, TEXT("Failed to get raw image data: %s"), *FullFilePath);
		return nullptr;
	}

	// Create the texture
	Width = ImageWrapper->GetWidth();
	Height = ImageWrapper->GetHeight();

	UE_LOG(LogRuntimeMeshLoader, Log, TEXT("Texture dimensions: %d x %d"), Width, Height);

	LoadedTexture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);

	if (!LoadedTexture)
	{
		UE_LOG(LogRuntimeMeshLoader, Error, TEXT("Failed to create transient texture"));
		return nullptr;
	}

	// Lock the texture for mip-level 0
	void* TextureData = LoadedTexture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(TextureData, RawData.GetData(), RawData.Num());
	LoadedTexture->GetPlatformData()->Mips[0].BulkData.Unlock();

	// Update the texture
	LoadedTexture->UpdateResource();

	IsValid = true;
	UE_LOG(LogRuntimeMeshLoader, Log, TEXT("Successfully loaded texture: %s"), *FullFilePath);
	return LoadedTexture;
}
