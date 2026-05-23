@echo off
echo Building AVAA Installer...

:: Путь к WiX (стандартная установка)
set WIX_PATH=C:\Program Files (x86)\WiX Toolset v3.14\bin

:: Проверяем наличие WiX
if exist "%WIX_PATH%\candle.exe" (
    echo Found WiX at: %WIX_PATH%
    set PATH=%PATH%;%WIX_PATH%
) else (
    echo WiX Toolset not found!
    echo Please install WiX Toolset v3.14 from: https://wixtoolset.org/releases/
    pause
    exit /b 1
)

:: Компилируем
candle.exe AVAA.wxs -out AVAA.wixobj
if %errorlevel% neq 0 (
    echo Compilation failed!
    pause
    exit /b 1
)

:: Линкуем
light.exe AVAA.wixobj -out AVAA_Installer.msi -ext WixUIExtension
if %errorlevel% neq 0 (
    echo Linking failed!
    pause
    exit /b 1
)

echo Success! Installer created: AVAA_Installer.msi
dir AVAA_Installer.msi
pause