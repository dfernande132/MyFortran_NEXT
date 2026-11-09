/* tacvm.h - Interprete del TAC para las pruebas de PC (no se enlaza en el Next).
 *
 * Ejecuta el codigo que dejo gen.c en las paginas de datos, con la semantica de
 * docs/especificacion-tac.md. Sirve para comprobar que el TAC HACE lo que debe
 * (el listado solo dice como se ve). Tambien valida la estructura: etiquetas
 * definidas una sola vez, saltos a etiquetas que existen, temporales CHARACTER
 * declarados con TMP, indices dentro del array, EXT donde corresponde.
 */
#ifndef MF_TACVM_H
#define MF_TACVM_H

#include <stddef.h>
#include <stdint.h>

#define TACVM_HALT   0
#define TACVM_STOP   1
#define TACVM_ERREJEC 2            /* error de ejecucion (division por cero, 0/0, overflow REAL): el mensaje ya esta en la salida */
#define TACVM_ERROR  (-1)

typedef struct {
    int      estado;           /* TACVM_HALT / STOP / ERREJEC / ERROR                     */
    uint32_t pasos;            /* instrucciones ejecutadas                      */
    char     msg[96];          /* ERROR: motivo; STOP: argumento si lo hay      */
} TacVmRes;

/* Texto que "teclea" el programa en los READ: lineas separadas por '\n'. Se eco-a en la salida como en el Next (cada linea leida,
 * seguida de un salto de linea). NULL = sin entrada. Debe llamarse antes de tacvm_ejecutar. */
void tacvm_entrada(const char *texto);

/* Comprueba la estructura. 0 = bien; si no, escribe el motivo en msg. */
int  tacvm_validar(char *msg, size_t n);

/* Ejecuta (max_pasos: limite contra bucles infinitos). La salida de PUT/PUTNL
 * se acumula en salida (terminada en '\0', se trunca si no cabe). */
void tacvm_ejecutar(char *salida, size_t n, uint32_t max_pasos, TacVmRes *res);

#endif
