@echo off
REM Build and run the headless engine test. Tries MSVC (cl) first, then MinGW (g++).
REM Run from a "x64 Native Tools Command Prompt for VS" for the cl path.
setlocal
cd /d "%~dp0"

where cl >nul 2>nul
if %errorlevel%==0 (
    echo [cl] building...
    cl /nologo /utf-8 /EHsc /std:c++17 test.cpp engine.cpp /Fe:enginetest.exe || goto :fail
    enginetest.exe
    goto :eof
)

where g++ >nul 2>nul
if %errorlevel%==0 (
    echo [g++] building...
    g++ -std=c++17 -static test.cpp engine.cpp -o enginetest.exe || goto :fail
    enginetest.exe
    goto :eof
)

echo No C++ compiler found (need MSVC 'cl' or MinGW 'g++' on PATH).
echo Tip: run verify.py for the same corpus with Python:  python verify.py
exit /b 1

:fail
echo Build failed.
exit /b 1
