/* mf_api.h - Contrato entre el EDITOR y el COMPILADOR (COORDINACION.md, seccion 3).
 *
 *   El editor PROPORCIONA:   ed_linea()                 (RESIDENTE, nunca en un banco)
 *   El compilador OFRECE:    mf_compilar(), mf_mensaje(), la tabla `mf`
 *
 * Uso:
 *     n = mf_compilar();                  // 0 = sin errores
 *     for (i = 0; i < n && i < MF_DIAG_MAX; i++) {
 *         mf_mensaje(mf.diag[i].cod, buf);       // buf: MF_MENSAJE_MAX bytes, residente
 *         ... mostrar mf.diag[i].linea / .columna / buf ...
 *     }
 * `mf.n` cuenta TODOS los errores; en `mf.diag` solo se guardan los primeros
 * MF_DIAG_MAX (en orden de aparicion en el fichero). Los mensajes estan en ingles.
 *
 * Reglas del contrato:
 *
 * Comprobar y construir (COORDINACION.md, entrada 2026-10-31 "construir"):
 *     mf_salida(NULL);          // "comprobar": solo analiza, no genera nada
 *     mf_salida(&salida);       // "construir": si no hay errores, genera el Z80 y escribe el .nex
 *     n = mf_compilar();
 *     if (n == 0 && mf.gen == MF_GEN_ERR) mf_mensaje(mf.gen_cod, buf);   // fallo al generar/guardar
 * Sin mf_salida registrada mf_compilar se comporta exactamente como antes (mf.gen = MF_GEN_NO).
 *
 *  - ed_linea(n, dst): copia en dst la linea fisica n (la primera es la 1), SIN salto
 *    de linea, como mucho 72 caracteres mas '\0' (dst tiene 73 bytes, residente).
 *    Devuelve 1 si la linea existe y 0 si n pasa del final. El compilador la pide en
 *    orden creciente (n, n+1, ...) y solo vuelve atras al empezar otra compilacion,
 *    asi que basta con una cache de la ultima linea. Sin '\t' ni '\r' (si llegan, el
 *    compilador los tolera: tabuladores segun source.c, '\r' lo trata el editor).
 *  - Mientras dura mf_compilar el editor no cambia el texto.
 *  - Las columnas son 1..72 (el texto empieza en la 7). Una columna >255 se satura.
 */
#ifndef MF_API_H
#define MF_API_H

#include <stdint.h>

#define MF_DIAG_MAX     16
#define MF_MENSAJE_MAX  64      /* el mensaje mas largo mide 56 + '\0' */

/* Codigo de diagnostico (MfDiag.cod): un unico espacio de numeros de 8 bits.
 * Los de la fase semantica conservan su valor de ErrorSem (1..59). */
#define MF_COD_FUENTE   60      /* + ErrorFuente (2..4)                      */
#define MF_COD_STMT     70      /* + ErrorStmt (1..17)                       */
#define MF_COD_EXPR     90      /* + ErrorExpr (1..10)                       */
#define MF_COD_LEX      110      /* + ErrorLex (0..6)                         */
#define MF_COD_GEN      120     /* generacion del .nex (1..8), solo con mf.gen == MF_GEN_ERR */
#define MF_COD_UNI      130     /* + ErrorUni (1..20): unidades de programa y llamadas (unid.h)  */

#define MF_GEN_NO       0       /* no se genero (comprobar, o la compilacion tuvo errores)   */
#define MF_GEN_OK       1       /* generado y escrito entero                                 */
#define MF_GEN_ERR      2       /* fallo al generar o al escribir: mf_mensaje(mf.gen_cod)    */

typedef struct {
    uint16_t linea;             /* linea fisica del fichero (1..)            */
    uint8_t  columna;           /* 1..72 (255 = saturada)                    */
    uint8_t  cod;               /* codigo, ver arriba                        */
} MfDiag;

typedef struct {
    uint16_t n;                 /* errores totales de la ultima compilacion  */
    MfDiag   diag[MF_DIAG_MAX]; /* los primeros MF_DIAG_MAX                  */
    uint8_t  gen;               /* MF_GEN_NO / MF_GEN_OK / MF_GEN_ERR        */
    uint8_t  gen_cod;           /* con MF_GEN_ERR: codigo para mf_mensaje    */
} Mf;

/* Salida del .nex, PROPORCIONADA POR EL EDITOR (esxDOS). Las tres funciones deben ser RESIDENTES: se llaman
 * desde un banco del compilador, que ocupa la misma ventana que el banco 41 del editor.
 *   abrir():      crea/abre el fichero (nombre, sobrescritura y avisos al usuario los decide el editor);
 *                 devuelve 1 si pudo y 0 si no (entonces no se llama a escribir ni a cerrar).
 *   escribir():   escribe n bytes (n <= 64, el buffer vive en la pila); 1 = bien, 0 = fallo.
 *   cerrar(ok):   cierra; ok = 0 si hubo un fallo (el editor puede borrar el fichero a medias).
 *                 Devuelve 1 si el cierre fue bien. */
typedef struct {
    uint8_t (*abrir)(void);
    uint8_t (*escribir)(const void *datos, uint16_t n);
    uint8_t (*cerrar)(uint8_t ok);
} MfSalida;

/* Registra la salida (NULL = solo comprobar). No se copia: debe seguir viva. */
void mf_salida(const MfSalida *s);

extern Mf mf;

/* PROPORCIONADA POR EL EDITOR (o por platform/texto.c en pruebas y medidas). */
int ed_linea(uint16_t n, char *dst);

/* Compila el fuente que sirve ed_linea. Devuelve mf.n. */
uint16_t mf_compilar(void);

/* Copia en dst (MF_MENSAJE_MAX bytes) el texto del codigo cod. */
void mf_mensaje(uint8_t cod, char *dst);

/* Uso interno de las fases del compilador: anotan un diagnostico. */
void mf_diag_iniciar(void);
void mf_diag(uint8_t cod, uint16_t linea, uint16_t columna);
void mf_diag_pos(uint8_t cod, uint16_t pos_bruta);   /* pos de la sentencia actual */

#endif
