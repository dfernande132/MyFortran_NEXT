@echo off
rem limpiar.bat - Borra lo que se puede regenerar: compilaciones, temporales y restos.
rem Uso:   tools\limpiar.bat            (pide confirmacion antes de borrar)
rem        tools\limpiar.bat /f77pag    (ademas borra build\f77pag, imagenes de referencia del estandar)
rem No toca fuentes, documentos, ejemplos .f, ficheros .esperado.txt ni el historial de git.
rem Cerrar antes VS Code (tareas en marcha) y cualquier git.
setlocal EnableExtensions

cd /d "%~dp0.."
if not exist compiler\src\expr.c goto noraiz
if not exist tools\build.ps1 goto noraiz

set TODO=0
if /i "%~1"=="/f77pag" set TODO=1
if "%~1"=="/?" goto ayuda

echo Carpeta del proyecto: %CD%
echo.
echo Se va a borrar (todo se regenera con las tareas de VS Code):
echo   [1] build\            compilaciones: objetos, .map, .bin, .nex, .exe (salvo build\f77pag)
echo   [2] examples\*.nex    generados al construir desde el editor (salvo e1_integrar.nex y MyFortran.nex)
echo   [3] _to_delete\       ficheros apartados en sesiones anteriores
echo   [4] temporales        *.tmp *.bak *.orig *.stackdump Thumbs.db desktop.ini
echo   [5] restos de git     .git\stale-*  .git\*.stale-*  .git\objects\*\tmp_obj_*
if "%TODO%"=="1" echo   [6] build\f77pag      imagenes de referencia (pedido con /f77pag)
echo.
echo No se toca: compiler, editor, platform, docs, tools, examples\*.f, *.esperado.txt, .vscode, historial de git.
echo.
choice /C SN /N /M "Borrar ahora? (S/N) "
if errorlevel 2 goto cancelado

echo.
echo [1] build\
if exist build (
    for /d %%D in (build\*) do (
        if /i not "%%~nxD"=="f77pag" rmdir /s /q "%%D"
    )
    del /q /f build\*.* 2>nul
)
if "%TODO%"=="1" if exist build\f77pag rmdir /s /q build\f77pag

echo [2] examples\*.nex
for %%F in (examples\*.nex) do (
    if /i not "%%~nxF"=="e1_integrar.nex" if /i not "%%~nxF"=="MyFortran.nex" del /q /f "%%F"
)

echo [3] _to_delete\
if exist _to_delete rmdir /s /q _to_delete
if exist examples\estres rmdir examples\estres 2>nul

echo [4] temporales
for /f "delims=" %%F in ('dir /s /b /a-d *.tmp *.bak *.orig *.stackdump Thumbs.db desktop.ini 2^>nul') do (
    echo %%F | find /i "\.git\" >nul || del /q /f "%%F"
)

echo [5] restos de git
if exist .git (
    del /q /f ".git\stale-*" 2>nul
    del /q /f ".git\*.stale-*" 2>nul
    for /d %%D in (.git\objects\??) do del /q /f "%%D\tmp_obj_*" 2>nul
)

echo.
echo Hecho. La proxima tarea de compilacion recrea build\ sola.
goto fin

:ayuda
echo Uso: tools\limpiar.bat [/f77pag]
echo   Borra compilaciones (build\), examples\*.nex generados, _to_delete\, temporales y restos de git.
echo   /f77pag  borra tambien build\f77pag.
goto fin

:noraiz
echo No parece la carpeta de MyFortran (falta compiler\src\expr.c o tools\build.ps1). No se borra nada.
exit /b 1

:cancelado
echo Cancelado. No se ha borrado nada.

:fin
endlocal
