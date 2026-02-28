@echo off

call L:\package\gt\prod\setup_gt.bat prod

if "%1"=="" (
root -b -q "wmon.C()"
) else (
if "%1"=="E" (
root -b -q "wmon.C(\"E\")"
) else (
if "%1"=="A" (
root -b -q "wmon.C(\"A\")"
) else (
if "%1"=="C" (
root -b -q "wmon.C(\"C\")"
) else (
if "%1"=="M" (
root -b -q "wmon.C(\"M\")"
)
if "%1"=="html" (
root -b -q "wmon.C(\"html\")"
)
)
)
)
)
