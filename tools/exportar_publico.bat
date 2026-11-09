@echo off
rem Exporta una copia limpia (sin historial y sin lo marcado export-ignore en .gitattributes)
rem a ..\MyFortran-publico, lista para git init + push al repositorio publico.
setlocal
cd /d "%~dp0.."
set DEST=..\MyFortran-publico
if exist "%DEST%" ( echo Ya existe %DEST%. Borrala o renombrala antes. & exit /b 1 )
mkdir "%DEST%"
git archive HEAD | tar -x -C "%DEST%"
if errorlevel 1 exit /b 1
echo Listo: %DEST%
echo Siguiente: cd %DEST% ^&^& git init ^&^& git add . ^&^& git commit -m "MyFortran 0.1" ^&^& git remote add origin URL ^&^& git push -u origin main
