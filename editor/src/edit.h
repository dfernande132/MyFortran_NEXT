/* edit.h - Logica del editor: cursor, edicion, vista y pintado.
 *
 * Independiente de la maquina (usa ed_plat.h) y del tamano de la pantalla
 * (ED_ANCHO, ED_FILAS_TEXTO). El texto esta en edbuf.h.
 *
 * Estado: UNA variable global `ed` (decision 0006).
 *
 * Teclas (ed_tecla):
 *   caracteres 32..126   se insertan en el cursor (maximo ED_MAXCOL por linea)
 *   ENTER                parte la linea
 *   BORRAR               borra hacia atras; al principio de linea une con la anterior
 *   cursores             mueven el cursor; arriba/abajo conservan la columna deseada
 *   EDIT (o Esc)         modo comando: la tecla SIGUIENTE se interpreta asi
 *       H inicio de linea   E fin de linea   T inicio del texto   B fin del texto
 *       U pagina arriba     D pagina abajo
 *       K borrar la linea del cursor
 *       C comprobar (mf_compilar sin generar) y saltar al primer error; X / Z: error
 *         siguiente / anterior
 *       O construir: comprobar y, si no hay errores, escribir el .nex (nombre del fuente con
 *         extension .nex; si no tiene nombre lo pide; avisa si el .nex ya existe)
 *         (C y O solo con -DED_CON_COMPILADOR; si no, avisan de que no hay compilador)
 *       G ir a una linea (pide el numero en la ultima fila)
 *       N nuevo   L cargar   S guardar   A guardar como (piden nombre en la ultima fila;
 *       N y L preguntan antes de tirar cambios sin guardar)
 *       5 izquierda  6 abajo  7 arriba  8 derecha   (plan B si los cursores no llegan)
 *   TAB                  inserta blancos hasta la siguiente tabulacion (ver tab_espacios en edit.c)
 */
#ifndef MF_EDIT_H
#define MF_EDIT_H

#include <stdint.h>

#define ED_NOMBRE_MAX 31    /* caracteres del nombre de fichero (sin el NUL) */
#define ED_MAXCOL   72      /* F77 de formato fijo: columnas 1-72; no se teclea mas alla */

typedef struct {
    uint16_t pos;           /* cursor: posicion en el texto (0 .. longitud) */
    uint16_t ini_lin;       /* posicion donde empieza la linea del cursor */
    uint16_t linea;         /* numero de la linea del cursor (desde 0) */
    uint16_t col_obj;       /* columna deseada al moverse en vertical */
    uint16_t top;           /* posicion donde empieza la primera linea visible */
    uint16_t top_linea;     /* numero de esa linea */
    uint16_t izq;           /* primera columna visible (desplazamiento horizontal) */
    int      ultima;        /* ultimo codigo de tecla recibido (crudo) */
    const char *msg;        /* mensaje para la ultima fila (0 = ninguno) */
    uint8_t  msg_err;       /* el mensaje es un error (se pinta con el estilo de error) */
    const char *nombre;     /* fichero cargado (para la barra de estado); debe seguir vivo */
    uint8_t  modo;          /* 0 normal, 1 modo comando, 2 pidiendo nombre, 3 pidiendo confirmacion */
    uint8_t  modificado;    /* hay cambios sin guardar */
    uint8_t  sucio_todo;    /* hay que repintar todas las filas de texto */
    uint8_t  sucio_fila;    /* hay que repintar la fila del cursor */
} Editor;

extern Editor ed;

#define ED_ACCION_EJECUTAR 1   /* reservado: el editor pide ejecutar el programa */
#define ED_ACCION_SALIR    2   /* el editor termina */

/* Arranca el editor (la primera vez: pantalla, texto de arranque) y atiende teclas.
 * Es el punto de entrada unico: main plano o el trampolin t_ed_ejecutar. Ver ed_bucle.c. */
uint8_t ed_ejecutar(void);

/* Vacia el texto y deja el editor listo. Llama a eb_iniciar (no a banco_iniciar). */
void ed_iniciar(void);

/* Inserta una cadena en el cursor ('\n' = nueva linea). Devuelve cuantos caracteres
 * entraron (puede ser menos que su longitud si se llena o supera ED_MAXCOL). */
uint16_t ed_insertar_texto(const char *s);

/* Sustituye el texto por el contenido del fichero (se descartan los '\r'). Deja el
 * cursor al principio y pone en ed.msg el resultado. Devuelve:
 *   0 cargado   1 no se pudo abrir (el texto actual NO se toca)
 *   2 error de lectura (queda lo leido hasta entonces)   3 no cabe (queda lo que cupo) */
uint8_t ed_cargar(const char *nombre);

/* Guarda el texto en el fichero (se crea o se vacia). Si va bien fija el nombre del
 * fichero actual y quita la marca de modificado. Devuelve:
 *   0 guardado   1 no se pudo crear (nada cambia)   2 error al escribir (fichero incompleto) */
uint8_t ed_guardar(const char *nombre);

/* Empieza un fichero nuevo: texto vacio y sin nombre. */
void ed_nuevo(void);

/* Procesa una tecla (codigo crudo de plat_tecla). No pinta: ver ed_refrescar. */
void ed_tecla(int c);

/* Pinta lo que haya cambiado y coloca el cursor. */
void ed_refrescar(void);

#ifdef ED_PRUEBAS
/* Comprueba las invariantes (cursor, linea, vista). 0 = bien; otro valor = cual falla. */
uint8_t ed_verificar(void);
#endif

#endif
