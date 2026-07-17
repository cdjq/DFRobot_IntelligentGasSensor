@echo off
setlocal
chcp 65001 >nul

where py >nul 2>nul
if errorlevel 1 goto use_python

py -3 "%~dp0setup.py" %*
goto finished

:use_python
python "%~dp0setup.py" %*

:finished
set "SETUP_EXIT=%ERRORLEVEL%"
echo.
if "%SETUP_EXIT%"=="0" echo Setup completed.
if not "%SETUP_EXIT%"=="0" echo Setup did not fully pass. Exit code: %SETUP_EXIT%
if defined DFROBOT_SETUP_NO_PAUSE goto exit_setup
pause

:exit_setup
exit /b %SETUP_EXIT%
