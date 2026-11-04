@call "%~dp0%test_nasa.cmd"
@if errorlevel 1 exit /b 1
@call "%~dp0%build_and_run.cmd" test/main_test_nasa_display.cpp
@if errorlevel 1 exit /b 1

@call "%~dp0%test_non_nasa.cmd"
@if errorlevel 1 exit /b 1

@call "%~dp0%test_non_nasa_queue.cmd"
@if errorlevel 1 exit /b 1
