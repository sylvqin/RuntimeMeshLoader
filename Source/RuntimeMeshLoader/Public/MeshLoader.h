// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "ProceduralMeshComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MeshLoader.generated.h"

UENUM(BlueprintType)
enum class EPathType : uint8
{
	Absolute,
	Relative
};

// Get the appropriate vector types based on UE version
#if WITH_UE_5_0
    // UE 5.0+ uses double precision vectors
    #define FVectorCompat FVector3d
    #define FVector2DCompat FVector2D
#else
    // UE 4.x uses single precision vectors
    #define FVectorCompat FVector
    #define FVector2DCompat FVector2D
#endif

USTRUCT(BlueprintType)
struct FMeshData
{
    GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FinalReturnData")
	TArray<FVector> Vertices;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FinalReturnData")
	TArray<int32> Triangles;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FinalReturnData")
	TArray<FVector> Normals;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FinalReturnData")
	TArray<FVector2D> UVs;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FinalReturnData")
	TArray<FProcMeshTangent> Tangents;
    
    // Default constructor to initialize arrays
    FMeshData()
    {
        // Arrays are already default initialized
    }
};

USTRUCT(BlueprintType)
struct FNodeData
{
    GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FinalReturnData")
	FTransform RelativeTransformTransform;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FinalReturnData")
	int NodeParentIndex = -1;  // Initialize with -1 as default

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FinalReturnData")
	TArray<FMeshData> Meshes;
    
    // Default constructor
    FNodeData() : NodeParentIndex(-1)
    {
        // RelativeTransformTransform is already default initialized
        // Meshes array is already default initialized
    }
};

USTRUCT(BlueprintType)
struct FFinalReturnData
{
    GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FinalReturnData")
	bool Success = false;  // Initialize with false as default

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FinalReturnData")
	TArray<FNodeData> Nodes;
    
    // Default constructor
    FFinalReturnData() : Success(false)
    {
        // Nodes array is already default initialized
    }
};

/**
 * 
 */
UCLASS()
class RUNTIMEMESHLOADER_API UMeshLoader : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable,Category="RuntimeMeshLoader")
	static FFinalReturnData LoadMeshFromFile(FString FilePath, EPathType type = EPathType::Absolute);

	UFUNCTION(BlueprintCallable,Category="RuntimeMeshLoader")
	static bool DirectoryExists(FString DirectoryPath);

	UFUNCTION(BlueprintCallable,Category="RuntimeMeshLoader")
	static bool CreateDirectory(FString DirectoryPath);

	UFUNCTION(BlueprintCallable,Category="RuntimeMeshLoader")
	static TArray<FString> ListFolders(FString DirectoryPath);

	UFUNCTION(BlueprintCallable,Category="RuntimeMeshLoader")
	static UTexture2D* LoadTexture2DFromFile(const FString& FullFilePath, bool& IsValid, int32& Width, int32& Height);
};
