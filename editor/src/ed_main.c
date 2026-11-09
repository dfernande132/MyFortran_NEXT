/* ed_main.c - Programa de prueba del editor (version autonoma, sin compilador).
 *
 * Es el .nex que se carga en el Next para probar pantalla, teclado, edicion y
 * ficheros. Todo el trabajo esta en ed_ejecutar() (ed_bucle.c); aqui solo se inicia el
 * sistema de paginas de datos. Un .nex no vuelve al Browser: se reinicia la maquina.
 */
#include "banco.h"
#include "edit.h"

int main(void)
{
    banco_iniciar();
    ed_ejecutar();
    return 0;
}
