#pragma once

#include "CoreMinimal.h"
#include "MeshLoader.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"
#include "RuntimeMeshLoaderHelper.generated.h"

/**
 * Helper utility for working with RuntimeMeshLoader
 * Provides simplified functions for common operations
 */
UCLASS(BlueprintType, Blueprintable)
class RUNTIMEMESHLOADER_API URuntimeMeshLoaderHelper : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Load a mesh from file with automatic texture loading
     * 
     * @param ProceduralMeshComponent - The procedural mesh component to populate
     * @param FilePath - Path to the mesh file (.fbx, .obj, etc.)
     * @param Type - Whether the path is absolute or relative
     * @param Scale - Scale to apply to the mesh
     * @param bClearMesh - Whether to clear the mesh before adding sections
     * @return bool - True if successful
     */
    UFUNCTION(BlueprintCallable, Category = "RuntimeMeshLoader|Helper")
    static bool LoadMeshWithTextures(UProceduralMeshComponent* ProceduralMeshComponent, 
                                   FString FilePath, 
                                   EPathType Type = EPathType::Absolute,
                                   FVector Scale = FVector(1.0f, 1.0f, 1.0f),
                                   bool bClearMesh = true);

    /**
     * Creates a material instance with the texture and normal map
     * 
     * @param DiffuseTexture - The diffuse/albedo texture
     * @param NormalTexture - The normal map texture
     * @return UMaterialInstanceDynamic* - The created material instance
     */
    UFUNCTION(BlueprintCallable, Category = "RuntimeMeshLoader|Helper")
    static UMaterialInstanceDynamic* CreateMaterialWithTextures(UTexture2D* DiffuseTexture, UTexture2D* NormalTexture);
}; 