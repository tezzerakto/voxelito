@echo off
REM Cambia el nombre del archivo según tu archivo fuente
set SOURCE=vox.c
set OUTPUT=vox.exe
set RESOURCE=icono.rc
set RESFILE=icono.res

REM Cambia al directorio donde está tu archivo fuente
cd /d c:\voxelito

REM Compila el archivo de recursos para generar el archivo .res
windres %RESOURCE% -O coff -o %RESFILE%

REM Verifica si la compilación del archivo de recursos fue exitosa
if %ERRORLEVEL% neq 0 (
    echo Error durante la compilación del recurso.
    pause
    exit /b
)

REM Compila el programa usando GCC, incluyendo el archivo de recursos
gcc %SOURCE% %RESFILE% -o %OUTPUT% -lgdi32 -lwinmm

REM Verifica si la compilación fue exitosa
if %ERRORLEVEL% neq 0 (
    echo Error durante la compilación.
    pause
    exit /b
)

echo Compilación exitosa. Ejecutando el programa...
start %OUTPUT%
