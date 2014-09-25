@echo off
echo WARNING - use of this batch file is only recommended for advanced users.  
echo           It is used to automate the first step required in creating the 
echo           GPERF HASH files for resiprocate on windows.
echo.
echo Note - Ensure gperf.exe is present in the path or resip/stack directory  
echo        before continuing
echo.
pause
if not exist gen mkdir gen
gperf -C -D -E -L C++ -t --key-positions="*" --compare-strncmp -Z DayOfWeekHash DayOfWeekHash.gperf > gen\DayOfWeekHash.cxx
gperf -C -D -E -L C++ -t --key-positions="*" --compare-strncmp -Z MonthHash MonthHash.gperf > gen\MonthHash.cxx
gperf -C -D -E -L C++ -t --key-positions="*" --compare-strncmp -Z MethodHash MethodHash.gperf > gen\MethodHash.cxx
gperf -C -D -E -L C++ -t --key-positions="*" --compare-strncmp --ignore-case -Z HeaderHash HeaderHash.gperf > gen\HeaderHash.cxx
gperf -C -D -E -L C++ -t --key-positions="*" --compare-strncmp --ignore-case -Z ParameterHash ParameterHash.gperf > gen\ParameterHash.cxx
echo DayOfWeekHash.cxx, MonthHash.cxx, MethodHash.cxx, HeaderHash.cxx and ParameterHash.cxx have been created using gperf.
pause
