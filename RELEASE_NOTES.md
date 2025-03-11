# RuntimeMeshLoader for Unreal Engine 5.5 - v1.1

## Release Notes

This release includes significant improvements to the RuntimeMeshLoader plugin, focusing on better texture handling, compatibility with Unreal Engine 5.5, and improved usability.

### Major Improvements

1. **Enhanced Texture Loading System**
   - Better error handling and detailed logging for texture loading
   - Support for JPEG and BMP formats in addition to PNG
   - Improved texture parameter assignment for materials

2. **New RuntimeMeshLoaderHelper Class**
   - Simplified mesh loading with automatic texture and material handling
   - One-step loading for procedural meshes with proper materials
   - Automatic scale correction to prevent "zero scale" warnings

3. **Unreal Engine 5.5 Compatibility**
   - Updated for full compatibility with UE 5.5 API changes
   - Fixed IPluginManager references and module dependencies
   - Improved DLL loading mechanisms for better reliability

4. **Material System Enhancement**
   - Automatic material creation with fallback options
   - Better texture assignment for standard engine materials
   - Support for normal maps in the material workflow

5. **Code Quality and Documentation**
   - Comprehensive documentation in the README
   - Detailed troubleshooting guides
   - Well-documented code with clear error messages

### Known Limitations

- Complex models with extensive animations may not import correctly
- Some FBX files from the internet may require pre-processing in Blender

### Installation Instructions

1. Close Unreal Engine if it's running
2. Extract the zip file to your project's Plugins folder
3. Open your project - when prompted to rebuild the plugin, select "Yes"
4. Run the CopyDLL.bat script to ensure the Assimp DLL is properly installed
5. See the README.md for detailed usage instructions

### Bug Reports

If you encounter any issues, please report them on the GitHub repository with:
- Detailed steps to reproduce
- Logs from the Output window
- Information about your model files
- Unreal Engine version and operating system details 