@echo off
cd /d "%~dp0"
if exist "collected_all.txt" del "collected_all.txt"

for /r %%F in (*.h *.cpp) do (
    echo ===== %%~nxF ===== >> collected_all.txt
    echo Path: %%F >> collected_all.txt
    type "%%F" >> collected_all.txt
    echo. >> collected_all.txt
    echo. >> collected_all.txt
)

echo Done.
pause
