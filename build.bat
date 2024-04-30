@echo off
setlocal
pushd .

:: setup build directory
if not exist build mkdir build
cd build

:: common flags
set compiler_flags=/Zi /nologo /std:c17
set linker_flags=/INCREMENTAL:NO

:: main exe
cl %compiler_flags% ..\main.c ..\render_d3d11.cpp /Feedith.exe /link %linker_flags% /SUBSYSTEM:WINDOWS

:: app dll
::cl %compiler_flags% ..\app.c /DCOMPILE_APP_DLL /Fe:edith.dll /LD         /link %linker_flags%

:: copy shaders
echo f | xcopy /y /q ..\render_d3d11_grid.hlsl render_d3d11_grid.hlsl
echo f | xcopy /y /q ..\render_d3d11_rect.hlsl render_d3d11_rect.hlsl

endlocal
popd

