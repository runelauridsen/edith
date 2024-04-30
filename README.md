# Edith
Text editor built from scratch.

https://github.com/runelauridsen/edith/assets/57326389/6e79f18f-20b6-43cc-8db7-678062baf7c2

## Build
### Windows
`cd` to the repository root and run the `build.bat` script.
```
> build.bat
> build\edith.exe
```
Requirements: MSVC compiler + Windows SDK

## Overview
`base/*`: Basic library used across different projects: Math, strings, unicode, allocators, OS abstraction layer and testing. This code is not specific to Edith.

`ui/*`: UI library for managing layout, input events, animation and more. This code is not specific to Edith.

`edith_*`: Core application logic such as editor commands, textviews, undo/redo and UI.

`lang/*`: Language support. Currently only supports syntax highlighting, via a basic C lexer and declaraion parser.

`render_*`: Simple 2d rendering layer, currently only implemented in D3D11.

`main.c`: Entrypoint and platform layer, currently only implemented for Win32.

`thirdparty/*`: Third-party libraries and assets:
- [stb_truetype + stb_image](https://github.com/nothings/stb): Font rasterization and bitmap loading
- [Tracy](https://github.com/wolfpld/tracy): Frame profiler
- OpenSans: UI font
- LiberationMono: Editor font
