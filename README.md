# RuntimeMeshLoader

A plugin for Unreal Engine 5.5, which allows importing meshes during runtime.

This is a fork of [Chrizey91's project](https://github.com/Chrizey91/RuntimeMeshLoader) which is a fork of [RuntimeMeshLoader repository](https://github.com/GameInstitute/RuntimeMeshLoader), now updated for compatibility with Unreal Engine 5.5.

RuntimeMeshLoader (RML) uses [Assimp](https://github.com/assimp/assimp) as a third-party library to handle most of the import-stuff.
 
## Table of contents
* [Supported operating systems](#supported-operating-systems)
* [Supported Unreal Engine versions](#supported-unreal-engine-versions)
* [Installation](#installation)
* [DLL Management](#dll-management)
* [How-to](#how-to)
* [Troubleshooting](#troubleshooting)
* [Limitations](#limitations)

## Supported operating systems
Currently, Windows (x64) has been verified (used to work on Linux according to @Chrizey91)

## Supported Unreal Engine versions
The Plugin has been tested and is compatible with Unreal Engine 5.5.

## Installation 

1. Download this repository, by clicking on the green `Code`-Button in the upper right hand corner and select `Download ZIP`.

2. In your Unreal Engine project open the `Plugins` directory. If it does not exist, simply create a new folder in the root directory of your project and call it `Plugins`.

3. Copy the contents of the downloaded ZIP-folder into it. The folder might have a name such as `RuntimeMeshLoader-main`. If that is the case, rename it to `RuntimeMeshLoader`.

4. Afterwards, load up your project. Unreal Engine might tell you it needs to rebuild the Plugin. Simply click on `Yes`. After the rebuild process your project should load as usual.

5. In your project's `source panel` you should now have a folder labeled `RuntimeMeshLoader Content`. Inside you will find an example map, which will show you how to use the plugin with an example.

## DLL Management

The plugin relies on the Assimp library, which is distributed as a DLL file. For the plugin to work correctly, this DLL must be properly installed in your project. The plugin includes a utility script `CopyDLL.bat` to help with this process:

1. The script automatically copies the Assimp DLL (`assimp-vc142-mt.dll`) from the plugin's `ThirdParty/assimp/bin` directory to:
   - The plugin's `Binaries/Win64` directory
   - Your project's `Binaries/Win64` directory

2. This script runs automatically when the plugin is built through the UE5.5 build system if you're using the full source version of the plugin.

3. If you encounter DLL-related errors, you can manually run this script:
   - Navigate to the `Plugins/RuntimeMeshLoader` directory
   - Right-click on `CopyDLL.bat` and select "Run as administrator"

The plugin also contains code to handle various DLL loading situations and will attempt to find the DLL in different locations if the primary location fails.

## How-to
The plugin can be used in many ways. You could create a file-dialogue for the user to select assets to be loaded, or load assets from a pre-defined folder, or any other creative way.

In the example map, a folder called `RuntimeMeshLoader` is created in the user's `Documents` folder. Inside this folder, the plugin expects for each asset to be loaded a separate folder, in which 3 files should be located. `<folder name>.fbx`, `<folder name>_T.png` and `<folder name>_N.png`, which are the mesh itself as an fbx-file, its texture and its normal map as PNG-files.

If you load up the plugin for the first time, the folder will be empty. As an example, you can copy the two provided mesh-folders inside the `Resources` directory into the aforementioned `RuntimeMeshLoader`-folder.

### Basic Blueprint Usage

1. Create a blueprint and add a function call to `LoadMeshFromFile`
2. Provide the file path to your 3D model
3. Use the returned mesh data to create or update procedural mesh components

## Troubleshooting

### Common Issues

1. **Plugin compilation fails**: 
   - Make sure you have Visual Studio installed with C++ tools
   - Ensure all necessary dependencies are installed for UE 5.5
   - Delete the `Intermediate` and `Binaries` folders from the plugin directory and rebuild

2. **DLL not found errors**:
   - Run the `CopyDLL.bat` script manually
   - Verify that `assimp-vc142-mt.dll` exists in both the plugin's and project's `Binaries/Win64` directories
   - Check that no other conflicting versions of the Assimp DLL are in your system path

3. **Models fail to load or crash**:
   - Check the Output Log in Unreal Engine for specific error messages
   - Try preprocessing your model files by opening them in Blender and re-exporting them
   - Verify that the model file format is supported by Assimp

4. **UE5.5 compatibility issues**:
   - This version has been specifically updated for UE 5.5, but if you encounter API-related errors, check the Unreal Engine release notes for any deprecated functions

## Limitations
- Only PNG-formats can be loaded for textures. JPG is currently not supported.
- In the example map, only fbx-files are supported. However, the plugin is already capable of loading many more 3D-file formats supported by Assimp.
- Some fbx-files downloaded from the internet may cause issues. Usually, opening it up in Blender and exporting it again solves this problem.
- Complex models with extensive animations or advanced material properties may not import correctly.
