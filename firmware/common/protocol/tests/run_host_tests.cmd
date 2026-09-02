@echo off
setlocal

if not defined TCC_EXE set "TCC_EXE=tcc.exe"
where "%TCC_EXE%" >nul 2>nul
if not exist "%TCC_EXE%" (
  echo TinyCC was not found. Put tcc.exe on PATH or update TCC_EXE in this script.
  exit /b 2
)

if not exist "firmware\build\development-only" mkdir "firmware\build\development-only"

"%TCC_EXE%" -Wall -Werror ^
  -I firmware\common\protocol ^
  -I firmware\common\include ^
  firmware\common\protocol\tests\bafang_protocol_test.c ^
  firmware\common\protocol\bafang_protocol.c ^
  firmware\common\protocol\protocol.c ^
  -o firmware\build\development-only\bafang_protocol_test.exe
if errorlevel 1 exit /b %errorlevel%

firmware\build\development-only\bafang_protocol_test.exe
exit /b %errorlevel%
