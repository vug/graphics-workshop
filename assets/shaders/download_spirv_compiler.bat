@echo off
setlocal

set URL=https://github.com/KhronosGroup/glslang/releases/download/main-tot/glslang-master-windows-Release.zip
set ZIPFILE=%~dp0glslang-master-windows-Release.zip
set THISFOLDER=%~dp0

PowerShell -Command "Invoke-WebRequest -Uri '%URL%' -OutFile '%ZIPFILE%'"
PowerShell -Command "Expand-Archive -Path '%ZIPFILE%' -DestinationPath '%THISFOLDER%\glslang' -Force"
PowerShell -Command "Copy-Item -Path '%THISFOLDER%\glslang\bin\glslangValidator.exe' -Destination '%THISFOLDER%\'"
PowerShell -Command "Remove-Item -Path '%THISFOLDER%\glslang' -Recurse -Force"

endlocal