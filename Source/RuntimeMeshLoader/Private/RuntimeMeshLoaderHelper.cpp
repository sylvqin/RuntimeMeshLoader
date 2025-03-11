#include "RuntimeMeshLoaderHelper.h"
#include "MeshLoader.h"
#include "RuntimeMeshLoader.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionMultiply.h"

// Create a simpler basic material function that doesn't directly access material properties
UMaterial* CreateBasicMaterial()
{
    // Create a new material
    UMaterial* NewMaterial = NewObject<UMaterial>(GetTransientPackage(), NAME_None, RF_Transient);
    
    // We can't directly manipulate properties in runtime-only mode, so we'll return it as-is
    // These properties will be set via the material instance dynamic instead
    
    return NewMaterial;
}

bool URuntimeMeshLoaderHelper::LoadMeshWithTextures(UProceduralMeshComponent* ProceduralMeshComponent, 
                                                 FString FilePath, 
                                                 EPathType Type,
                                                 FVector Scale,
                                                 bool bClearMesh)
{
    if (!ProceduralMeshComponent)
    {
        UE_LOG(LogRuntimeMeshLoader, Error, TEXT("LoadMeshWithTextures: Invalid ProceduralMeshComponent"));
        return false;
    }

    // Check for zero scale values
    if (FMath::IsNearlyZero(Scale.X))
    {
        UE_LOG(LogRuntimeMeshLoader, Warning, TEXT("LoadMeshWithTextures: Scale.X is nearly zero, setting to 0.01"));
        Scale.X = 0.01f;
    }
    if (FMath::IsNearlyZero(Scale.Y))
    {
        UE_LOG(LogRuntimeMeshLoader, Warning, TEXT("LoadMeshWithTextures: Scale.Y is nearly zero, setting to 0.01"));
        Scale.Y = 0.01f;
    }
    if (FMath::IsNearlyZero(Scale.Z))
    {
        UE_LOG(LogRuntimeMeshLoader, Warning, TEXT("LoadMeshWithTextures: Scale.Z is nearly zero, setting to 0.01"));
        Scale.Z = 0.01f;
    }

    // Set scale on the procedural mesh component
    ProceduralMeshComponent->SetRelativeScale3D(Scale);
    
    // Note: In UE5.5, we'll handle two-sided rendering at the material level instead of the component level

    // Load the mesh
    FFinalReturnData ReturnData = UMeshLoader::LoadMeshFromFile(FilePath, Type);
    if (!ReturnData.Success)
    {
        UE_LOG(LogRuntimeMeshLoader, Error, TEXT("LoadMeshWithTextures: Failed to load mesh from %s"), *FilePath);
        return false;
    }

    // Clear existing mesh if requested
    if (bClearMesh)
    {
        ProceduralMeshComponent->ClearAllMeshSections();
    }

    // Load textures
    FString BaseFilePath = FilePath;
    FString BaseName = FPaths::GetBaseFilename(BaseFilePath);
    FString Directory = FPaths::GetPath(BaseFilePath);
    
    FString TexturePath = FPaths::Combine(Directory, BaseName + TEXT("_T.png"));
    FString NormalPath = FPaths::Combine(Directory, BaseName + TEXT("_N.png"));
    
    bool bIsTextureValid = false;
    bool bIsNormalValid = false;
    int32 Width = 0, Height = 0;
    
    UTexture2D* DiffuseTexture = UMeshLoader::LoadTexture2DFromFile(TexturePath, bIsTextureValid, Width, Height);
    UTexture2D* NormalTexture = UMeshLoader::LoadTexture2DFromFile(NormalPath, bIsNormalValid, Width, Height);
    
    // Create materials
    UMaterialInstanceDynamic* Material = nullptr;
    
    if (bIsTextureValid || bIsNormalValid)
    {
        Material = CreateMaterialWithTextures(DiffuseTexture, NormalTexture);
    }
    else
    {
        // Create a basic material if no textures were found
        UMaterial* BaseMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
        if (!BaseMaterial)
        {
            BaseMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
        }
        
        if (BaseMaterial)
        {
            Material = UMaterialInstanceDynamic::Create(BaseMaterial, nullptr);
            Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.5f, 0.5f, 0.5f));
            
            // Try to enable two-sided rendering via material parameters
            Material->SetScalarParameterValue("TwoSided", 1.0f);
            Material->SetScalarParameterValue("Double_Sided", 1.0f);
        }
    }

    // Add mesh sections
    int32 SectionIdx = 0;
    for (const FNodeData& Node : ReturnData.Nodes)
    {
        for (const FMeshData& MeshData : Node.Meshes)
        {
            // Skip empty meshes
            if (MeshData.Vertices.Num() == 0 || MeshData.Triangles.Num() == 0)
            {
                continue;
            }
            
            ProceduralMeshComponent->CreateMeshSection(
                SectionIdx,
                MeshData.Vertices,
                MeshData.Triangles,
                MeshData.Normals,
                MeshData.UVs,
                TArray<FColor>(),
                MeshData.Tangents,
                true // Create collision
            );
            
            if (Material)
            {
                ProceduralMeshComponent->SetMaterial(SectionIdx, Material);
            }
            
            SectionIdx++;
        }
    }
    
    return true;
}

UMaterialInstanceDynamic* URuntimeMeshLoaderHelper::CreateMaterialWithTextures(UTexture2D* DiffuseTexture, UTexture2D* NormalTexture)
{
    // First, try to find a known two-sided material in the engine
    static const TCHAR* TwoSidedMaterials[] = {
        TEXT("/Engine/EngineMaterials/DefaultLitMaterial.DefaultLitMaterial"), // Try DefaultLitMaterial first
        TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"),
        TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial"),
        TEXT("/Engine/EngineMaterials/DefaultPhysicalMaterial.DefaultPhysicalMaterial"),
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")
    };

    UMaterial* BaseMaterial = nullptr;
    
    // Try each material in order until we find one
    for (const TCHAR* MaterialPath : TwoSidedMaterials)
    {
        BaseMaterial = LoadObject<UMaterial>(nullptr, MaterialPath);
        if (BaseMaterial)
        {
            UE_LOG(LogRuntimeMeshLoader, Log, TEXT("Found base material: %s"), MaterialPath);
            break;
        }
    }
    
    // If no material was found, log an error and return nullptr
    if (!BaseMaterial)
    {
        UE_LOG(LogRuntimeMeshLoader, Error, TEXT("CreateMaterialWithTextures: Failed to find any suitable base material"));
        return nullptr;
    }
    
    // Create a dynamic material instance
    UMaterialInstanceDynamic* MaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial, nullptr);
    if (!MaterialInstance)
    {
        UE_LOG(LogRuntimeMeshLoader, Error, TEXT("CreateMaterialWithTextures: Failed to create material instance"));
        return nullptr;
    }
    
    // Set the texture parameters if we have them
    if (DiffuseTexture)
    {
        // Try all common parameter names for diffuse textures
        const TCHAR* DiffuseTextureNames[] = {
            TEXT("Diffuse"),
            TEXT("DiffuseTexture"),
            TEXT("BaseColor"),
            TEXT("Color"),
            TEXT("Albedo"),
            TEXT("GridTexture"),
            TEXT("MainTexture"),
            TEXT("Texture"),
            TEXT("DiffuseMap"),
            TEXT("BaseColorMap") // Common in PBR materials
        };
        
        for (const TCHAR* ParamName : DiffuseTextureNames)
        {
            MaterialInstance->SetTextureParameterValue(ParamName, DiffuseTexture);
        }
        
        // Set bright base color to ensure the material isn't too dark
        // Use white or light gray as default color multiplier
        const TCHAR* ColorParamNames[] = {
            TEXT("DiffuseColor"),
            TEXT("BaseColorTint"),
            TEXT("ColorTint"),
            TEXT("TintColor"),
            TEXT("Color")
        };
        
        for (const TCHAR* ParamName : ColorParamNames)
        {
            MaterialInstance->SetVectorParameterValue(ParamName, FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
        }
        
        // Set scalar parameters that might affect texture display
        MaterialInstance->SetScalarParameterValue("TextureScale", 1.0f);
        MaterialInstance->SetScalarParameterValue("UVScale", 1.0f);
        
        // Fix UV scaling issues - try different common UV scale parameters
        const TCHAR* UVScaleParams[] = {
            TEXT("UVScale"),
            TEXT("TextureScale"),
            TEXT("UVTiling"),
            TEXT("Tiling"),
            TEXT("UVMultiplier")
        };
        
        for (const TCHAR* ParamName : UVScaleParams)
        {
            // Try a few different UV scales to see if one works better
            MaterialInstance->SetScalarParameterValue(ParamName, 1.0f);
            
            // Also try vector param version for X/Y separate scaling
            MaterialInstance->SetVectorParameterValue(ParamName, FLinearColor(1.0f, 1.0f, 0.0f, 0.0f));
        }
    }
    
    if (NormalTexture)
    {
        // Try all common parameter names for normal maps
        const TCHAR* NormalTextureNames[] = {
            TEXT("Normal"),
            TEXT("NormalTexture"),
            TEXT("NormalMap"),
            TEXT("Normals")
        };
        
        for (const TCHAR* ParamName : NormalTextureNames)
        {
            MaterialInstance->SetTextureParameterValue(ParamName, NormalTexture);
        }
        
        // Set normal intensity parameters to make the lighting more pronounced
        const TCHAR* NormalIntensityParams[] = {
            TEXT("NormalStrength"),
            TEXT("NormalIntensity"),
            TEXT("BumpScale"),
            TEXT("NormalScale")
        };
        
        for (const TCHAR* ParamName : NormalIntensityParams)
        {
            MaterialInstance->SetScalarParameterValue(ParamName, 1.0f);
        }
    }
    
    // Try to enable two-sided rendering via material parameters using common parameter names
    const TCHAR* TwoSidedParamNames[] = {
        TEXT("TwoSided"),
        TEXT("IsTwoSided"),
        TEXT("Double_Sided"),
        TEXT("DoubleSided"),
        TEXT("bTwoSided"),
        TEXT("Two_Sided")
    };
    
    for (const TCHAR* ParamName : TwoSidedParamNames)
    {
        MaterialInstance->SetScalarParameterValue(ParamName, 1.0f);
    }
    
    // Set additional material parameters that improve appearance and lighting
    MaterialInstance->SetScalarParameterValue("Roughness", 0.5f);  // Less roughness for more specularity
    MaterialInstance->SetScalarParameterValue("Metallic", 0.0f);   // No metallic for most materials
    MaterialInstance->SetScalarParameterValue("Specular", 0.5f);   // Increased specular for better highlights
    
    // Set the material to be more receptive to lighting
    const TCHAR* LightingParams[] = {
        TEXT("EmissiveScale"),
        TEXT("EmissiveIntensity"),
        TEXT("EmissiveBrightness"),
        TEXT("Emissive")
    };
    
    for (const TCHAR* ParamName : LightingParams)
    {
        // Add slight emissive to brighten up the material
        MaterialInstance->SetScalarParameterValue(ParamName, 0.2f);
        MaterialInstance->SetVectorParameterValue(ParamName, FLinearColor(0.2f, 0.2f, 0.2f, 1.0f));
    }
    
    return MaterialInstance;
} 