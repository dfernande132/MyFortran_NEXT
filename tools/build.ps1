# build.ps1 - Lanzador de compilacion de MyFortran (PowerShell, viene con Windows).
# Lo usan las tareas de .vscode/tasks.json. Se puede ejecutar tambien a mano:
#   powershell -File tools/build.ps1 -Objetivo smoke-pc
#
# Objetivos:
#   smoke-pc, smoke-next  comprobar que las herramientas funcionan
#   test                  compiler/src/*.c + cada compiler/tests/test_*.c; ejecuta las pruebas
#   tac-pc                compila un programa Fortran (-Programa fichero.f), muestra su codigo de tres direcciones y lo ejecuta en PC
#   z80-pc                compila un programa Fortran (-Programa fichero.f) a Z80: muestra el .asm, lo ejecuta en el emulador y deja build/pc/<nombre>.asm, .bin, .nex (para CSpect/Next) y .esperado.txt
#   spike-next            tools/spike/<Spike>.c + platform/next/*.c con z88dk (experimentos de arquitectura)
#   next                  MyFortran: editor + compilador en un .nex (build/next/MyFortran.nex); es el spike e1_integrar con nombre definitivo
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('smoke-pc', 'smoke-next', 'test', 'tac-pc', 'z80-pc', 'spike-next', 'next')]
    [string]$Objetivo,
    # Solo para spike-next: nombre del fichero de tools/spike/ sin extension (p. ej. s1_banco)
    [string]$Spike = '',
    # Solo para tac-pc: fichero Fortran 77 de formato fijo
    [string]$Programa = ''
)

$ErrorActionPreference = 'Stop'
$raiz = Split-Path -Parent $PSScriptRoot   # carpeta MyFortran
Set-Location $raiz

# Rutas de los compiladores. Cada uno trae sus propias DLLs (libgcc_s_seh-1.dll,
# libstdc++-6.dll, zlib1.dll...) con el MISMO nombre pero versiones distintas.
# Si se mezclan en el PATH, cc1.exe de gcc muere sin escribir ningun mensaje.
# Solucion: poner delante, solo durante esta compilacion, el directorio del
# compilador que toca. No se modifica el PATH de Windows.
$msys  = 'C:\msys64\ucrt64\bin'
$z88dk = if ($env:Z88DK) { "$env:Z88DK\bin" } else { 'C:\z88dk\bin' }

switch ($Objetivo) {
    'smoke-pc' {
        $env:Path = "$msys;$env:Path"
        New-Item -ItemType Directory -Force build/pc | Out-Null
        # -Wall -Wextra -Wpedantic: avisos estrictos; -g: simbolos para depurar con gdb
        gcc -std=c99 -Wall -Wextra -Wpedantic -g -o build/pc/smoke.exe tools/smoke/smoke.c
    }
    'test' {
        $env:Path = "$msys;$env:Path"
        New-Item -ItemType Directory -Force build/pc | Out-Null
        # gcc en Windows no siempre expande *.c, asi que listamos los ficheros aqui
        $fuentes = @(Get-ChildItem compiler/src/*.c, platform/pc/*.c, platform/texto.c | ForEach-Object { $_.FullName })
        # Un ejecutable por capitulo: cada compiler/tests/test_*.c tiene su main
        $fallo = 0
        foreach ($t in Get-ChildItem compiler/tests/test_*.c) {
            $exe = "build/pc/$($t.BaseName).exe"
            gcc -std=c99 -Wall -Wextra -Wpedantic -g -Icompiler/src -Icompiler/include -Iplatform -o $exe $fuentes $t.FullName
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
            Write-Host "== $($t.BaseName) =="
            & $exe
            if ($LASTEXITCODE -ne 0) { $fallo = $LASTEXITCODE }
        }
        exit $fallo
    }
    'tac-pc' {
        if ($Programa -eq '') { Write-Host 'Falta -Programa <fichero.f>'; exit 1 }
        $env:Path = "$msys;$env:Path"
        New-Item -ItemType Directory -Force build/pc | Out-Null
        $fuentes = @(Get-ChildItem compiler/src/*.c, platform/pc/*.c, platform/texto.c | ForEach-Object { $_.FullName })
        gcc -std=c99 -Wall -Wextra -Wpedantic -g -Icompiler/src -Icompiler/include -Iplatform -o build/pc/lst.exe $fuentes tools/medir/lst.c
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        Get-Content -Raw $Programa | & build/pc/lst.exe
        exit 0
    }
    'z80-pc' {
        if ($Programa -eq '') { Write-Host 'Falta -Programa <fichero.f>'; exit 1 }
        $env:Path = "$msys;$env:Path"
        New-Item -ItemType Directory -Force build/pc | Out-Null
        $fuentes = @(Get-ChildItem compiler/src/*.c, platform/pc/*.c, platform/texto.c | ForEach-Object { $_.FullName })
        gcc -std=c99 -Wall -Wextra -Wpedantic -g -Icompiler/src -Icompiler/include -Iplatform -o build/pc/z80.exe $fuentes tools/medir/z80.c
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        $nombre = [System.IO.Path]::GetFileNameWithoutExtension($Programa)
        Get-Content -Raw $Programa | & build/pc/z80.exe "build/pc/$nombre.asm" "build/pc/$nombre.bin" "build/pc/$nombre.nex" "build/pc/$nombre.esperado.txt"
        # Comprobacion independiente: si hay z80asm de z88dk, ensambla el .asm y compara con nuestro .bin
        $z80asm = Join-Path $z88dk 'z88dk-z80asm.exe'
        if (Test-Path $z80asm) {
            $env:Path = "$z88dk;$env:Path"
            New-Item -ItemType Directory -Force build/pc/ref | Out-Null
            Copy-Item "build/pc/$nombre.asm" "build/pc/ref/$nombre.asm" -Force
            Remove-Item "build/pc/ref/$nombre.bin" -ErrorAction SilentlyContinue
            & $z80asm -b "build/pc/ref/$nombre.asm"
            if ($LASTEXITCODE -ne 0 -or -not (Test-Path "build/pc/ref/$nombre.bin")) {
                Write-Host '--- z80asm: ERROR al ensamblar (ver mensajes arriba)'
            } else {
                $a = [System.IO.File]::ReadAllBytes("$raiz/build/pc/$nombre.bin")
                $b = [System.IO.File]::ReadAllBytes("$raiz/build/pc/ref/$nombre.bin")
                $d = -1
                if ($a.Length -eq $b.Length) {
                    for ($i = 0; $i -lt $a.Length; $i++) { if ($a[$i] -ne $b[$i]) { $d = $i; break } }
                    if ($d -lt 0) { Write-Host "--- z80asm: COINCIDEN los $($a.Length) bytes" }
                    else { Write-Host ("--- z80asm: DIFIEREN en el offset {0} (direccion {1:X4}): mio {2:X2}, z80asm {3:X2}" -f $d, (0x8000 + $d), $a[$d], $b[$d]) }
                } else {
                    Write-Host "--- z80asm: DIFIEREN en longitud: mio $($a.Length), z80asm $($b.Length)"
                }
            }
        } else {
            Write-Host "--- (sin z80asm en ${z88dk} - no se hace la comprobacion independiente)"
        }
        exit 0
    }
    { $_ -in 'spike-next', 'next' } {
        $Salida = $Spike
        if ($Objetivo -eq 'next') { $Spike = 'e1_integrar'; $Salida = 'MyFortran' }
        if ($Spike -eq '') { Write-Host 'Falta -Spike <nombre> (fichero de tools/spike/ sin .c)'; exit 1 }
        $env:Path = "$z88dk;$env:Path"
        New-Item -ItemType Directory -Force build/next | Out-Null
        $plat = @(Get-ChildItem platform/next/*.c | ForEach-Object { $_.FullName })
        # los spikes de integracion (i1_, g1_, e1_) usan el ed_linea REAL del editor: no enlazar texto.c
        if (-not ($Spike.StartsWith("i1_") -or $Spike.StartsWith("g1_") -or $Spike.StartsWith("e1_"))) { $plat += (Resolve-Path platform/texto.c).Path }
        # Un spike puede tener varios ficheros: <prefijo>_*.c (p. ej. s2_codigo.c y s2_banco47.c)
        $prefijo = ($Spike -split '_')[0]
        $fuentesSpike = @(Get-ChildItem "tools/spike/${prefijo}_*.c" | ForEach-Object { $_.FullName })
        # ensamblador del spike (p. ej. e1_tramp.asm: trampolines de banco genericos)
        $fuentesSpike += @(Get-ChildItem "tools/spike/${prefijo}_*.asm" -ErrorAction SilentlyContinue | ForEach-Object { $_.FullName })
        # Flags extra opcionales: tools/spike/<Spike>.flags (una por linea, # = comentario)
        $extra = @()
        $ficheroFlags = "tools/spike/$Spike.flags"
        if (Test-Path $ficheroFlags) {
            $extra = @(Get-Content $ficheroFlags | Where-Object { $_.Trim() -ne '' -and -not $_.Trim().StartsWith('#') } | ForEach-Object { $_.Trim() })
        }
        zcc +zxn -clib=classic -lndos -lesxdos -create-app -subtype=nex -m -DMF_TARGET_NEXT $extra -Icompiler/src -Icompiler/include -Iplatform -o "build/next/$Salida" $plat $fuentesSpike
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        & "$PSScriptRoot/tamano.ps1" -Nombre $Salida
        # Simbolos de bancos de codigo (si los hay)
        if (Test-Path "build/next/$Salida.map") {
            Write-Host '--- simbolos de banco en el .map ---'
            Select-String -Path "build/next/$Salida.map" -Pattern '^_b[0-9]+_|^__CODE_[0-9]+_(head|size)|^__BANK_[0-9]+_size +=  \$0*[1-9A-F]|^__tail |^__BSS_tail |^__BSS_END_tail |^__DATA_tail |^__DATA_END_tail ' | Select-Object -First 20 | ForEach-Object { $_.Line }
        }
        Get-ChildItem build/next -Filter "$Salida*" | ForEach-Object { Write-Host ("{0,-28} {1,8} bytes" -f $_.Name, $_.Length) }
        exit 0
    }
    'smoke-next' {
        $env:Path = "$z88dk;$env:Path"
        New-Item -ItemType Directory -Force build/next | Out-Null
        # Mismos flags base que MyLISP (docs/referencia, seccion 2.2):
        #   +zxn -clib=classic -lndos -create-app -subtype=nex -m (genera .map)
        zcc +zxn -clib=classic -lndos -create-app -subtype=nex -m -DMF_TARGET_NEXT -o build/next/smoke tools/smoke/smoke.c
    }
}
exit $LASTEXITCODE
