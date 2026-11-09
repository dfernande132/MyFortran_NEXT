/* ed_bucle.c - ed_ejecutar(): arranque y bucle de teclas del editor.
 *
 * Es lo que llama el programa principal (ed_main.c plano, o t_ed_ejecutar en la version
 * con el editor en el banco 41). Al arrancar intenta cargar ED_FICHERO_ARRANQUE; si no
 * puede, deja el texto de ejemplo y dice el error en la ultima fila.
 * El fichero de arranque se carga SIN nombre (ed.nombre = 0): Esc S / Esc O piden uno y
 * no se sobrescribe por accidente la pantalla de bienvenida.
 * Se pinta ANTES de llamar a esxdos: si la maquina se cuelga ahi, queda ese mensaje.
 *
 * Opciones de compilacion:
 *   -DED_SIN_ARRANQUE_FICHERO   no llama a esxdos al arrancar (emuladores sin esxdos)
 *   -DED_FICHERO_ARRANQUE=\"x\"  fichero a cargar al arrancar (por defecto inicio.f)
 *   -DED_GANCHO_TECLA(c)        expresion que se evalua con cada tecla ANTES de ed_tecla;
 *                               si da distinto de 0 la tecla ya esta atendida (pruebas del
 *                               compilador: medir, etc.)
 *
 * Devuelve ED_ACCION_SALIR cuando el editor decida salir (hoy nunca: un .nex no vuelve).
 */
#include "edit.h"
#include "ed_plat.h"

#ifndef ED_FICHERO_ARRANQUE
#define ED_FICHERO_ARRANQUE "inicio.f"
#endif

static const char demo[] =
    "C     MYFORTRAN 0.1  (C) 2026 DFSANTOS\n";

static uint8_t iniciado;

uint8_t ed_ejecutar(void)
{
    const char *m;
    int c;

    if (!iniciado) {
        iniciado = 1;
        plat_iniciar();
        ed_iniciar();
        m = 0;
#ifndef ED_SIN_ARRANQUE_FICHERO
        {
            uint8_t r;

            ed.msg = "Loading " ED_FICHERO_ARRANQUE " ...";
            ed_refrescar();
            r = ed_cargar(ED_FICHERO_ARRANQUE);
            if (r == 1) {
                m = ed.msg;                     /* "Cannot open ..." */
                ed_insertar_texto(demo);
                ed.modificado = 0;
            } else {
                m = ed.msg;
                if (r == 0) ed.nombre = 0;      /* sin nombre: S y O piden uno y no se pisa inicio.f */
            }
        }
#else
        ed_insertar_texto(demo);
        ed.modificado = 0;
#endif
        ed.msg = m;
    }
    ed_refrescar();
    for (;;) {
        c = plat_tecla();
#ifdef ED_GANCHO_TECLA
        if (ED_GANCHO_TECLA(c)) {
            ed_refrescar();
            continue;
        }
#endif
        ed_tecla(c);
        ed_refrescar();
    }
    return ED_ACCION_SALIR;
}
