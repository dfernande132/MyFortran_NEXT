/* test_mf.c - Pruebas del punto de entrada mf_compilar / mf_mensaje (mf_api.h). */
#include <stdio.h>
#include <string.h>
#include "mf_api.h"
#include "source.h"
#include "texto.h"
#include "sem.h"
#include "banco.h"

static int pruebas = 0;
static int fallos = 0;

#define COMPROBAR(cond)                                              \
    do {                                                             \
        pruebas++;                                                   \
        if (!(cond)) {                                               \
            fallos++;                                                \
            printf("FALLO (linea %d): %s\n", __LINE__, #cond);       \
        }                                                            \
    } while (0)

static uint16_t compilar(const char *texto)
{
    banco_iniciar();
    texto_poner(texto);
    return mf_compilar();
}

/* mf.diag[i] == (linea, columna, cod) */
static int diag_es(int i, uint16_t lin, uint8_t col, uint8_t cod)
{
    int ok = mf.diag[i].linea == lin && mf.diag[i].columna == col && mf.diag[i].cod == cod;

    if (!ok) {
        printf("   diag[%d] = %u:%u cod %u (esperado %u:%u cod %u)\n", i,
               (unsigned)mf.diag[i].linea, (unsigned)mf.diag[i].columna,
               (unsigned)mf.diag[i].cod, (unsigned)lin, (unsigned)col, (unsigned)cod);
    }
    return ok;
}

static void test_ed_linea(void)
{
    char b[80];

    texto_poner("AB\nCD\n\nEF");
    COMPROBAR(ed_linea(1, b) == 1 && strcmp(b, "AB") == 0);
    COMPROBAR(ed_linea(2, b) == 1 && strcmp(b, "CD") == 0);
    COMPROBAR(ed_linea(3, b) == 1 && b[0] == '\0');          /* linea vacia existe */
    COMPROBAR(ed_linea(4, b) == 1 && strcmp(b, "EF") == 0);  /* sin '\n' final */
    COMPROBAR(ed_linea(5, b) == 0);
    COMPROBAR(ed_linea(2, b) == 1 && strcmp(b, "CD") == 0);  /* hacia atras */
    texto_poner("A\n");
    COMPROBAR(ed_linea(1, b) == 1 && ed_linea(2, b) == 0);   /* tras el ultimo '\n' no hay linea */
    texto_poner("");
    COMPROBAR(ed_linea(1, b) == 0);
    texto_poner("X\r\nY\r\n");
    COMPROBAR(ed_linea(1, b) == 1 && strcmp(b, "X") == 0);
    texto_poner("0123456789012345678901234567890123456789012345678901234567890123456789012345678\n");
    COMPROBAR(ed_linea(1, b) == 1 && strlen(b) == 72);        /* 72 columnas como mucho */
}

static void test_sin_errores(void)
{
    COMPROBAR(compilar("      INTEGER I\n      I = 1\n      END\n") == 0);
    COMPROBAR(mf.n == 0);
    COMPROBAR(compilar("") == 1);                  /* vacio: falta END */
    COMPROBAR(mf.diag[0].cod == SEM_FALTA_END);
    /* Tabuladores y CR/LF que pudieran llegar de otras fuentes */
    COMPROBAR(compilar("\tI = 1\r\n\tEND\r\n") == 0);
    COMPROBAR(compilar("      I = 1 +\n\t1 2\n      END\n") == 0);
}

static void test_columnas_73(void)
{
    /* Todo lo que pase de la columna 72 se ignora (incluso basura) */
    COMPROBAR(compilar("      I = 1                                                                  @@@ ((\n      END\n") == 0);
}

static void test_errores(void)
{
    /* error de expresion (sintaxis) */
    COMPROBAR(compilar("      INTEGER I\n      I = (1\n      END\n") == 1);
    COMPROBAR(diag_es(0, 2, 13, MF_COD_EXPR + 2));           /* EXPR_PAR_CIERRA */
    /* error lexico */
    COMPROBAR(compilar("      I = 2 @ 3\n      END\n") == 1);
    COMPROBAR(mf.diag[0].cod == MF_COD_LEX + 0 && mf.diag[0].linea == 1);
    /* error de sentencia */
    COMPROBAR(compilar("      GOTO\n      END\n") == 1);
    COMPROBAR(mf.diag[0].cod >= MF_COD_STMT && mf.diag[0].cod < MF_COD_EXPR && mf.diag[0].linea == 1);
    /* error semantico */
    COMPROBAR(compilar("      INTEGER A\n      REAL A\n      END\n") == 1);
    COMPROBAR(diag_es(0, 2, 12, SEM_YA_DEFINIDO));
    /* de fuente */
    COMPROBAR(compilar("     +X\n      END\n") == 1);
    COMPROBAR(diag_es(0, 1, 6, MF_COD_FUENTE + FUENTE_CONTINUACION_INVALIDA));
    COMPROBAR(compilar("AB    X = 1\n      END\n") == 1);
    COMPROBAR(diag_es(0, 1, 1, MF_COD_FUENTE + FUENTE_ETIQUETA_INVALIDA));
}

static void test_orden_y_saturacion(void)
{
    /* Orden de aparicion: sem (l.2), sintaxis (l.3), lexico (l.4), sem al final (falta END) */
    COMPROBAR(compilar("      INTEGER A\n      REAL A\n      A = (1\n      A = 2 @ 3\n") == 4);
    COMPROBAR(mf.diag[0].linea == 2 && mf.diag[0].cod == SEM_YA_DEFINIDO);
    COMPROBAR(mf.diag[1].linea == 3 && mf.diag[1].cod == MF_COD_EXPR + 2);
    COMPROBAR(mf.diag[2].linea == 4 && mf.diag[2].cod == MF_COD_LEX);
    COMPROBAR(mf.diag[3].cod == SEM_FALTA_END);

    /* mas errores que huecos: se cuentan todos, se guardan los 16 primeros */
    {
        static char prog[20 * 14 + 16];
        int i;

        prog[0] = '\0';
        for (i = 0; i < 20; i++) {
            strcat(prog, "      I = (1\n");
        }
        strcat(prog, "      END\n");
        COMPROBAR(compilar(prog) == 20);
        COMPROBAR(mf.n == 20);
        COMPROBAR(mf.diag[0].linea == 1 && mf.diag[15].linea == 16);
    }

    /* una compilacion no hereda errores de la anterior */
    COMPROBAR(compilar("      I = (1\n      END\n") == 1);
    COMPROBAR(compilar("      I = 1\n      END\n") == 0);
    COMPROBAR(mf.diag[0].cod == 0 || mf.n == 0);
}

static void test_mensajes(void)
{
    char b[MF_MENSAJE_MAX];
    uint8_t cod;
    int c, bien = 1;
    static const struct { uint8_t ini, fin; } rangos[] = {
        { 1, SEM_DESPUES_END },                                 /* ErrorSem */
        { MF_COD_FUENTE + 2, MF_COD_FUENTE + 4 },
        { MF_COD_STMT + 1, MF_COD_STMT + 17 },
        { MF_COD_EXPR + 1, MF_COD_EXPR + 10 },
        { MF_COD_LEX, MF_COD_LEX + 6 }
    };
    unsigned r;

    for (r = 0; r < sizeof(rangos) / sizeof(rangos[0]); r++) {
        for (c = rangos[r].ini; c <= rangos[r].fin; c++) {
            cod = (uint8_t)c;
            memset(b, 'x', sizeof(b));
            mf_mensaje(cod, b);
            if (b[0] == '\0' || strcmp(b, "unknown error") == 0
                || memchr(b, '\0', sizeof(b)) == NULL) {
                printf("   mensaje del codigo %d: '%s'\n", c, b);
                bien = 0;
            }
        }
    }
    COMPROBAR(bien);
    mf_mensaje(SEM_YA_DEFINIDO, b);
    COMPROBAR(strcmp(b, "name already defined") == 0);
}

int main(void)
{
    test_ed_linea();
    test_sin_errores();
    test_columnas_73();
    test_errores();
    test_orden_y_saturacion();
    test_mensajes();
    printf("%d pruebas, %d fallos\n", pruebas, fallos);
    return fallos != 0;
}
