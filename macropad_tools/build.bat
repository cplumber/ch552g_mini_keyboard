@echo off
setlocal

set "MINGW_HOME=D:\tools\mingw64"
set "GPP=%MINGW_HOME%\bin\g++.exe"
set "AR=%MINGW_HOME%\bin\ar.exe"
set "OUTDIR=%~dp0build"

if not exist "%GPP%" (
  echo MinGW-w64 g++ not found at "%GPP%".
  exit /b 1
)

if not exist "%AR%" (
  echo MinGW-w64 ar not found at "%AR%".
  exit /b 1
)

if not exist "%OUTDIR%" mkdir "%OUTDIR%"

"%GPP%" ^
  -std=c++17 ^
  -O2 ^
  -Wall -Wextra -Wpedantic ^
  -c "%~dp0common\macropad_hid.cpp" ^
  -o "%OUTDIR%\macropad_hid.o"

"%AR%" rcs "%OUTDIR%\libmacropad_hid.a" "%OUTDIR%\macropad_hid.o"

"%GPP%" ^
  -std=c++17 ^
  -O2 ^
  -Wall -Wextra -Wpedantic ^
  -static ^
  -static-libgcc ^
  -static-libstdc++ ^
  "%~dp0main.cpp" "%OUTDIR%\libmacropad_hid.a" ^
  -o "%OUTDIR%\mic-mute-bridge.exe" ^
  -lole32 ^
  -loleaut32 ^
  -lsetupapi ^
  -lhid ^
  -luser32 ^
  -luuid

if errorlevel 1 exit /b 1

"%GPP%" ^
  -std=c++17 ^
  -O2 ^
  -Wall -Wextra -Wpedantic ^
  -static ^
  -static-libgcc ^
  -static-libstdc++ ^
  "%~dp0config_tool\main.cpp" "%OUTDIR%\libmacropad_hid.a" ^
  -o "%OUTDIR%\macropad-config.exe" ^
  -lsetupapi ^
  -lhid

endlocal
