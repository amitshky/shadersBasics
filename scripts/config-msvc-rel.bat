@echo off
cmake -B build/msvc -S . -DCMAKE_BUILD_TYPE=Release
xcopy /s /y assets\ build\msvc\assets\