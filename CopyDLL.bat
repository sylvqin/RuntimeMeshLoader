@echo off
echo Runtime Mesh Loader - DLL Copy Utility
echo =====================================
echo.

set SOURCE_DLL="%~dp0ThirdParty\assimp\bin\assimp-vc142-mt.dll"
set TARGET_PLUGIN_BINARIES="%~dp0Binaries\Win64\assimp-vc142-mt.dll"
set TARGET_PROJECT_BINARIES="%~dp0..\..\Binaries\Win64\assimp-vc142-mt.dll"

echo Source DLL path: %SOURCE_DLL%
echo.

if not exist %SOURCE_DLL% (
    echo ERROR: Source DLL not found at %SOURCE_DLL%
    exit /b 1
)

echo Creating target directories if they don't exist...
if not exist "%~dp0Binaries\Win64" mkdir "%~dp0Binaries\Win64"
if not exist "%~dp0..\..\Binaries\Win64" mkdir "%~dp0..\..\Binaries\Win64"

echo.
echo Copying DLL to plugin binaries...
copy /Y %SOURCE_DLL% %TARGET_PLUGIN_BINARIES%
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to copy DLL to plugin binaries
) else (
    echo SUCCESS: DLL copied to plugin binaries
)

echo.
echo Copying DLL to project binaries...
copy /Y %SOURCE_DLL% %TARGET_PROJECT_BINARIES%
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to copy DLL to project binaries
) else (
    echo SUCCESS: DLL copied to project binaries
)

echo.
echo DLL copy operation completed.
echo ===================================== 