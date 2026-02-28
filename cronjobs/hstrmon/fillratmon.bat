@echo off

call L:\package\gt\prod\setup_gt_root519.bat prod > NUL 2>&1

set log_dir=L:\package\gt\log\fillratmon
set log_file=%log_dir%\fillratmon.log

fillratmon.py

