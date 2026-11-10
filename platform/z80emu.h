/* z80emu.h - Emulador Z80 MINIMO para las pruebas de PC (no se enlaza en el Next).
 *
 * Implementa solo las instrucciones que emite back.c; cualquier otro codigo de
 * operacion termina la ejecucion con Z80EMU_OPCODE, asi un cambio en el backend que
 * use una instruccion nueva obliga a anadirla aqui a proposito. RST 16 se
 * intercepta: escribe el registro A en la salida (13 = salto de linea).
 */
#ifndef MF_Z80EMU_H
#define MF_Z80EMU_H

#include <stddef.h>
#include <stdint.h>

#define Z80EMU_OK       0       /* el programa volvio (RET a la direccion 0)  */
#define Z80EMU_OPCODE   (-1)    /* instruccion no implementada                */
#define Z80EMU_LIMITE   (-2)    /* demasiados pasos                           */

typedef struct {
    int      estado;
    uint16_t pc;                /* donde se paro                              */
    uint8_t  op;
    uint32_t pasos;
    uint32_t ciclos_aprox;      /* numero de instrucciones ejecutadas x 1 (solo referencia) */
    uint16_t pila_usada;        /* maximo de bytes de pila usados (SP inicial - SP minimo; incluye la direccion de retorno) */
} Z80EmuRes;

/* mem: 65536 bytes con el programa ya cargado. Se llama a `pc` como subrutina. */
void z80emu_ejecutar(uint8_t *mem, uint16_t pc, uint32_t max_pasos,
                     char *salida, size_t n, Z80EmuRes *res);

#endif
