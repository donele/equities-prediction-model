@echo off


:: gt_path
::

set GTVER=%1
set GTPlatform=%2
set isNJ=%COMPUTERNAME:~5,2%
set DRV=L
if "%isNJ%" == "NJ" (
  set DRV=K
)

if "%GTVER%" == "dev" (
if not "%gt_dev%" == "" (
if exist "%gt_dev%" (
  set gt_path=%gt_dev%
) else (
  echo path "%gt_dev%" is not found..
  goto :myeof
)
) else (
  echo Error: %%gt_dev%% is not defined.
  goto :myeof
)
) else (
if exist "%DRV%:\package\gt\%GTVER%" (
  set gt_path=%DRV%:\package\gt\%GTVER%
) else (
if exist "%GTVER%" (
  set gt_path=%GTVER%
) else (
  echo invalid version.
  goto :myeof
)))

if not exist "%gt_path%" (
  echo path "%gt_path%" is not found.
  goto :myeof
)


:: other variables
::
set model_path=%DRV%:\package\model\prod
set ROOTSYS=%DRV%:\package\root\v5_19_02
set pythondir=C:\Python25
set pythonpath=%ROOTSYS%\bin


:: PATH
::

set PATH=%gt_path%\win32\release;%PATH%

if "%PROCESSOR_ARCHITECTURE%" == "AMD64" (
if "%GTPlatform%" == "x64" (
  call :set64
))

set PATH=%model_path%\lib;%PATH%
set PATH=%ROOTSYS%\bin;%PATH%
set PATH=%pythondir%;%PATH%

goto :myeof


:: subroutines
::

:set64
set PATH=%gt_path%\x64\release;%PATH%
goto :eof


:myeof
