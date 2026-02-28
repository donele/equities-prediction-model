@echo off

SET bin_du="C:\Program Files\Resource Kit\DIRUSE.EXE" /M

SET dir_f_nj15=\\smrc-nj-mrct15\f
SET dir_f_nj14=\\smrc-nj-mrct14\f

SET logfile=diruse.txt

date /T > %logfile%
%bin_du% %dir_f_nj15% >> %logfile%
%bin_du% %dir_f_nj14% >> %logfile%
