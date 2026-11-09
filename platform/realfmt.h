/* realfmt.h - Referencia de PC del FORMATO de un REAL en PRINT * y de las reglas de la aritmetica (decision 0013).
 *
 * real_formato(): lo que imprime el programa generado, al estilo gfortran con 9 cifras significativas:
 *   0.1 -> "0.100000001", 100 -> "100.000000", 123456792 -> "123456792.", 1E10 -> "1.00000000E+10", 0.01 -> "9.99999978E-03".
 * Entre 0.1 y 1E9 (sin llegar) va en notacion fija, fuera de ese rango en notacion E; el cero es "0.00000000". Sin el blanco inicial
 * (PRINT pone uno antes de cada elemento) y con "-" si el bit de signo esta puesto (tambien en -0).
 *
 * Reglas de la aritmetica REAL del programa generado (no hay denormales, infinitos ni NaN):
 *   - redondeo al par; un resultado menor que el menor normal (1.17549435E-38) vale 0, con el signo que le tocaria;
 *   - ERRORES DE EJECUCION (el programa imprime el mensaje y PARA; ANSI X3.9-1978 6.6 prohibe "dividing by zero"; el desbordamiento
 *     es dependiente del procesador y aqui es tambien un error): x/0 -> "Division by zero"; 0/0 -> "0/0 is indeterminate";
 *     resultado de mas de 3.40282347E+38 -> "REAL overflow". Lo mismo en INTEGER: n/0 y 0/0.
 *   - el resultado se redondea a 24 bits de mantisa SIN limite inferior del exponente, y despues, si es menor que el menor normal, vale 0
 *     (el float del PC redondea en cambio a un denormal: difiere solo cuando el resultado exacto cae a menos de 2^-150 del menor normal).
 */
#ifndef MF_REALFMT_H
#define MF_REALFMT_H

#include <stdint.h>

void     real_formato(uint32_t bits, char *dst);              /* dst: 20 B */
uint32_t real_regla(float resultado, int zero_negativo);       /* aplica las reglas de arriba a un resultado en float */
/* Errores de ejecucion (los codigos son los que recibe la rutina RT_ERR del programa generado). */
#define RERR_NINGUNO  0
#define RERR_DIV0     1
#define RERR_0DIV0    2
#define RERR_OVF      3
#define RERR_SQRTNEG  4
#define RERR_POW0     5
#define RERR_ENTRADA  6       /* READ: dato de entrada no valido */
#define RERR_FINENT   7       /* READ: se acabo la entrada (solo en las pruebas: el teclado no se acaba) */
const char *real_err_texto(int codigo);                       /* el mensaje que imprime el programa (sin el salto de linea) */

/* a (op) b con las reglas de arriba; op = '+', '-', '*', '/'. Devuelve RERR_*; si es RERR_NINGUNO, *r es el resultado.
 * Es la referencia de la tacvm y de las pruebas del runtime del Z80. */
int real_op(char op, uint32_t a, uint32_t b, uint32_t *r);
/* AINT (k=0), ANINT (k=1, mitades lejos del cero) y SQRT (k=2): RERR_SQRTNEG si x < 0. */
int real_fun(int k, uint32_t x, uint32_t *r);
/* AMOD exacto (el signo es el de x; p = 0: RERR_DIV0 / RERR_0DIV0). */
int real_mod(uint32_t x, uint32_t p, uint32_t *r);
/* x ** n (n entero): como __powisf2 de libgcc; 0 ** (n <= 0): RERR_POW0. */
int real_powi(uint32_t x, int16_t n, uint32_t *r);

#endif
