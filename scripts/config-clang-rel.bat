@echo off
cmake -B build/clang/rel -S . -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Release
xcopy /y .\build\clang\rel\compile_commands.json .