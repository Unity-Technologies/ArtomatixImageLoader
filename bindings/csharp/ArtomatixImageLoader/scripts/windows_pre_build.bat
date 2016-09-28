set "PROJDIR=%1"
set "CONFIG=%2"
set "REALARCH=%3"

if %REALARCH%==x64 (
	set "ARCH= Win64"
) else (
	if %REALARCH%==x86 (
		set "ARCH="
	) else (
		echo "bad ARCH value"
		exit /b 1
	)
)


echo "--------------------------" %PROJDIR% %CONFIG% %REALARCH%

set "TOP_DIR=%PROJDIR%\..\..\..\.."
set "BUILD_DIR=%TOP_DIR%\src_c\build_%CONFIG%_%REALARCH%"
set "C_DLL=%BUILD_DIR%\inst\lib\AIL.dll"

echo %BUILD_DIR%

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

set CURRDIR=%CD%

cd "%BUILD_DIR%"

cmake .. -DCMAKE_BUILD_TYPE=%CONFIG% -DPYTHON_ENABLED=Off -DCMAKE_INSTALL_PREFIX=inst -G "Visual Studio 14%ARCH%"
if %errorlevel% neq 0 exit /b %errorlevel%
cmake --build . --target install --config %CONFIG%
if %errorlevel% neq 0 exit /b %errorlevel%

if not exist "%PROJDIR%\embedded_files" mkdir "%PROJDIR%\embedded_files"

copy "%C_DLL%" "%PROJDIR%\embedded_files\native_code"

cd %CURRDIR%