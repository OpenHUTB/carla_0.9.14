@echo off
setlocal enabledelayedexpansion

rem 创建二进制 Carla 的批处理脚本
rem 通过启用 x64 Visual C++ Toolset 的命令行运行该脚本。

set LOCAL_PATH=%~dp0
set FILE_N=-[%~n0]:

rem 打印批处理参数（为了调试）
echo %FILE_N% [Batch params]: %*

rem ============================================================================
rem -- 解析参数 ----------------------------------------------------------------
rem ============================================================================

set BUILD_UE4_EDITOR=false
set LAUNCH_UE4_EDITOR=false
set REMOVE_INTERMEDIATE=false
set USE_CARSIM=false
set USE_CHRONO=false
set CARSIM_STATE="CarSim OFF"
set CHRONO_STATE="Chrono OFF"
set EDITOR_FLAGS=""

:arg-parse
echo %1
if not "%1"=="" (
    if "%1"=="--editor-flags" (
        set EDITOR_FLAGS=%2
        shift
    )
    if "%1"=="--build" (
        set BUILD_UE4_EDITOR=true
    )
    if "%1"=="--launch" (
        set LAUNCH_UE4_EDITOR=true
    )
    if "%1"=="--clean" (
        set REMOVE_INTERMEDIATE=true
    )
    if "%1"=="--carsim" (
        set USE_CARSIM=true
    )
    if "%1"=="--chrono" (
        set USE_CHRONO=true
    )
    if "%1"=="-h" (
        goto help
    )
    if "%1"=="--help" (
        goto help
    )
    shift
    goto arg-parse
)
rem 移除参数中的引号
set EDITOR_FLAGS=%EDITOR_FLAGS:"=%

if %REMOVE_INTERMEDIATE% == false (
    if %LAUNCH_UE4_EDITOR% == false (
        if %BUILD_UE4_EDITOR% == false (
            goto help
        )
    )
)

rem 获得虚幻引擎的根目录
if not defined UE4_ROOT (
    set KEY_NAME="HKEY_LOCAL_MACHINE\SOFTWARE\EpicGames\Unreal Engine"
    set VALUE_NAME=InstalledDirectory
    for /f "usebackq tokens=1,2,*" %%A in (`reg query !KEY_NAME! /s /reg:64`) do (
        if "%%A" == "!VALUE_NAME!" (
            set UE4_ROOT=%%C
        )
    )
    if not defined UE4_ROOT goto error_unreal_no_found
)
if not "%UE4_ROOT:~-1%"=="\" set UE4_ROOT=%UE4_ROOT%\

rem 设置VS解决方案的目录
rem
set UE4_PROJECT_FOLDER=%ROOT_PATH:/=\%Unreal\CarlaUE4\
pushd "%UE4_PROJECT_FOLDER%"

rem 清除由构建系统生成的二进制文件和中间文件
rem
if %REMOVE_INTERMEDIATE% == true (
    rem 删除目录
    for %%G in (
        "%UE4_PROJECT_FOLDER%Binaries",
        "%UE4_PROJECT_FOLDER%Build",
        "%UE4_PROJECT_FOLDER%Saved",
        "%UE4_PROJECT_FOLDER%Intermediate",
        "%UE4_PROJECT_FOLDER%Plugins\Carla\Binaries",
        "%UE4_PROJECT_FOLDER%Plugins\Carla\Intermediate",
        "%UE4_PROJECT_FOLDER%.vs"
    ) do (
        if exist %%G (
            echo %FILE_N% Cleaning %%G
            rmdir /s/q %%G
        )
    )

    rem 删除文件
    for %%G in (
        "%UE4_PROJECT_FOLDER%CarlaUE4.sln"
    ) do (
        if exist %%G (
            echo %FILE_N% Cleaning %%G
            del %%G
        )
    )
)

rem 构建 Carla 编辑器
rem

if %USE_CARSIM% == true (
    py -3 %ROOT_PATH%Util/BuildTools/enable_carsim_to_uproject.py -f="%ROOT_PATH%Unreal/CarlaUE4/CarlaUE4.uproject" -e
    set CARSIM_STATE="CarSim ON"
) else (
    py -3 %ROOT_PATH%Util/BuildTools/enable_carsim_to_uproject.py -f="%ROOT_PATH%Unreal/CarlaUE4/CarlaUE4.uproject"
    set CARSIM_STATE="CarSim OFF"
)
if %USE_CHRONO% == true (
    set CHRONO_STATE="Chrono ON"
) else (
    set CHRONO_STATE="Chrono OFF"
)
set OPTIONAL_MODULES_TEXT=%CARSIM_STATE% %CHRONO_STATE%
echo %OPTIONAL_MODULES_TEXT% > "%ROOT_PATH%Unreal/CarlaUE4/Config/OptionalModules.ini"

if %BUILD_UE4_EDITOR% == true (
    echo %FILE_N% Building Unreal Editor...

    call "%UE4_ROOT%Engine\Build\BatchFiles\Build.bat"^
        CarlaUE4Editor^
        Win64^
        Development^
        -WaitMutex^
        -FromMsBuild^
        "%ROOT_PATH%Unreal/CarlaUE4/CarlaUE4.uproject"
    if errorlevel 1 goto bad_exit

    call "%UE4_ROOT%Engine\Build\BatchFiles\Build.bat"^
        CarlaUE4^
        Win64^
        Development^
        -WaitMutex^
        -FromMsBuild^
        "%ROOT_PATH%Unreal/CarlaUE4/CarlaUE4.uproject"
    if errorlevel 1 goto bad_exit
)

rem 启动 Carla 编辑器
rem
if %LAUNCH_UE4_EDITOR% == true (
    echo %FILE_N% Launching Unreal Editor...
    call "%UE4_ROOT%\Engine\Binaries\Win64\UE4Editor.exe"^
        "%UE4_PROJECT_FOLDER%CarlaUE4.uproject" %EDITOR_FLAGS%
    if %errorlevel% neq 0 goto error_build
)

goto good_exit

rem ============================================================================
rem -- 消息和错误 --------------------------------------------------------------
rem ============================================================================

:help
    echo Build LibCarla.
    echo "Usage: %FILE_N% [-h^|--help] [--build] [--launch] [--clean]"
    goto good_exit

:error_build
    echo.
    echo %FILE_N% [ERROR] There was a problem building CarlaUE4.
    echo %FILE_N%         Please go to "Carla\Unreal\CarlaUE4", right click on
    echo %FILE_N%         "CarlaUE4.uproject" and select:
    echo %FILE_N%         "Generate Visual Studio project files"
    echo %FILE_N%         Open de generated "CarlaUE4.sln" and try to manually compile it
    echo %FILE_N%         and check what is causing the error.
    goto bad_exit

:good_exit
    endlocal
    exit /b 0

:bad_exit
    endlocal
    exit /b %errorlevel%

:error_unreal_no_found
    echo.
    echo %FILE_N% [ERROR] Unreal Engine not detected
    goto bad_exit
