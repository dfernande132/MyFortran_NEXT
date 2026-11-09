/* c4_banco46.c - Capitulo 4: FASE "lectura" en el banco 46 = source + lexer + clasif.
 *
 * Unity build: el codigo real del compilador se incluye tal cual, sin tocarlo;
 * solo se le antepone el pragma de banco. Todas las llamadas entre estos tres
 * modulos son directas (mismo banco). Su estado global (sentencia, lexer,
 * cadenas) queda en el area residente (bss), como se decidio.
 */
#pragma bank 46

#include "../../compiler/src/source.c"
#include "../../compiler/src/lexer.c"
/* lexer.c y clasif.c definen cada uno su es_letra/es_digito (static): al
 * juntarlos en una unidad de compilacion chocarian. Se renombran los de clasif. */
#define es_letra  clasif_es_letra
#define es_digito clasif_es_digito
#include "../../compiler/src/clasif.c"
#undef es_letra
#undef es_digito
