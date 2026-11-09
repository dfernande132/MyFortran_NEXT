# build_editor.ps1 - Compilacion del EDITOR de MyFortran (PowerShell).
# Lo usan las tareas "Editor ..." de .vscode/tasks.json. Es aparte de build.ps1
# (que es del compilador) para que las dos conversaciones no editen el mismo fichero.
# Mismas reglas que build.ps1: el PATH se ajusta solo durante la compilacion
# (las DLLs de z88dk y MSYS2 se pisan entre si; ver README, "Incidencias de entorno").
#
# Objetivos:
#   test   editor/src/*.c (sin ed_main.c) + platform/pc/banco_pc.c + cada editor/tests/test_ed*.c
#          con gcc; ejecuta las pruebas. La pantalla es falsa: no hace falta el Next.
#   tilemap SPIKE: editor/plataforma/next/spike_tilemap.c -> build/next/spike_tilemap.nex
#          (80x32 con color por caracter; no usa el editor)
#   next   editor/src/*.c + platform/next/banco_next.c + editor/plataforma/next/ed_next.c + ed_fich.c
#          con z88dk -> build/next/editor.nex; informa de tamano y margen.
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('test', 'next', 'tilemap')]
    [string]$Objetivo,
    # Solo para next: el editor NO lee inicio.f al arrancar (emuladores sin esxdos, donde RST $08 puede colgar)
    [switch]$SinFichero
)

$ErrorActionPreference = 'Stop'
$raiz = Split-Path -Parent $PSScriptRoot   # carpeta MyFortran
Set-Location $raiz

$msys  = 'C:\msys64\ucrt64\bin'
$z88dk = if ($env:Z88DK) { "$env:Z88DK\bin" } else { 'C:\z88dk\bin' }

switch ($Objetivo) {
    'test' {
        $env:Path = "$msys;$env:Path"
        New-Item -ItemType Directory -Force build/pc | Out-Null
        $fuentes = @(Get-ChildItem editor/src/*.c | Where-Object { $_.Name -ne 'ed_main.c' } | ForEach-Object { $_.FullName })
        $fuentes += (Resolve-Path platform/pc/banco_pc.c).Path
        $fallo = 0
        foreach ($t in Get-ChildItem editor/tests/test_ed*.c) {
            $exe = "build/pc/$($t.BaseName).exe"
            # -DED_PRUEBAS: activa ed_verificar() (invariantes del editor)
            # -DED_CON_COMPILADOR + -Icompiler/include: Esc+C/X/Z llaman a mf_compilar (en las pruebas, un compilador FALSO
            #   del propio test_ed01.c; el .nex suelto del editor se compila SIN el: Esc+C dice que no hay compilador)
            # -DED_K_TAB=250: un codigo de tecla Tab inventado, para probar esa ruta (en el Next es 0 = sin definir)
            gcc -std=c99 -Wall -Wextra -Wpedantic -g -DED_PRUEBAS -DED_K_TAB=250 -DED_CON_COMPILADOR -Icompiler/include -Ieditor/src -Iplatform -o $exe $fuentes $t.FullName
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
            Write-Host "== $($t.BaseName) =="
            & $exe
            if ($LASTEXITCODE -ne 0) { $fallo = $LASTEXITCODE }
        }
        exit $fallo
    }
    'tilemap' {
        $env:Path = "$z88dk;$env:Path"
        New-Item -ItemType Directory -Force build/next | Out-Null
        zcc +zxn -clib=classic -lndos -create-app -subtype=nex -m -DMF_TARGET_NEXT -pragma-define:REGISTER_SP=0xBFF0 -o build/next/spike_tilemap editor/plataforma/next/spike_tilemap.c
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        & "$PSScriptRoot/tamano.ps1" -Nombre spike_tilemap
        exit 0
    }
    'next' {
        $env:Path = "$z88dk;$env:Path"
        New-Item -ItemType Directory -Force build/next | Out-Null
        $fuentes = @(Get-ChildItem editor/src/*.c | ForEach-Object { $_.FullName })
        $fuentes += (Resolve-Path platform/next/banco_next.c).Path
        $fuentes += (Resolve-Path editor/plataforma/next/ed_next.c).Path
        $fuentes += (Resolve-Path editor/plataforma/next/ed_fich.c).Path
        # Flags base de MyLISP / core-next (docs/referencia 2.2). Diferencias:
        #   REGISTER_SP=0xFFF0       pila al final de la RAM. El editor SOLO ya no cabe bajo $C000:
        #                            con Nuevo/Cargar/Guardar la imagen acaba en $C168 (16.744 B) y con
        #                            SP=0xBFF0 la pila pisaba las variables (eb, ed...) y reiniciaba el
        #                            emulador. En el .nex integrado el codigo ira al banco 41.
        # (sin CLIB_FGETC_CONS_DELAY: el teclado se lee con in_Inkey, ver ed_next.c)
        $extra = @()
        if ($SinFichero) { $extra += '-DED_SIN_ARRANQUE_FICHERO' }
        # Ficheros junto al .nex: el editor carga inicio.f al arrancar (prueba.f queda para pruebas con Esc+L) (directorio actual de esxdos)
        Copy-Item examples/prueba.f build/next/prueba.f -Force
        Copy-Item examples/inicio.f build/next/inicio.f -Force
        # -lesxdos: esxdos_f_open/read/close (docs/referencia, seccion 5)
        zcc +zxn -clib=classic -lndos -lesxdos -create-app -subtype=nex -m -DMF_TARGET_NEXT -pragma-define:REGISTER_SP=0xFFF0 $extra -Ieditor/src -Iplatform -o build/next/editor $fuentes
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        & "$PSScriptRoot/tamano.ps1" -Nombre editor
        exit 0
    }
}
exit $LASTEXITCODE
