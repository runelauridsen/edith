@echo off
setlocal
pushd .

:: setup build directory
if not exist build mkdir build
cd build

:: common flags
set compiler_flags=/Zi /nologo /std:c17
set linker_flags=/INCREMENTAL:NO

cls

:: main exe
cl %compiler_flags% ..\main.c ..\render_d3d11.cpp /DRUN_TESTS /Fetests.exe /link %linker_flags% /subsystem:console

if %errorlevel% == 0 tests.exe

endlocal
popd

