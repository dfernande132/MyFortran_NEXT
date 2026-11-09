/* sim.c - Tabla de simbolos (ver sim.h). Sin malloc; registros por copia. */
#include <string.h>
#include "sim.h"
#include "banco.h"
#include "lexer.h"

/* Cabeceras del hash (id del primero de cada cadena, 0 = vacia): al final de la pagina de dimensiones, no en el
 * residente del Next. Las dimensiones ocupan el principio de esa pagina. */
#define CUB_OFF  ((uint16_t)(BANCO_PAGINA_TAM - 2 * SIM_CUBETAS))

static uint16_t cub_get(uint8_t h)
{
    uint16_t v;

    banco_leer(SIM_PAGINA_DIM, (uint16_t)(CUB_OFF + 2 * h), &v, 2);
    return v;
}

static void cub_set(uint8_t h, uint16_t v)
{
    banco_escribir(SIM_PAGINA_DIM, (uint16_t)(CUB_OFF + 2 * h), &v, 2);
}

static uint16_t cuenta;
static uint16_t cuenta_dim;

/* Los nombres de una unidad no son visibles desde la siguiente: se vacian las cabeceras del hash. Los registros
 * (y sus id) se conservan: el backend reparte memoria a las variables de TODAS las unidades. */
void sim_unidad(void)
{
    uint8_t h;

    for (h = 0; h < SIM_CUBETAS; h++) {
        cub_set(h, 0);
    }
}

void sim_iniciar(void)
{
    sim_unidad();
    cuenta = 0;
    cuenta_dim = 0;
}

uint16_t sim_cuenta(void)
{
    return cuenta;
}

static uint8_t hash(const char nombre[NOMBRE_MAX])
{
    uint8_t h = 0;
    uint8_t i;

    for (i = 0; i < NOMBRE_MAX && nombre[i] != '\0'; i++) {
        h = (uint8_t)((h << 1) + (uint8_t)nombre[i]);
    }
    return (uint8_t)(h & (SIM_CUBETAS - 1));
}

/* Nombre del llamador a formato de registro (relleno con '\0'). 0 si no cabe. */
static uint8_t normalizar(const char *nombre, char dst[NOMBRE_MAX])
{
    uint8_t n = 0;

    memset(dst, 0, NOMBRE_MAX);
    while (nombre[n] != '\0') {
        if (n == NOMBRE_MAX) {
            return 0;
        }
        dst[n] = nombre[n];
        n++;
    }
    return n != 0;
}

void sim_leer(uint16_t id, Simbolo *s)
{
    banco_leer(SIM_PAGINA, (uint16_t)((id - 1) * sizeof(Simbolo)), s, sizeof(Simbolo));
}

void sim_escribir(uint16_t id, const Simbolo *s)
{
    banco_escribir(SIM_PAGINA, (uint16_t)((id - 1) * sizeof(Simbolo)), s, sizeof(Simbolo));
}

uint16_t sim_buscar(const char *nombre)
{
    char n[NOMBRE_MAX];
    Simbolo s;
    uint16_t id;

    if (!normalizar(nombre, n)) {
        return 0;
    }
    for (id = cub_get(hash(n)); id != 0; id = s.sig) {
        sim_leer(id, &s);
        if (memcmp(s.nombre, n, NOMBRE_MAX) == 0) {
            return id;
        }
    }
    return 0;
}

uint16_t sim_alta(const char *nombre, uint8_t tipo, uint8_t clase)
{
    Simbolo s;
    uint8_t h;

    if (cuenta >= SIM_MAX || !normalizar(nombre, s.nombre)
        || sim_buscar(nombre) != 0) {
        return 0;
    }
    h = hash(s.nombre);
    s.tc = SIM_TC(tipo, clase);
    s.flags = 0;
    s.sig = cub_get(h);
    s.extra = 0;
    s.dir = 0;
    cuenta++;
    sim_escribir(cuenta, &s);
    cub_set(h, cuenta);
    return cuenta;
}

uint16_t sim_dim_nueva(const Dim *d)
{
    uint16_t i = cuenta_dim;

    if ((uint16_t)((i + 1) * sizeof(Dim)) > TABLA_OFF) {      /* la tabla de cadenas (lexer.h) ocupa la pagina desde TABLA_OFF */
        return 0xFFFF;
    }
    banco_escribir(SIM_PAGINA_DIM, (uint16_t)(i * sizeof(Dim)), d, sizeof(Dim));
    cuenta_dim++;
    return i;
}

void sim_dim_leer(uint16_t indice, Dim *d)
{
    banco_leer(SIM_PAGINA_DIM, (uint16_t)(indice * sizeof(Dim)), d, sizeof(Dim));
}
