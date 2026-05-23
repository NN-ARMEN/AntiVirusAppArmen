@echo off
echo Creating simple installer for AVAA...

set BUILD_DIR=C:\AVAA\build\Release
set INSTALL_DIR=C:\AVAA\InstallerPackage
set OUTPUT_ZIP=C:\AVAA\AVAA_Installer.zip

:: Создаем временную папку
rmdir /s /q %INSTALL_DIR% 2>nul
mkdir %INSTALL_DIR%

:: Копируем файлы
copy "%BUILD_DIR%\AVAA.exe" "%INSTALL_DIR%\"
copy "%BUILD_DIR%\AVAAService.exe" "%INSTALL_DIR%\"
copy "%BUILD_DIR%\AVAAMockLicenseServer.exe" "%INSTALL_DIR%\"

:: Создаем скрипт установки
echo @echo off > "%INSTALL_DIR%\install.bat"
echo echo Installing AVAA Antivirus... >> "%INSTALL_DIR%\install.bat"
echo echo. >> "%INSTALL_DIR%\install.bat"
echo echo Creating Program Files folder... >> "%INSTALL_DIR%\install.bat"
echo mkdir "%ProgramFiles%\AVAA" 2^>nul >> "%INSTALL_DIR%\install.bat"
echo echo Copying files... >> "%INSTALL_DIR%\install.bat"
echo copy /y AVAA.exe "%ProgramFiles%\AVAA\" >> "%INSTALL_DIR%\install.bat"
echo copy /y AVAAService.exe "%ProgramFiles%\AVAA\" >> "%INSTALL_DIR%\install.bat"
echo copy /y AVAAMockLicenseServer.exe "%ProgramFiles%\AVAA\" >> "%INSTALL_DIR%\install.bat"
echo echo Installing service... >> "%INSTALL_DIR%\install.bat"
echo sc create AVAA_Service binPath= "\"%ProgramFiles%\AVAA\AVAAService.exe\"" start= auto >> "%INSTALL_DIR%\install.bat"
echo echo Starting service... >> "%INSTALL_DIR%\install.bat"
echo sc start AVAA_Service >> "%INSTALL_DIR%\install.bat"
echo echo. >> "%INSTALL_DIR%\install.bat"
echo echo Installation complete! >> "%INSTALL_DIR%\install.bat"
echo echo You can now run AVAA.exe from Start Menu or Program Files >> "%INSTALL_DIR%\install.bat"
echo pause >> "%INSTALL_DIR%\install.bat"

:: Создаем скрипт удаления
echo @echo off > "%INSTALL_DIR%\uninstall.bat"
echo echo Uninstalling AVAA Antivirus... >> "%INSTALL_DIR%\uninstall.bat"
echo echo Stopping service... >> "%INSTALL_DIR%\uninstall.bat"
echo sc stop AVAA_Service >> "%INSTALL_DIR%\uninstall.bat"
echo echo Deleting service... >> "%INSTALL_DIR%\uninstall.bat"
echo sc delete AVAA_Service >> "%INSTALL_DIR%\uninstall.bat"
echo echo Removing files... >> "%INSTALL_DIR%\uninstall.bat"
echo del /q "%ProgramFiles%\AVAA\AVAA.exe" 2^>nul >> "%INSTALL_DIR%\uninstall.bat"
echo del /q "%ProgramFiles%\AVAA\AVAAService.exe" 2^>nul >> "%INSTALL_DIR%\uninstall.bat"
echo del /q "%ProgramFiles%\AVAA\AVAAMockLicenseServer.exe" 2^>nul >> "%INSTALL_DIR%\uninstall.bat"
echo rmdir "%ProgramFiles%\AVAA\" 2^>nul >> "%INSTALL_DIR%\uninstall.bat"
echo echo. >> "%INSTALL_DIR%\uninstall.bat"
echo echo Uninstall complete! >> "%INSTALL_DIR%\uninstall.bat"
echo pause >> "%INSTALL_DIR%\uninstall.bat"

:: Создаем README
echo AVAA Antivirus Installer > "%INSTALL_DIR%\README.txt"
echo. >> "%INSTALL_DIR%\README.txt"
echo ======================================== >> "%INSTALL_DIR%\README.txt"
echo AVAA Antivirus Installation Guide >> "%INSTALL_DIR%\README.txt"
echo ======================================== >> "%INSTALL_DIR%\README.txt"
echo. >> "%INSTALL_DIR%\README.txt"
echo INSTALLATION: >> "%INSTALL_DIR%\README.txt"
echo 1. Right-click on install.bat and select "Run as Administrator" >> "%INSTALL_DIR%\README.txt"
echo 2. Follow the on-screen instructions >> "%INSTALL_DIR%\README.txt"
echo 3. After installation, launch AVAA.exe from Start Menu or Program Files >> "%INSTALL_DIR%\README.txt"
echo. >> "%INSTALL_DIR%\README.txt"
echo UNINSTALLATION: >> "%INSTALL_DIR%\README.txt"
echo 1. Right-click on uninstall.bat and select "Run as Administrator" >> "%INSTALL_DIR%\README.txt"
echo 2. Follow the on-screen instructions >> "%INSTALL_DIR%\README.txt"
echo. >> "%INSTALL_DIR%\README.txt"
echo Test credentials: >> "%INSTALL_DIR%\README.txt"
echo Login: test >> "%INSTALL_DIR%\README.txt"
echo Password: test >> "%INSTALL_DIR%\README.txt"
echo Activation code: DEMO-KEY >> "%INSTALL_DIR%\README.txt"

:: Создаем ZIP архив
powershell -Command "Compress-Archive -Path '%INSTALL_DIR%\*' -DestinationPath '%OUTPUT_ZIP%' -Force"

echo.
echo ========================================
echo Installer created: %OUTPUT_ZIP%
echo ========================================
echo.
echo To test the installer:
echo 1. Extract AVAA_Installer.zip
echo 2. Run install.bat as Administrator
echo.
pause