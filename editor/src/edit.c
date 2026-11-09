/* edit.c - Logica del editor (ver edit.h). */
#include <string.h>
#include "edbuf.h"
#include "ed_plat.h"
#include "ed_teclas.h"
#include "edit.h"
#ifdef ED_CON_COMPILADOR
#include "mf_api.h"          /* mf_compilar, mf_mensaje, mf, mf_salida (compiler/include) */
#include "ed_salida.h"      /* ed_salida, ed_nex_nombre */
#endif

Editor ed;

/* Buffer de una fila de pantalla (texto o barra de estado). Se usa de forma
 * secuencial: nadie lo necesita mientras otro lo tiene a medias. */
static uint8_t lin[ED_ANCHO];
static uint8_t est[ED_ANCHO];       /* estilo de cada caracter de lin */
static uint8_t n_lin;
static uint8_t est_act;             /* estilo con el que escriben s_car/s_cad */

static const char msg_ayuda[]    = " Esc, then: H/E T/B U/D G goto K del   C check O build X/Z err   N/L/S/A file";
static const char msg_comando[]  = " CMD: H/E line  T/B text  U/D page  G goto  K delete line";
static const char msg_comando2[] = " C check  O build  X/Z next/prev error  N new  L load  S save  A save as";
#ifdef ED_CON_COMPILADOR
static const char msg_compilando[] = "Checking ...";
static const char msg_construyendo[] = "Building ...";
static const char msg_compilado[]  = "Checked OK: no errors";
static const char msg_sin_gen[]    = "Checked OK, nothing built";
static const char msg_gen_ok[]     = "Built ";
static const char msg_gen_mal[]    = "Build failed: ";
static const char msg_mismo[]      = "Output name is the source file";
static const char msg_sin_errores[] = "No errors to show (Esc C checks)";
static const char msg_texto_nuevo[] = "Text changed: check again (Esc C)";
#else
static const char msg_sin_comp[]   = "No compiler in this build";
#endif
static const char msg_pedir[]    = " Type the file name     ENTER accept     DELETE erase     Esc cancel";
static const char msg_confirma[] = " Y = yes      any other key = cancel";
static const char msg_cancelado[] = "Cancelled";
static const char msg_sin_nombre[] = "No file name";
static const char msg_nuevo[]    = "New file";
static const char msg_perder[]   = "Unsaved changes will be lost. Continue? (Y/N)";
static const char msg_existe[]   = "File exists. Overwrite? (Y/N)";
static const char msg_parcial[]  = "File was only partly loaded: saving LOSES the rest. Save anyway? (Y/N)";
static const char et_cargar[]    = "Load file: ";
static const char et_guardar[]   = "Save as: ";
static const char et_ir[]        = "Go to line: ";
static const char et_nex[]       = "Build as: ";

/* Estados de ed.modo */
#define M_NORMAL   0
#define M_COMANDO  1
#define M_NOMBRE   2     /* escribiendo el nombre de un fichero (entrada[]) */
#define M_CONFIRMA 3     /* esperando Y para seguir */

/* Que se esta haciendo en M_NOMBRE / M_CONFIRMA */
#define A_NUEVO    1
#define A_CARGAR   2
#define A_GUARDAR  3
#define A_PARCIAL  5     /* guardar encima de un fichero que se cargo incompleto */
#define A_IR       4     /* ir a una linea: entrada[] solo lleva cifras */
#define A_NEX      6     /* nombre del .nex a construir */

/* Nombre del fichero actual (ed.nombre apunta aqui, o es 0 si no tiene) y lo que se
 * teclea en la ultima fila. Estan FUERA de `ed`: ed_iniciar borra `ed` y estos nombres
 * se siguen usando justo despues (cargar el fichero que acaba de teclearse). */
static char fichero[ED_NOMBRE_MAX + 1];
static char entrada[ED_NOMBRE_MAX + 1];
static uint8_t n_ent;
static uint8_t accion;
static uint8_t incompleto;    /* el fichero cargado no cupo entero (o fallo la lectura): guardarlo encima perderia datos */

static void ir_entrada(void);          /* G: salta a la linea tecleada (mas abajo, usa msgbuf) */
static void lanzar(uint8_t construir_nex);  /* C / O: comprueba (o construye) y muestra el primer error */
static void construir(void);           /* O: decide el nombre del .nex y lanza */
static void otro_error(int8_t dir);    /* X / Z: siguiente / anterior error */
static const char msg_ignorado[] = "Unknown command";
static const char msg_lleno[]    = "Buffer full";
static const char msg_col[]      = "Line limit: column 72";

/* ------------------------------------------------------------------
 * Utilidades
 * ------------------------------------------------------------------ */

static uint16_t columna(void)
{
    return ed.pos - ed.ini_lin;
}

static void aviso(const char *m)
{
    ed.msg = m;
    ed.msg_err = 1;
}

/* ------------------------------------------------------------------
 * Edicion
 * ------------------------------------------------------------------ */

static void insertar_car(uint8_t c)
{
    if (columna() >= ED_MAXCOL) {
        aviso(msg_col);
        return;
    }
    if (!eb_insertar(ed.pos, c)) {
        aviso(msg_lleno);
        return;
    }
    ed.pos++;
    ed.col_obj = columna();
    ed.modificado = 1;
    ed.sucio_fila = 1;
}

/* Tabulador: paradas cada 3 columnas (0, 3, 6, 9...). En F77 de formato fijo el
 * codigo empieza en la columna 7 (indice 6), asi que dos Tab en una linea en blanco
 * llegan justo ahi y los siguientes sangran de 3 en 3. Devuelve cuantos blancos
 * hacen falta desde la columna col (de 1 a 3).
 * Lo usan el teclado y la carga de ficheros: un fichero con tabuladores queda
 * igual que si se hubieran tecleado aqui. (Decision de Dani, 2026-10-30: tabuladores
 * simples; antes se probo con paradas 5, 6, 9... y se descarto por complicado.) */
static uint8_t tab_espacios(uint16_t col)
{
    return (uint8_t)(3 - (col % 3));
}

static void insertar_tab(void)
{
    uint8_t k;

    for (k = tab_espacios(columna()); k != 0; k--) {
        if (columna() >= ED_MAXCOL) {
            aviso(msg_col);
            return;
        }
        insertar_car(' ');
        if (ed.msg != 0) return;          /* lleno */
    }
}

static void insertar_nl(void)
{
    if (!eb_insertar(ed.pos, '\n')) {
        aviso(msg_lleno);
        return;
    }
    ed.pos++;
    ed.linea++;
    ed.ini_lin = ed.pos;
    ed.col_obj = 0;
    ed.modificado = 1;
    ed.sucio_todo = 1;
}

static void borrar_atras(void)
{
    uint8_t c;

    if (ed.pos == 0) return;
    c = eb_byte(ed.pos - 1);
    eb_borrar(ed.pos - 1);
    ed.pos--;
    if (c == '\n') {
        /* Se unieron dos lineas: la del cursor es ahora la anterior. */
        ed.linea--;
        ed.ini_lin = eb_ini_linea(ed.pos);
        ed.sucio_todo = 1;
    } else {
        ed.sucio_fila = 1;
    }
    ed.col_obj = columna();
    ed.modificado = 1;
}

/* ------------------------------------------------------------------
 * Movimiento
 * ------------------------------------------------------------------ */

static void mover_izq(void)
{
    if (ed.pos == 0) return;
    ed.pos--;
    if (eb_byte(ed.pos) == '\n') {
        ed.linea--;
        ed.ini_lin = eb_ini_linea(ed.pos);
    }
    ed.col_obj = columna();
}

static void mover_der(void)
{
    if (ed.pos >= eb_longitud()) return;
    if (eb_byte(ed.pos) == '\n') {
        ed.pos++;
        ed.linea++;
        ed.ini_lin = ed.pos;
    } else {
        ed.pos++;
    }
    ed.col_obj = columna();
}

static void mover_arr(void)
{
    uint16_t ini, l;

    if (ed.linea == 0) return;
    ini = eb_ini_linea(ed.ini_lin - 1);       /* inicio de la linea anterior */
    l = ed.ini_lin - 1 - ini;                 /* su longitud */
    ed.pos = ini + ((ed.col_obj < l) ? ed.col_obj : l);
    ed.ini_lin = ini;
    ed.linea--;
}

static void mover_aba(void)
{
    uint16_t fin, ini, l;

    fin = eb_fin_linea(ed.pos);
    if (fin >= eb_longitud()) return;         /* ultima linea */
    ini = fin + 1;
    l = eb_fin_linea(ini) - ini;
    ed.pos = ini + ((ed.col_obj < l) ? ed.col_obj : l);
    ed.ini_lin = ini;
    ed.linea++;
}

static void ir_inicio_linea(void)
{
    ed.pos = ed.ini_lin;
    ed.col_obj = 0;
}

static void ir_fin_linea(void)
{
    ed.pos = eb_fin_linea(ed.pos);
    ed.col_obj = columna();
}

static void ir_inicio_texto(void)
{
    ed.pos = 0;
    ed.ini_lin = 0;
    ed.linea = 0;
    ed.col_obj = 0;
}

static void ir_fin_texto(void)
{
    uint16_t len;

    len = eb_longitud();
    ed.pos = len;
    ed.ini_lin = eb_ini_linea(len);
    ed.linea = eb_contar_nl(len);
    ed.col_obj = columna();
}

static void pagina(uint8_t abajo)
{
    uint8_t i;

    for (i = 0; i < ED_FILAS_TEXTO - 1; i++) {
        if (abajo) mover_aba(); else mover_arr();
    }
}

/* ------------------------------------------------------------------
 * Vista: que parte del texto se ve
 * ------------------------------------------------------------------ */

/* Pone la linea del cursor hacia la mitad de la pantalla. */
static void centrar(void)
{
    uint16_t p, n;

    p = ed.ini_lin;
    n = 0;
    while (n < ED_FILAS_TEXTO / 2 && p > 0) {
        p = eb_ini_linea(p - 1);
        n++;
    }
    ed.top = p;
    ed.top_linea = ed.linea - n;
}

static void ajustar_vista(void)
{
    uint16_t col;

    /* Vertical: un salto de una sola linea desplaza una linea; uno mayor, centra. */
    if (ed.linea < ed.top_linea) {
        if (ed.top_linea - ed.linea == 1) {
            ed.top = ed.ini_lin;
            ed.top_linea = ed.linea;
        } else {
            centrar();
        }
        ed.sucio_todo = 1;
    } else if (ed.linea >= ed.top_linea + ED_FILAS_TEXTO) {
        if (ed.linea - ed.top_linea == ED_FILAS_TEXTO) {
            ed.top = eb_fin_linea(ed.top) + 1;
            ed.top_linea++;
        } else {
            centrar();
        }
        ed.sucio_todo = 1;
    }

    /* Horizontal: al salirse, desplaza 8 columnas mas alla del cursor. */
    col = columna();
    if (col < ed.izq) {
        ed.izq = (col >= 8) ? col - 8 : 0;
        ed.sucio_todo = 1;
    } else if (col >= ed.izq + ED_ANCHO) {
        ed.izq = col + 9 - ED_ANCHO;
        ed.sucio_todo = 1;
    }
}

/* ------------------------------------------------------------------
 * Pintado
 * ------------------------------------------------------------------ */

static void s_car(uint8_t c)
{
    if (n_lin < ED_ANCHO) {
        est[n_lin] = est_act;
        lin[n_lin++] = c;
    }
}

static void s_cad(const char *s)
{
    while (*s != 0) s_car((uint8_t)*s++);
}

/* Escribe v en decimal en d (hasta 5 cifras, sin NUL, de mas a menos significativa)
 * y devuelve cuantas cifras son. */
static uint8_t num_txt(uint16_t v, char *d)
{
    char t[5];
    uint8_t i, n;

    i = 0;
    do {
        t[i++] = (char)('0' + (v % 10));
        v /= 10;
    } while (v != 0);
    n = i;
    while (i != 0) {
        *d++ = t[--i];
    }
    return n;
}

static void s_num(uint16_t v)
{
    char d[5];
    uint8_t i, n;

    n = num_txt(v, d);
    for (i = 0; i < n; i++) s_car((uint8_t)d[i]);
}

static void s_hex2(uint8_t v)
{
    static const char hex[] = "0123456789ABCDEF";

    s_car((uint8_t)hex[v >> 4]);
    s_car((uint8_t)hex[v & 15]);
}

/* Rellena con blancos del estilo est_act hasta n columnas y envia la fila entera. */
static void emitir_fila(uint8_t fila)
{
    while (n_lin < ED_ANCHO) {
        est[n_lin] = est_act;
        lin[n_lin++] = ' ';
    }
    plat_escribir(fila, 0, lin, est, ED_ANCHO);
}

/* Resaltado de F77 de formato fijo. Cada linea se clasifica sola (en F77 no hay
 * estado entre lineas): comentario si empieza por C, c o *; columnas 1-5 etiqueta;
 * columna 6 continuacion; 73 en adelante ignoradas; cadenas entre apostrofes;
 * palabras clave y numeros. s[0..n-1] es el trozo visible y col0 su columna absoluta
 * (con desplazamiento horizontal el trozo empieza a mitad de linea: la palabra o la
 * cadena cortada por el borde izquierdo se clasifica de forma aproximada). */
static const char *const claves[] = {
    "PROGRAM", "SUBROUTINE", "FUNCTION", "END", "INTEGER", "REAL", "DOUBLE",
    "PRECISION", "LOGICAL", "CHARACTER", "COMPLEX", "DIMENSION", "COMMON",
    "PARAMETER", "DATA", "IF", "THEN", "ELSE", "ELSEIF", "ENDIF", "DO",
    "CONTINUE", "GOTO", "CALL", "RETURN", "STOP", "PRINT", "READ", "WRITE",
    "FORMAT", "OPEN", "CLOSE", 0
};

static uint8_t es_letra(uint8_t c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static uint8_t es_digito(uint8_t c)
{
    return c >= '0' && c <= '9';
}

static void clasificar(const uint8_t *s, uint8_t n, uint16_t col0, uint8_t coment, uint8_t *e)
{
    uint8_t i, j, k, w, enstr, resta, c;
    uint16_t col;
    const char *kw;

    enstr = 0;
    resta = 0;
    for (i = 0; i < n; i++) {
        col = col0 + i;
        c = s[i];
        if (coment) { e[i] = ED_E_COMENT; continue; }
        if (col >= ED_MAXCOL) { e[i] = ED_E_FUERA; continue; }
        if (col < 5) { e[i] = es_digito(c) ? ED_E_ETIQ : ED_E_TEXTO; continue; }
        if (col == 5) { e[i] = (c != ' ') ? ED_E_CONT : ED_E_TEXTO; continue; }
        if (c == '\'') { e[i] = ED_E_CADENA; enstr = !enstr; continue; }
        if (enstr) { e[i] = ED_E_CADENA; continue; }
        if (resta != 0) { e[i] = ED_E_CLAVE; resta--; continue; }
        e[i] = ED_E_TEXTO;
        if (es_letra(c) && (i == 0 || !es_letra(s[i - 1]))) {
            for (j = i; j < n && es_letra(s[j]); j++) { }
            for (k = 0; claves[k] != 0; k++) {
                kw = claves[k];
                for (w = 0; kw[w] != 0 && (uint8_t)(i + w) < j
                            && (uint8_t)(s[i + w] & 0xDF) == (uint8_t)kw[w]; w++) { }
                if (kw[w] == 0 && (uint8_t)(i + w) == j) {
                    e[i] = ED_E_CLAVE;
                    resta = (uint8_t)(w - 1);
                    break;
                }
            }
        } else if (es_digito(c) && (i == 0 || !es_letra(s[i - 1]))) {
            e[i] = ED_E_NUM;
        }
    }
}

/* Pinta la linea de texto que empieza en p en la fila de pantalla dada.
 * Devuelve la posicion donde termina (su '\n' o el final del texto). */
static uint16_t dibujar_linea(uint8_t fila, uint16_t p)
{
    uint16_t fin, n, i;
    uint8_t coment;

    fin = eb_fin_linea(p);
    n = 0;
    if (fin - p > ed.izq) {
        n = fin - p - ed.izq;
        if (n > ED_ANCHO) n = ED_ANCHO;
        eb_leer(p + ed.izq, lin, n);
    }
    for (i = 0; i < n; i++) {
        if (lin[i] < 32 || lin[i] > 126) lin[i] = '.';
    }
    coment = 0;
    if (fin > p) {
        i = eb_byte(p);
        coment = (i == 'C' || i == 'c' || i == '*');
    }
    clasificar(lin, (uint8_t)n, ed.izq, coment, est);
    n_lin = (uint8_t)n;
    est_act = ED_E_TEXTO;
    emitir_fila(fila);
    return fin;
}

static void dibujar_texto(void)
{
    uint16_t p, len;
    uint8_t f;

    p = ed.top;
    len = eb_longitud();
    for (f = 0; f < ED_FILAS_TEXTO; f++) {
        if (p > len) {
            n_lin = 0;
            est_act = ED_E_TEXTO;
            emitir_fila(1 + f);
        } else {
            p = dibujar_linea(1 + f, p) + 1;
        }
    }
}

/* Barra de estado: titulo y fichero a la izquierda; a partir de la columna 40,
 * linea, columna, memoria libre, ultima tecla (hex) y estado. */
static void dibujar_estado(void)
{
    const char *s;

    n_lin = 0;
    est_act = ED_E_ESTADO;
    s_cad(" MYFORTRAN  ");
    s = ed.nombre;
    if (s != 0) {
        while (*s != 0 && n_lin < 38) s_car((uint8_t)*s++);
    }
    while (n_lin < 40) s_car(' ');
    s_cad("L:");
    s_num(ed.linea + 1);
    s_cad(" C:");
    s_num(columna() + 1);
    s_cad("  Free ");
    s_num(eb_libre());
    s_cad("  Key ");
    s_hex2((uint8_t)ed.ultima);
    if (ed.modo == M_COMANDO) s_cad("  CMD");
    else if (ed.modo != M_NORMAL) s_cad("  FILE");
    else if (ed.modificado) s_cad("  *");
    emitir_fila(0);
}

/* Texto que precede a lo tecleado en la ultima fila */
static const char *etiqueta(void)
{
    if (accion == A_CARGAR) return et_cargar;
    if (accion == A_IR) return et_ir;
    if (accion == A_NEX) return et_nex;
    return et_guardar;
}

static void dibujar_ayuda_y_mensaje(void)
{
    uint8_t i;

    n_lin = 0;
    est_act = ED_E_AYUDA;
    if (ed.modo == M_NOMBRE) s_cad(msg_pedir);
    else if (ed.modo == M_CONFIRMA) s_cad(msg_confirma);
    else s_cad((ed.modo != M_NORMAL) ? msg_comando : msg_ayuda);
    emitir_fila(ED_FILA_AYUDA);
    if (ed.modo == M_COMANDO) {                        /* segunda fila de ayuda: sustituye al mensaje */
        n_lin = 0;
        est_act = ED_E_AYUDA;
        s_cad(msg_comando2);
        emitir_fila(ED_FILA_MSG);
        return;
    }

    n_lin = 0;
    est_act = ed.msg_err ? ED_E_ERROR : ED_E_TEXTO;
    if (ed.modo == M_NOMBRE) {
        s_cad(" ");
        s_cad(etiqueta());
        for (i = 0; i < n_ent; i++) s_car((uint8_t)entrada[i]);
    } else if (ed.modo == M_CONFIRMA) {
        est_act = ED_E_ERROR;
        s_cad(" ");
        s_cad((accion == A_GUARDAR || accion == A_NEX) ? msg_existe : (accion == A_PARCIAL) ? msg_parcial : msg_perder);
    } else if (ed.msg != 0) {
        s_cad(" ");
        s_cad(ed.msg);
    }
    emitir_fila(ED_FILA_MSG);
}

void ed_refrescar(void)
{
    ajustar_vista();
    if (ed.sucio_todo) dibujar_texto();
    else if (ed.sucio_fila) dibujar_linea(1 + (uint8_t)(ed.linea - ed.top_linea), ed.ini_lin);
    ed.sucio_todo = 0;
    ed.sucio_fila = 0;
    dibujar_estado();
    dibujar_ayuda_y_mensaje();
    if (ed.modo == M_NOMBRE) {
        /* el cursor va al final de lo que se teclea, en la ultima fila */
        plat_cursor(ED_FILA_MSG, (uint8_t)(1 + strlen(etiqueta()) + n_ent));
    } else {
        plat_cursor(1 + (uint8_t)(ed.linea - ed.top_linea), (uint8_t)(columna() - ed.izq));
    }
}

/* ------------------------------------------------------------------
 * Teclas
 * ------------------------------------------------------------------ */

/* Ficheros: menus de nuevo / cargar / guardar */

static void fijar_nombre(const char *n)
{
    uint8_t i;

    for (i = 0; i < ED_NOMBRE_MAX && n[i] != 0; i++) fichero[i] = n[i];
    fichero[i] = 0;
    ed.nombre = fichero;
}

static void pedir_nombre(uint8_t a, const char *inicial)
{
    uint8_t i;

    accion = a;
    n_ent = 0;
    if (inicial != 0) {
        for (i = 0; i < ED_NOMBRE_MAX && inicial[i] != 0; i++) entrada[n_ent++] = inicial[i];
    }
    ed.modo = M_NOMBRE;
}

static uint8_t fichero_existe(const char *n)
{
    int h;

    h = plat_fich_abrir_lectura(n);
    if (h < 0) return 0;
    plat_fich_cerrar(h);
    return 1;
}

/* Guarda en el fichero n. Si el texto viene de una carga incompleta y se va a pisar ESE
 * fichero, antes pregunta (en otro nombre no hay riesgo: el original no se toca). */
static void guardar_en(const char *n)
{
    uint8_t i;

    if (incompleto && ed.nombre != 0 && strcmp(n, fichero) == 0) {
        if (n != entrada) {
            for (i = 0; i < ED_NOMBRE_MAX && n[i] != 0; i++) entrada[i] = n[i];
            entrada[i] = 0;
        }
        accion = A_PARCIAL;
        ed.modo = M_CONFIRMA;
        return;
    }
    ed_guardar(n);
}

/* Nombre del .nex (Esc O). Se fija al construir: el del fuente con extension .nex, o el
 * que se teclee si el texto no tiene nombre. nex_fijado = ya esta decidido y no se vuelve
 * a preguntar (hasta Nuevo/Cargar). Sin compilador no hay nada de esto. */
static uint8_t nex_fijado;
static void nex_confirmado(void);      /* Y al "File exists" del .nex */
static void nex_entrada(void);         /* Enter en "Build as:" */

/* La confirmacion ha sido Y: se hace lo que estaba esperando. */
static void confirmado(void)
{
    if (accion == A_NUEVO) ed_nuevo();
    else if (accion == A_CARGAR) pedir_nombre(A_CARGAR, 0);
    else if (accion == A_NEX) nex_confirmado();
    else ed_guardar(entrada);
}

/* Teclas mientras se escribe el nombre. */
static void tecla_nombre(int c)
{
    if (c == ED_K_MENU || c == ED_K_MENU2) {
        ed.modo = M_NORMAL;
        ed.msg = msg_cancelado;
    } else if (c == ED_K_ENTER) {
        ed.modo = M_NORMAL;
        if (n_ent == 0) {
            aviso(msg_sin_nombre);
            return;
        }
        entrada[n_ent] = 0;
        if (accion == A_IR) {
            ir_entrada();
        } else if (accion == A_CARGAR) {
            ed_cargar(entrada);
        } else if (accion == A_NEX) {
            nex_entrada();
        } else if ((ed.nombre == 0 || strcmp(entrada, fichero) != 0) && fichero_existe(entrada)) {
            ed.modo = M_CONFIRMA;              /* otro fichero que ya existe */
        } else {
            guardar_en(entrada);
        }
    } else if (c == ED_K_BORRAR) {
        if (n_ent != 0) n_ent--;
    } else if (c >= 32 && c <= 126) {
        if (accion == A_IR) {
            if (c >= '0' && c <= '9' && n_ent < 5) entrada[n_ent++] = (char)c;
        } else if (n_ent < ED_NOMBRE_MAX) {
            entrada[n_ent++] = (char)c;
        }
    }
}

/* Borra la linea del cursor entera. El cursor queda al principio de la linea que ocupa
 * su lugar; si era la ultima, al principio de la anterior (y la linea desaparece). Con
 * una sola linea solo se vacia. */
static void borrar_linea(void)
{
    uint16_t ini, fin, n, len, pos_nueva;
    uint8_t quitar_anterior;

    ini = ed.ini_lin;
    fin = eb_fin_linea(ini);
    len = eb_longitud();
    n = fin - ini;
    quitar_anterior = 0;
    pos_nueva = ini;
    if (fin < len) {
        n++;                                    /* y su salto de linea */
    } else if (ini > 0) {
        quitar_anterior = 1;                    /* ultima linea: se lleva el salto anterior */
        ini--;
        n++;
        pos_nueva = eb_ini_linea(ini);
    }
    if (n == 0) return;                         /* texto vacio */
    while (n != 0) {
        eb_borrar(ini);
        n--;
    }
    ed.pos = pos_nueva;
    ed.ini_lin = pos_nueva;
    if (quitar_anterior) ed.linea--;
    ed.col_obj = 0;
    ed.modificado = 1;
    ed.sucio_todo = 1;
}

static void comando(int c)
{
    if (c >= 'a' && c <= 'z') c -= 32;
    switch (c) {
    case 'H': ir_inicio_linea(); break;
    case 'E': ir_fin_linea(); break;
    case 'T': ir_inicio_texto(); break;
    case 'B': ir_fin_texto(); break;
    case 'U': pagina(0); break;
    case 'D': pagina(1); break;
    case 'G': pedir_nombre(A_IR, 0); break;
    case 'K': borrar_linea(); break;
    case 'C': lanzar(0); break;
    case 'O': construir(); break;
    case 'X': otro_error(1); break;
    case 'Z': otro_error(-1); break;
    case '5': mover_izq(); break;
    case '6': mover_aba(); break;
    case '7': mover_arr(); break;
    case '8': mover_der(); break;
    case 'N':
        if (ed.modificado) { accion = A_NUEVO; ed.modo = M_CONFIRMA; }
        else ed_nuevo();
        break;
    case 'L':
        if (ed.modificado) { accion = A_CARGAR; ed.modo = M_CONFIRMA; }
        else pedir_nombre(A_CARGAR, 0);
        break;
    case 'S':
        if (ed.nombre != 0) guardar_en(fichero);        /* ya tiene nombre: guarda directamente */
        else pedir_nombre(A_GUARDAR, 0);
        break;
    case 'A': pedir_nombre(A_GUARDAR, ed.nombre); break;
    default:  aviso(msg_ignorado); break;
    }
}

void ed_tecla(int c)
{
    ed.ultima = c;
    ed.msg = 0;
    ed.msg_err = 0;

    if (ed.modo == M_CONFIRMA) {
        ed.modo = M_NORMAL;
        if (c == 'Y' || c == 'y') confirmado();
        else ed.msg = msg_cancelado;
        return;
    }
    if (ed.modo == M_NOMBRE) {
        tecla_nombre(c);
        return;
    }
    if (ed.modo != M_NORMAL) {
        ed.modo = M_NORMAL;
        if (c == ED_K_MENU || c == ED_K_MENU2) return;     /* Esc otra vez: cancela sin quejarse */
        comando(c);
        return;
    }
    if (c == ED_K_MENU || c == ED_K_MENU2) {
        ed.modo = M_COMANDO;
        return;
    }
    if (c >= 32 && c <= 126) {
        insertar_car((uint8_t)c);
        return;
    }
    if (ED_K_TAB != 0 && c == ED_K_TAB) {
        insertar_tab();
        return;
    }
    switch (c) {
    case ED_K_ENTER:  insertar_nl(); break;
    case ED_K_BORRAR: borrar_atras(); break;
    case ED_K_IZQ:    mover_izq(); break;
    case ED_K_DER:    mover_der(); break;
    case ED_K_ARR:    mover_arr(); break;
    case ED_K_ABA:    mover_aba(); break;
    default: break;
    }
}

/* ------------------------------------------------------------------
 * Arranque
 * ------------------------------------------------------------------ */

void ed_iniciar(void)
{
    eb_iniciar();
    memset(&ed, 0, sizeof(ed));
    incompleto = 0;
    nex_fijado = 0;
    ed.sucio_todo = 1;
}

uint16_t ed_insertar_texto(const char *s)
{
    uint16_t n;
    uint16_t antes;

    n = 0;
    while (*s != 0) {
        antes = eb_longitud();
        if (*s == '\n') insertar_nl();
        else if (*s >= 32 && *s <= 126) insertar_car((uint8_t)*s);
        if (eb_longitud() != antes) n++;
        s++;
    }
    ed.sucio_todo = 1;
    ed.msg = 0;
    ed.msg_err = 0;
    return n;
}

/* ------------------------------------------------------------------
 * Carga de ficheros
 * ------------------------------------------------------------------ */

/* Mensaje compuesto ("Loaded NAME: N bytes"...): una sola cadena estatica,
 * porque ed.msg es un puntero y el mensaje debe seguir vivo tras volver. */
static char msgbuf[80];
static uint8_t nmb;

static void mb_cad(const char *s)
{
    while (*s != 0 && nmb < sizeof(msgbuf) - 1) msgbuf[nmb++] = *s++;
}

static void mb_num(uint16_t v)
{
    char d[5];
    uint8_t i, n;

    n = num_txt(v, d);
    for (i = 0; i < n && nmb < sizeof(msgbuf) - 1; i++) msgbuf[nmb++] = d[i];
}

static void mb_fin(void)
{
    msgbuf[nmb] = 0;
    ed.msg = msgbuf;
}

/* Blancos para expandir tabuladores al cargar (el maximo de tab_espacios es 3). */
static const uint8_t blancos[3] = { ' ', ' ', ' ' };

uint8_t ed_cargar(const char *nombre)
{
    int h, n, i, ini;
    uint16_t total, col, k, cupo;
    uint8_t r;

    h = plat_fich_abrir_lectura(nombre);
    if (h < 0) {
        nmb = 0;
        mb_cad("Cannot open ");
        mb_cad(nombre);
        mb_cad(" (err ");
        mb_num((uint16_t)(-h));
        mb_cad(")");
        mb_fin();
        ed.msg_err = 1;
        return 1;
    }

    ed_iniciar();
    fijar_nombre(nombre);
    r = 0;
    total = 0;
    col = 0;                                   /* columna actual en el fichero, para los tabuladores */
    while (r == 0) {
        n = plat_fich_leer(h, lin, ED_ANCHO);
        if (n < 0) { r = 2; break; }
        if (n == 0) break;
        /* Se copian de una vez los tramos normales; '\r' se salta y '\t' se expande. */
        ini = 0;
        for (i = 0; i <= n && r == 0; i++) {
            if (i < n && lin[i] != '\r' && lin[i] != '\t') {
                if (lin[i] == '\n') col = 0; else col++;
                continue;
            }
            if (i > ini) {                      /* vaciar el tramo normal pendiente */
                k = eb_agregar(lin + ini, (uint16_t)(i - ini));
                total += k;
                if (k < (uint16_t)(i - ini)) { r = 3; break; }
            }
            if (i < n && lin[i] == '\t') {
                k = tab_espacios(col);
                cupo = eb_agregar(blancos, k);
                total += cupo;
                if (cupo < k) { r = 3; break; }
                col += k;
            }
            ini = i + 1;
        }
    }
    plat_fich_cerrar(h);

    ir_inicio_texto();
    ed.modificado = 0;
    ed.sucio_todo = 1;
    incompleto = (r == 2 || r == 3);
    nmb = 0;
    if (r == 0) {
        mb_cad("Loaded ");
        mb_cad(nombre);
        mb_cad(": ");
        mb_num(total);
        mb_cad(" bytes");
    } else if (r == 2) {
        mb_cad("Read error after ");
        mb_num(total);
        mb_cad(" bytes");
    } else {
        mb_cad("File too big: loaded ");
        mb_num(total);
        mb_cad(" bytes");
    }
    mb_fin();
    ed.msg_err = (r != 0);
    return r;
}

/* ------------------------------------------------------------------
 * Guardado y fichero nuevo
 * ------------------------------------------------------------------ */

uint8_t ed_guardar(const char *nombre)
{
    int h, n;
    uint16_t len, p, k;
    uint8_t r;

    h = plat_fich_abrir_escritura(nombre);
    nmb = 0;
    if (h < 0) {
        mb_cad("Cannot create ");
        mb_cad(nombre);
        mb_cad(" (err ");
        mb_num((uint16_t)(-h));
        mb_cad(")");
        mb_fin();
        ed.msg_err = 1;
        return 1;
    }

    r = 0;
    len = eb_longitud();
    for (p = 0; p < len; p += k) {
        k = len - p;
        if (k > ED_ANCHO) k = ED_ANCHO;
        eb_leer(p, lin, k);
        n = plat_fich_escribir(h, lin, k);
        if (n != (int)k) { r = 2; break; }
    }
    plat_fich_cerrar(h);

    if (r != 0) {
        mb_cad("Write error: ");
        mb_cad(nombre);
        mb_cad(" is incomplete");
        mb_fin();
        ed.msg_err = 1;
        return 2;
    }
    fijar_nombre(nombre);
    ed.modificado = 0;
    incompleto = 0;                             /* lo guardado es lo que hay: ya no hay nada que perder */
    mb_cad("Saved ");
    mb_cad(nombre);
    mb_cad(": ");
    mb_num(len);
    mb_cad(" bytes");
    mb_fin();
    ed.msg_err = 0;
    return 0;
}

/* Lleva el cursor al principio de la linea n (la 1.a es la 1); si n pasa del final,
 * a la ultima. */
static void ir_linea(uint16_t n)
{
    uint16_t f, len;

    len = eb_longitud();
    ed.pos = 0;
    ed.linea = 0;
    while (ed.linea + 1 < n) {
        f = eb_fin_linea(ed.pos);
        if (f >= len) break;
        ed.pos = f + 1;
        ed.linea++;
    }
    ed.ini_lin = ed.pos;
    ed.col_obj = 0;
}

/* Ir a la linea tecleada (1 = la primera). Pasada del final: va a la ultima. */
static void ir_entrada(void)
{
    uint16_t n;
    uint8_t i;

    n = 0;
    for (i = 0; i < n_ent; i++) {
        if (n > 6553) n = 65535;
        else n = (uint16_t)(n * 10 + (uint8_t)(entrada[i] - '0'));
    }
    if (n == 0) n = 1;
    ir_linea(n);
    if (ed.linea + 1 < n) {
        nmb = 0;
        mb_cad("Past the end: moved to line ");
        mb_num(ed.linea + 1);
        mb_fin();
    }
}

/* ------------------------------------------------------------------
 * Compilar (mf_api.h) y recorrer los errores
 * ------------------------------------------------------------------ */

#ifdef ED_CON_COMPILADOR
static uint8_t  comp_hay;         /* hay una compilacion con errores que recorrer */
static uint8_t  comp_i;           /* indice del error mostrado en mf.diag */
static uint16_t comp_ver;         /* version del texto (eb.ver) cuando se compilo */

/* Lleva el cursor al error i de mf.diag y pone su mensaje en la ultima fila. */
static void mostrar_error(uint8_t i)
{
    char m[MF_MENSAJE_MAX];
    uint16_t l;

    ir_linea(mf.diag[i].linea);
    l = eb_fin_linea(ed.pos) - ed.pos;                 /* longitud de esa linea */
    if (mf.diag[i].columna > 1) {
        l = (l < (uint16_t)(mf.diag[i].columna - 1)) ? l : (uint16_t)(mf.diag[i].columna - 1);
        ed.pos += l;                                   /* una columna pasada del final: al final */
    }
    ed.col_obj = ed.pos - ed.ini_lin;
    mf_mensaje(mf.diag[i].cod, m);
    nmb = 0;
    mb_cad("Error ");
    mb_num((uint16_t)(i + 1));
    mb_cad("/");
    mb_num(mf.n);
    mb_cad(" at ");
    mb_num(mf.diag[i].linea);
    mb_cad(":");
    mb_num(mf.diag[i].columna);
    mb_cad(": ");
    mb_cad(m);
    mb_fin();
    ed.msg_err = 1;
}
#endif

#ifdef ED_CON_COMPILADOR
/* Resultado de una compilacion: n errores (0 = ninguno). Va aparte de lanzar() para que el
 * buffer de 64 B de mostrar_error / mf_mensaje no este vivo en la pila mientras compila. */
static void informar(uint16_t n, uint8_t construir_nex)
{
    char m[MF_MENSAJE_MAX];

    comp_ver = eb.ver;
    comp_i = 0;
    comp_hay = (n != 0);
    if (n != 0) {
        mostrar_error(0);
    } else if (!construir_nex) {
        ed.msg = msg_compilado;
    } else if (mf.gen == MF_GEN_OK) {
        nmb = 0;
        mb_cad(msg_gen_ok);
        mb_cad(ed_nex_nombre);
        mb_fin();
    } else if (mf.gen == MF_GEN_ERR) {
        mf_mensaje(mf.gen_cod, m);
        nmb = 0;
        mb_cad(msg_gen_mal);
        mb_cad(m);
        mb_fin();
        ed.msg_err = 1;
    } else {
        ed.msg = msg_sin_gen;
    }
}
#endif

/* C (construir_nex = 0): solo comprueba. O (= 1): comprueba y, sin errores, escribe el .nex
 * en ed_nex_nombre a traves de ed_salida. mf_salida se registra solo durante la llamada. */
static void lanzar(uint8_t construir_nex)
{
#ifdef ED_CON_COMPILADOR
    uint16_t n;

    ed.msg = construir_nex ? msg_construyendo : msg_compilando;
    ed_refrescar();                                    /* que se vea mientras trabaja */
    if (construir_nex) mf_salida(&ed_salida);
    n = mf_compilar();
    if (construir_nex) mf_salida(0);
    informar(n, construir_nex);
#else
    (void)construir_nex;
    aviso(msg_sin_comp);
#endif
}

#ifdef ED_CON_COMPILADOR
/* Pone en ed_nex_nombre el nombre s con la extension cambiada por .nex (o anadida). La
 * extension es lo que sigue al ultimo '.' del ultimo componente (un '.' inicial no cuenta).
 * Cabe siempre: el nombre tiene como mucho ED_NOMBRE_MAX caracteres (comprobado abajo). */
typedef char ed_nex_cabe[(ED_NEX_MAX >= ED_NOMBRE_MAX + 4) ? 1 : -1];
static void nex_desde(const char *s)
{
    uint8_t i, base, punto, n;

    base = 0;
    punto = 0;
    for (i = 0; s[i] != 0; i++) {
        if (s[i] == '/' || s[i] == '\\') { base = (uint8_t)(i + 1); punto = 0; }
        else if (s[i] == '.' && i > base) punto = i;
    }
    n = (punto != 0) ? punto : i;
    for (i = 0; i < n; i++) ed_nex_nombre[i] = s[i];
    ed_nex_nombre[n++] = '.';
    ed_nex_nombre[n++] = 'n';
    ed_nex_nombre[n++] = 'e';
    ed_nex_nombre[n++] = 'x';
    ed_nex_nombre[n] = 0;
}
#endif

/* Con el nombre del .nex ya en ed_nex_nombre: construye (si no pisa el propio fuente). */
static void nex_lanzar(uint8_t fijar)
{
#ifdef ED_CON_COMPILADOR
    if (ed.nombre != 0 && strcmp(ed_nex_nombre, fichero) == 0) {
        aviso(msg_mismo);
        return;
    }
    if (fijar) nex_fijado = 1;                         /* nombre tecleado: no se vuelve a preguntar */
    lanzar(1);
#else
    (void)fijar;
#endif
}

/* Enter en "Build as:" */
static void nex_entrada(void)
{
#ifdef ED_CON_COMPILADOR
    nex_desde(entrada);
    if (fichero_existe(ed_nex_nombre)) {
        ed.modo = M_CONFIRMA;                          /* accion sigue siendo A_NEX */
        return;
    }
    nex_lanzar(1);
#endif
}

/* Y al "File exists" del .nex */
static void nex_confirmado(void)
{
    nex_lanzar(1);
}

/* O: construir. Con el nombre ya decidido o el del fuente se sobrescribe sin preguntar
 * (igual que S con nombre); sin ninguno de los dos se pide. */
static void construir(void)
{
#ifdef ED_CON_COMPILADOR
    if (nex_fijado) {
        nex_lanzar(0);                                 /* ed_nex_nombre ya tiene el nombre tecleado */
    } else if (ed.nombre != 0) {
        nex_desde(fichero);
        nex_lanzar(0);
    } else {
        pedir_nombre(A_NEX, 0);
    }
#else
    aviso(msg_sin_comp);
#endif
}

static void otro_error(int8_t dir)
{
#ifdef ED_CON_COMPILADOR
    uint8_t cuantos;

    if (!comp_hay) {
        ed.msg = msg_sin_errores;
        return;
    }
    if (comp_ver != eb.ver) {                          /* el texto cambio: las lineas ya no valen */
        comp_hay = 0;
        aviso(msg_texto_nuevo);
        return;
    }
    cuantos = (mf.n < MF_DIAG_MAX) ? (uint8_t)mf.n : MF_DIAG_MAX;
    if (dir > 0) comp_i = (uint8_t)((comp_i + 1 >= cuantos) ? 0 : comp_i + 1);
    else comp_i = (uint8_t)((comp_i == 0) ? cuantos - 1 : comp_i - 1);
    mostrar_error(comp_i);
#else
    (void)dir;
    aviso(msg_sin_comp);
#endif
}

void ed_nuevo(void)
{
    ed_iniciar();                      /* deja ed.nombre en 0: `fichero` ya no cuenta */
    ed.msg = msg_nuevo;
}

/* ------------------------------------------------------------------
 * Comprobacion de invariantes (solo pruebas)
 * ------------------------------------------------------------------ */

#ifdef ED_PRUEBAS
uint8_t ed_verificar(void)
{
    uint16_t len, col;

    len = eb_longitud();
    if (ed.pos > len) return 1;
    if (ed.ini_lin != eb_ini_linea(ed.pos)) return 2;
    if (ed.linea != eb_contar_nl(ed.pos)) return 3;
    if (ed.top > ed.ini_lin) return 4;
    if (ed.top != eb_ini_linea(ed.top)) return 5;
    if (ed.top_linea != eb_contar_nl(ed.top)) return 6;
    if (ed.linea < ed.top_linea || ed.linea - ed.top_linea >= ED_FILAS_TEXTO) return 7;
    col = columna();
    if (col < ed.izq || col >= ed.izq + ED_ANCHO) return 8;
    return 0;
}
#endif
