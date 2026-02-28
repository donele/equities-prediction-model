@echo off

call L:\package\gt\prod\setup_gt.bat prod win32 > NUL 2>&1

set region=%1
set idate=%2

:: --------------------------------------------------------------------------------
:: Date
:: --------------------------------------------------------------------------------
if "%region%" == "ca" (
  set dateAdj=-dd 1 -b
)
get_idate.exe -m US %dateAdj% > NUL 2>&1
if "%idate%" == "" (
  set idate=%errorlevel%
)

set basedir=\\smrc-ltc-mrct16\data00\tickMon\ordersumm
set yyyy=%idate:~0,4%
set yyyymm=%idate:~0,6%
set log_dir=L:\package\gt\log\fillratmon
set log_dir_y=%log_dir%\%yyyy%
set log_dir_ym=%log_dir%\%yyyy%\%yyyymm%
if not exist %log_dir_y% mkdir %log_dir_y%
if not exist %log_dir_ym% mkdir %log_dir_ym%
set log_file=%log_dir_ym%\ordersumm_%region%_%dest%.log

if "%region%" == "eu" (
  set markets=EA EB EI EP EF EL ED EM EZ EO EX EC EW EY
  set dests=primary ET EH EG
) else (
if "%region%" == "asia" (
  set markets=AT AH AS
  set dests=primary
) else (
if "%region%" == "ca" (
  set markets=CJ
  set dests=primary CC CH CO CP
)
)
)

for %%M in (%dests%) do (
  call :run %%M
)

goto :myeof


:run

set dest=%1

set conf_file=%region%_%dest%.conf
if exist "%conf_file%" (
  del %conf_file%
)

echo talk main >> %conf_file%
echo   module HInit 0 >> %conf_file%
echo   module HOrderSummRead 1 >> %conf_file%
echo   module HOrderSumm os_L_1 2 >> %conf_file%
echo   module HOrderSumm os_L_2 2 >> %conf_file%
echo   module HOrderSumm os_A_1 2 >> %conf_file%
echo   module HOrderSumm os_A_2 2 >> %conf_file%
echo exit >> %conf_file%
echo talk HInit >> %conf_file%
echo   verbose 1 >> %conf_file%
echo   market %markets% >> %conf_file%
echo   dateFrom %idate% >> %conf_file%
echo   dateTo %idate% >> %conf_file%
echo   requireDataOK false >> %conf_file%
echo   multiThreadModule true >> %conf_file%
echo   multiThreadTicker true >> %conf_file%
echo   nMaxThreadTicker 8 >> %conf_file%
echo exit >> %conf_file%
echo talk HOrderSummRead >> %conf_file%
echo   dest %dest% >> %conf_file%
echo   tickSource order >> %conf_file%
echo exit >> %conf_file%
echo talk os_L_1 >> %conf_file%
echo   execType L >> %conf_file%
echo   orderSchedType 1 >> %conf_file%
echo   basedir %basedir% >> %conf_file%
echo exit >> %conf_file%
echo talk os_L_2 >> %conf_file%
echo   execType L >> %conf_file%
echo   orderSchedType 2 >> %conf_file%
echo   basedir %basedir% >> %conf_file%
echo exit >> %conf_file%
echo talk os_A_1 >> %conf_file%
echo   execType A >> %conf_file%
echo   orderSchedType 1 >> %conf_file%
echo   basedir %basedir% >> %conf_file%
echo exit >> %conf_file%
echo talk os_A_2 >> %conf_file%
echo   execType A >> %conf_file%
echo   orderSchedType 2 >> %conf_file%
echo   basedir %basedir% >> %conf_file%
echo exit >> %conf_file%

start hf %conf_file%

goto :eof

:myeof
