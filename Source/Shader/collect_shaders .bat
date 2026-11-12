@echo off
cd /d "%~dp0"
if exist "collected_shaders.txt" del "collected_shaders.txt"

for %%F in (*.hlsl *.hlsli) do (
    echo ===== %%~nxF ===== >> collected_shaders.txt
    echo Path: %%F >> collected_shaders.txt
    type "%%F" >> collected_shaders.txt
    echo. >> collected_shaders.txt
    echo. >> collected_shaders.txt
)

echo Done.
pause
