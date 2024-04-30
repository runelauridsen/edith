@echo off
cloc . --exclude-dir="thirdparty" --include-ext="c,h,cpp,hlsl,glsl,bat" %*
