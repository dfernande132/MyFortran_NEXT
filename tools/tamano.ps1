# tamano.ps1 - Informe de tamano de un programa compilado para el Next.
# Lo llama build.ps1 tras compilar con zcc. Tambien se puede lanzar a mano:
#   powershell -File tools/tamano.ps1 -Nombre core
#
# Formula del margen (docs/referencia, seccion 2.4):
#   margen = 32767 - tamano del fichero binario (imagen de memoria desde $8000)
# Ese margen es TAMBIEN el hueco de pila en ejecucion: mirarlo antes de buscar
# bugs si el programa se cuelga.
#
# El script no interpreta el .map (formato sin verificar para este proyecto):
# extrae las lineas de NUESTROS modulos a un fichero de texto para analizarlas.
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
    $margen = 32767 - $tam
    Write-Host "Imagen binaria : $tam bytes"
    Write-Host "Margen         : $margen bytes (32767 - tamano). Referencia: MyLISP v1.0 tenia ~848."
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
