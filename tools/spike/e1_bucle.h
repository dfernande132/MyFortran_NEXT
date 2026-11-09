/* e1_bucle.h - Gancho de PRUEBAS de la integracion (ya no es el bucle: ese es editor/src/ed_bucle.c).
 * ED_GANCHO_TECLA(c) se evalua con cada tecla antes de ed_tecla; si da distinto de 0 la tecla ya esta atendida.
 *
 *   Esc y despues M   mide: fin del bss residente, pila usada, mapeos de banco
 *
 * Comprobar (Esc C) y construir (Esc O) son ahora del editor (edit.c); el nombre del .nex lo decide el editor
 * (el del fuente con .nex; si no tiene nombre lo pide). Se pierde el tiempo en ms que mostraba el modelo. */

extern unsigned long c4_mapeos;                  /* residente (trampolines) */
extern unsigned char _BSS_END_tail;
extern unsigned int e1_pila_usada(unsigned int *rango);


static void mb_hex4(uint16_t v)
{
    uint8_t i, d;

    for (i = 0; i < 4 && nmb < sizeof(msgbuf) - 1; i++) {
        d = (uint8_t)((v >> (12 - 4 * i)) & 15);
        msgbuf[nmb++] = (char)(d < 10 ? '0' + d : 'A' + d - 10);
    }
}

static void medidas(void)
{
    unsigned int rango, usada = e1_pila_usada(&rango);

    nmb = 0;
    mb_cad("bss end $");
    mb_hex4((uint16_t)&_BSS_END_tail);
    mb_cad("  stack ");
    mb_num(usada);
    mb_cad("/");
    mb_num(rango);
    mb_cad("  maps ");
    mb_num((uint16_t)c4_mapeos);
    mb_fin();
    ed.msg_err = 0;
}

static uint8_t e1_gancho(int c)
{
    if (ed.modo == M_COMANDO && (c == 'M' || c == 'm')) {
        ed.modo = M_NORMAL;
        medidas();
        return 1;
    }
    return 0;
}

#define ED_GANCHO_TECLA(c) e1_gancho(c)
