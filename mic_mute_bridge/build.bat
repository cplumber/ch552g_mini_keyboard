@echo off
setlocal

set "MINGW_HOME=D:\tools\mingw64"
set "GPP=%MINGW_HOME%\bin\g++.exe"
set "OUTDIR=%~dp0build"

if not exist "%GPP%" (
  echo MinGW-w64 g++ not found at "%GPP%".
  exit /b 1
)

if not exist "%OUTDIR%" mkdir "%OUTDIR%"

"%GPP%" ^
  -std=c++17 ^
  -O2 ^
  -Wall -Wextra -Wpedantic ^
  -static ^
  -static-libgcc ^
  -static-libstdc++ ^
  "%~dp0main.cpp" ^
  -o "%OUTDIR%\mic-mute-bridge.exe" ^
  -lole32 ^
  -loleaut32 ^
  -lsetupapi ^
  -lhid ^
  -luser32 ^
  -luuid

endlocal
