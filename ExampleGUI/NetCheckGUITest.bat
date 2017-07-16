@echo off
@DotNetCheckerGUI.exe -v blah
@echo Errorlevel is %ERRORLEVEL% expected 0
@DotNetCheckerGUI.exe -v 9.9
@echo Errorlevel is %ERRORLEVEL% expected 0
@DotNetCheckerGUI.exe -v 4.6.2
@echo Errorlevel is %ERRORLEVEL% expected 1
@DotNetCheckerGUI.exe -f test.txt
@echo Errorlevel is %ERRORLEVEL% expected 1
@DotNetCheckerGUI.exe -f 
@echo Errorlevel is %ERRORLEVEL% expected not 0 or 1
@DotNetCheckerGUI.exe -p
@echo Errorlevel is %ERRORLEVEL% expected 1
@DotNetCheckerGUI.exe -h
@echo Errorlevel is %ERRORLEVEL% expected 1
@pause