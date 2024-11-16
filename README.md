# Camera App
- Displays camera output on TV and GamePad


## Dependencies
- [devkitPPC](https://devkitpro.org)
- [wut](https://github.com/devkitPro/wut)
- [CafeGLSL](https://github.com/Exzap/CafeGLSL)

## Build
```
cmake -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/WiiU.cmake -S . -B build
cd build
cmake --build .
```
