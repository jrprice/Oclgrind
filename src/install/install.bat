@ECHO OFF

cd %~dp0

set "ROOT=%programfiles%\Oclgrind"

mkdir               "%ROOT%"                  || goto :error

xcopy include       "%ROOT%\include" /S /Y /I || goto :error
xcopy x86           "%ROOT%\x86"     /S /Y /I || goto :error
xcopy x64           "%ROOT%\x64"     /S /Y /I || goto :error
xcopy uninstall.bat "%ROOT%\"           /Y    || goto :error

regedit /S oclgrind-icd.reg                   || goto :error

echo.
echo Installation completed.
echo.

if not exist C:\Windows\system32\msvcp140.dll (
  echo WARNING: MSVCP140.dll not found - Oclgrind may fail to work correctly
  echo Download the Microsoft Visual C++ Redistributable from here:
  echo.
  echo   https://www.microsoft.com/en-us/download/details.aspx?id=48145
  echo.
  pause
)

goto :EOF


:error
echo INSTALLATION FAILED
echo Did you run as Administrator?
pause

