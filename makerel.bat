@echo off

set okay=0
set version=%1
set do_debug=%2

if "%version%" neq "" (
  if "%do_debug%" == "debug" (
    xcopy /S /I /Y win32\debug\*.* ..\%version%\win32\debug
    xcopy /S /I /Y x64\debug\*.* ..\%version%\x64\debug
  )

echo Preparing original...
xcopy /q /S /I /Y win32\release\*.exe ..\%version%\win32\release > nul
xcopy /q /S /I /Y win32\release\*.dll ..\%version%\win32\release > nul

xcopy /q /S /I /Y x64\release\*.exe ..\%version%\x64\release > nul
xcopy /q /S /I /Y x64\release\*.dll ..\%version%\x64\release > nul

xcopy /q /S /Y setup_gt.bat ..\%version%\ > nul
xcopy /q /S /Y setup_gt_root519.bat ..\%version%\ > nul
::xcopy /q /S /E /I /Y cronjobs ..\%version%\cronjobs > nul
xcopy /q /S /I /Y source ..\%version%\source > nul

) else (
goto :myeof
)

call :do_copy %version%
goto :finish


:do_copy
set version=%1
::set dir=\\smrc-ltc-mrct40\l\package\gt
set dir=L:\package\gt

echo Copying...
xcopy /q /S /I /Y ..\%version% %dir%\%version% > nul && set okay=1

goto :eof


:finish
del /Q /S ..\%version% > nul
rmdir /Q /S ..\%version%

if %okay% == 1 (
echo Files are copied to %dir%\%version%
) else (
echo Copy has failed.
)
date /T
time /T
echo.

:myeof

