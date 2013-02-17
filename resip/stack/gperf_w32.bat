@echo off
echo WARNING - use of this batch file is only recommended for advanced users.  
echo           It is used to automate the first step required in creating the 
echo           GPERF HASH files for resiprocate on windows.
echo.
echo Note - Ensure gperf.exe is present in the path or resip/stack directory  
echo        before continuing
echo.
pause
gperf -D -E -L C++ -t -k "*" --compare-strncmp --ignore-case -Z MethodHash MethodHash.gperf > MethodHash.cxx
gperf -D -E -L C++ -t -k "*" --compare-strncmp --ignore-case -Z HeaderHash HeaderHash.gperf > HeaderHash.cxx
gperf -D -E -L C++ -t -k "*" --compare-strncmp --ignore-case -Z ParameterHash ParameterHash.gperf > ParameterHash.cxx
echo Don't forget to manually add the tolower calls - see gperfNotes.txt and Makefile
pause
