@ECHO OFF

cd %~dp0

set "ROOT=%programfiles%\Oclgrind"

mkdir               "%ROOT%"                  || goto :error

xcopy include       "%ROOT%\include" /S /Y /I || goto :error
xcopy x86           "%ROOT%\x86"     /S /Y /I || goto :error
xcopy x64           "%ROOT%\x64"     /S /Y /I || goto :error
xcopy uninstall.bat "%ROOT%\"           /Y    || goto :error

regedit /S oclgrind-icd.reg                   || goto :error

goto :EOF


:error
echo INSTALLATION FAILED
echo Did you run as Administrator?
pause

