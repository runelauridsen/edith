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
cl %compiler_flags% ..\main.c ..\platform\platform_win32_d3d11.cpp /Feedith.exe /link %linker_flags% /SUBSYSTEM:WINDOWS

:: copy shaders
echo f | xcopy /y /q ..\platform\platform_win32_d3d11_rect.hlsl platform_win32_d3d11_rect.hlsl

endlocal
popd

