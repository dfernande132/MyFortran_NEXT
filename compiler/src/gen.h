/* gen.h - Generador de codigo de tres direcciones (decision 0011, docs/especificacion-tac.md).
 *
 * Una sola pasada: sem.c llama a estas funciones MIENTRAS la sentencia esta viva
 * (su AST con los tipos ya rellenados), asi no hay que guardar el programa.
 *
 *   gen_iniciar()          al empezar la unidad (lo llama sem_iniciar)
 *   gen_etiqueta()         la sentencia lleva etiqueta: emite LBL (antes de su codigo)
 *   gen_sentencia()        la sentencia ya esta tipada y sin errores: emite su codigo
 *   gen_print_item()       un elemento de la lista de PRINT (en flujo, ver stmt.h)
 *   gen_goto_etq()         una etiqueta de la lista del GOTO calculado (en flujo)
 *   gen_fin_sentencia()    tras la sentencia: cierra IF logico y DO que acaban en ella
 *
 * No genera nada si ya hay algun error (mf.n != 0) o si se acabo el espacio:
 * el TAC de un programa con errores no sirve. Lo que el generador aun no sabe
 * hacer (REAL, intrinsecas no enteras, FORMAT, ...) se anota como error normal.
 *
 * gen_activar(0) lo deja inerte: lo usan las pruebas del semantico solo (capitulos 5).
 * mf_compilar() lo activa.
 */
#ifndef MF_GEN_H
#define MF_GEN_H

#include <stdint.h>

#define GEN_PAGINA        2          /* comparte pagina con la tabla de etiquetas (sem.h) */
#define GEN_BLOQUES_OFF   4096       /* pila de bloques: 16 x 10 B                        */
#define GEN_GOTO_OFF      4352       /* etiquetas del GOTO calculado: 64 x 2 B            */
#define GEN_GOTO_MAX      64
#define GEN_ARG_OFF       4608       /* argumentos del CALL en curso: UNI_ARGS_MAX x 8 B */
#define GEN_SF_OFF        5632       /* funciones de sentencia de la unidad (sem.c): registros + cuerpos tipados */
#define GEN_SF_TAM        2560       /* hasta el final de la pagina (8192)                                       */

void gen_activar(uint8_t si);
void gen_iniciar(void);
/* Primera sentencia de una unidad (sem.c): emite ENTER. Los temporales, las palabras ocultas y las etiquetas internas
 * son del PROGRAMA: cada unidad usa los suyos, distintos de los de las demas (una llamada en mitad de una expresion
 * no puede pisar los temporales de quien llama). clase: UK_PROGRAMA / UK_SUBRUTINA (unid.h). */
void gen_unidad(uint8_t clase, uint8_t uid);
/* CALL: gen_llamada_inicio(unidad llamada); por cada argumento (ya tipado, en stm.e) gen_llamada_arg(k); y al final
 * gen_sentencia() emite las copias de entrada, el CALL y las copias de salida. */
void gen_llamada_inicio(uint8_t uid);
void gen_llamada_arg(uint8_t k);
void gen_etiqueta(uint32_t etiq);
void gen_sentencia(void);
void gen_print_item(void);
void gen_param_item(void);
void gen_read(uint8_t item);        /* READ: 0 = empieza la sentencia (RDLN), 1 = un elemento de la lista (stm.e) */
void gen_goto_etq(uint32_t etiq, uint16_t pos);
void gen_fin_sentencia(void);

/* Nombre de la unidad (offset en la tabla de cadenas; 0 = sin PROGRAM). */
uint16_t gen_nombre(void);

/* Listado en texto (tac_lst.c): una llamada a sal() por linea. */
void tac_listar(void (*sal)(const char *linea));

#endif
