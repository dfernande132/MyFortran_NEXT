# tamano.ps1 - Informe de tamano de un programa compilado para el Next.
# Lo llama build.ps1 tras compilar con zcc. Tambien se puede lanzar a mano:
#   powershell -File tools/tamano.ps1 -Nombre core
#
# Margen de pila: SE LEE DEL .map (REGISTER_SP - __BSS_END_tail). La imagen residente ocupa
# $8000..$BFFF (16 KB), asi que el tamano del fichero NO sirve (la formula heredada de MyLISP,
# 32767 - tamano, daba 17175 B cuando el hueco real era 776 B).
# Ese margen es TAMBIEN el hueco de pila en ejecucion: mirarlo antes de buscar bugs si se cuelga.
# Referencia medida (0.1): hueco nominal 776 B, base de pila en ejecucion $BFAD (709 B), presupuesto
# conservador 645 B (Esc+M); maximo.f usa 583 B.
#
# El resto del script extrae las lineas de NUESTROS modulos a un fichero de texto para analizarlas.
param(
    [Parameter(Mandatory = $true)]
    [string]$Nombre
)

$raiz = Split-Path -Parent $PSScriptRoot
$dir = Join-Path $raiz 'build/next'

Write-Host ''
Write-Host "== Ficheros generados: build/next/$Nombre* =="
Get-ChildItem "$dir/$Nombre*" | ForEach-Object {
    Write-Host ('{0,10:N0} bytes   {1}' -f $_.Length, $_.Name)
}

# La imagen binaria es el fichero SIN extension (como build/MyLISP en MyLISP).
$imagen = Join-Path $dir $Nombre
Write-Host ''
if (Test-Path $imagen -PathType Leaf) {
    $tam = (Get-Item $imagen).Length
    Write-Host "Imagen binaria : $tam bytes"
    $mapaSp = Join-Path $dir "$Nombre.map"
    if (Test-Path $mapaSp) {
        $txt = Get-Content -Raw $mapaSp
        $mSp  = [regex]::Match($txt, '(?m)^REGISTER_SP\s+= \$([0-9A-Fa-f]+)')
        $mBss = [regex]::Match($txt, '(?m)^__BSS_END_tail\s+= \$([0-9A-Fa-f]+)')
        if (-not $mSp.Success -or -not $mBss.Success) {
            Write-Warning 'No encuentro REGISTER_SP o __BSS_END_tail en el .map: margen de pila sin calcular'
        } else {
            $sp = [Convert]::ToInt32($mSp.Groups[1].Value, 16)
            $bss = [Convert]::ToInt32($mBss.Groups[1].Value, 16)
            $margen = $sp - $bss
            Write-Host ('Margen de pila : {0} bytes (SP ${1:X4} - fin de bss ${2:X4}); presupuesto conservador medido 645' -f $margen, $sp, $bss)
            if ($margen -le 512) { Write-Warning 'MARGEN CRITICO DE PILA (<= 512 B)' }
            elseif ($margen -lt 1536) { Write-Warning 'Margen de pila inferior al objetivo (1536 B)' }
        }
    }
} else {
    Write-Host "No encuentro la imagen binaria 'build/next/$Nombre' (sin extension)."
    Write-Host 'Mira la lista de ficheros de arriba y dime cual es el binario.'
}

# Lineas del .map que pertenecen a nuestros modulos.
$mapa = Join-Path $dir "$Nombre.map"
Write-Host ''
if (Test-Path $mapa) {
    $salida = Join-Path $dir "$Nombre-nuestros.txt"
    $lineas = @(Select-String -Path $mapa -Pattern 'source\.c|lexer\.c|clasif\.c|medir_core\.c' | ForEach-Object { $_.Line })
    $lineas | Set-Content $salida
    Write-Host ("Simbolos de nuestros modulos: {0} lineas en build/next/{1}-nuestros.txt" -f $lineas.Count, $Nombre)
} else {
    Write-Host "No existe build/next/$Nombre.map"
}
