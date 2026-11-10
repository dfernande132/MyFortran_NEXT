; e1_tramp.asm - Trampolines de banco GENERICOS (0.11). Sustituye a ~80 trampolines escritos en C (~48-70 B cada uno).
;
; Cada t_* es un resguardo de 7 bytes:   ld a,BANCO / ld hl,_funcion / jp __tramp
; __tramp hace lo mismo que el trampolin en C:  ant = banco_actual; mapear(BANCO); funcion(args); mapear(ant);
; y devuelve HL (y DE: valores de 32 bits) tal cual los dejo la funcion. Los argumentos (convencion de sccz80: se empujan
; de izquierda a derecha y los saca el que llama) pasan SIN tocar: __tramp saca la direccion de retorno del llamador, la
; guarda en una pila propia (_tr_pila), llama a la funcion con la pila como la dejo el llamador y, al volver, la restaura.
;
; Pila de trampolines: 3 bytes por nivel (retorno llamador, banco anterior). El anidamiento llega a ~2 niveles por cada
; F(...) anidado y por cada funcion-sentencia encadenada mas ~6 de base (gen_expr <-> gen_funcion, sem <-> sem_sf): hay 40 niveles
; (120 B de bss). Sin guarda: si un dia hiciera falta mas, subir el defs. NO usa registros alternativos (la ISR de la ROM puede tocarlos) ni toca IX/IY.
; mapear() y banco_actual siguen en c4_trampolines.c (C), que es quien conoce MMU6/7 y el estado inicial.

    SECTION code_compiler

    PUBLIC __tramp
    EXTERN _mapear
    EXTERN _banco_actual

__tramp:                        ; A = banco destino, HL = funcion; pila: [retorno llamador][argumentos...]
    pop de                      ; DE = retorno del llamador
    push hl                     ; guarda la funcion
    push af                     ; guarda el banco destino (A)
    ld hl,(__tr_ptr)
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    ld a,(_banco_actual)
    ld (hl),a                   ; banco anterior
    inc hl
    ld (__tr_ptr),hl
    pop af                      ; A = banco destino
    ld l,a
    ld h,0
    push hl
    call _mapear                ; mapear(destino)
    pop bc
    pop hl                      ; HL = funcion; la pila queda [argumentos...] como la dejo el llamador
    ld de,__tramp_vuelta
    push de
    jp (hl)                     ; llama a la funcion (vuelve en __tramp_vuelta)

__tramp_vuelta:                 ; HL (y DE) = resultado de la funcion
    push hl
    push de
    ld hl,(__tr_ptr)
    dec hl
    ld a,(hl)                   ; banco anterior
    dec hl
    ld d,(hl)
    dec hl
    ld e,(hl)                   ; DE = retorno del llamador
    ld (__tr_ptr),hl
    push de                     ; [retorno][DE res][HL res][args]
    ld l,a
    ld h,0
    push hl
    call _mapear                ; mapear(anterior)
    pop bc
    pop bc                      ; BC = retorno del llamador
    pop de
    pop hl                      ; resultado restaurado
    push bc
    ret                         ; vuelve al llamador con los argumentos intactos

    SECTION data_compiler
__tr_ptr:
    defw __tr_pila

    SECTION bss_compiler
__tr_pila:
    defs 120                    ; 40 niveles x 3 bytes

    SECTION code_compiler

; ---- resguardos (generado a partir de los antiguos trampolines de C; no editar a mano la lista) ----

; void t_tabla_iniciar(void) -> banco 46: tabla_iniciar
    PUBLIC _t_tabla_iniciar
    EXTERN _tabla_iniciar
_t_tabla_iniciar:
    ld a,46
    ld hl,_tabla_iniciar
    jp __tramp

; void t_fuente_iniciar(Fuente *f) -> banco 46: fuente_iniciar
    PUBLIC _t_fuente_iniciar
    EXTERN _fuente_iniciar
_t_fuente_iniciar:
    ld a,46
    ld hl,_fuente_iniciar
    jp __tramp

; ErrorFuente t_leer_sentencia(Fuente *f) -> banco 46: leer_sentencia
    PUBLIC _t_leer_sentencia
    EXTERN _leer_sentencia
_t_leer_sentencia:
    ld a,46
    ld hl,_leer_sentencia
    jp __tramp

; void t_lex_iniciar(void) -> banco 46: lex_iniciar
    PUBLIC _t_lex_iniciar
    EXTERN _lex_iniciar
_t_lex_iniciar:
    ld a,46
    ld hl,_lex_iniciar
    jp __tramp

; void t_lex_siguiente(Token *t) -> banco 46: lex_siguiente
    PUBLIC _t_lex_siguiente
    EXTERN _lex_siguiente
_t_lex_siguiente:
    ld a,46
    ld hl,_lex_siguiente
    jp __tramp

; void t_clasificar_sentencia(Clasificacion *c) -> banco 46: clasificar_sentencia
    PUBLIC _t_clasificar_sentencia
    EXTERN _clasificar_sentencia
_t_clasificar_sentencia:
    ld a,46
    ld hl,_clasificar_sentencia
    jp __tramp

; uint16_t t_lex_saltar_blancos(uint16_t p) -> banco 46: lex_saltar_blancos
    PUBLIC _t_lex_saltar_blancos
    EXTERN _lex_saltar_blancos
_t_lex_saltar_blancos:
    ld a,46
    ld hl,_lex_saltar_blancos
    jp __tramp

; const char * t_tabla_texto(uint16_t offset) -> banco 46: tabla_texto
    PUBLIC _t_tabla_texto
    EXTERN _tabla_texto
_t_tabla_texto:
    ld a,46
    ld hl,_tabla_texto
    jp __tramp

; void t_parser_iniciar(void) -> banco 47: parser_iniciar
    PUBLIC _t_parser_iniciar
    EXTERN _parser_iniciar
_t_parser_iniciar:
    ld a,47
    ld hl,_parser_iniciar
    jp __tramp

; uint16_t t_expr_parsear_completa(void) -> banco 47: expr_parsear_completa
    PUBLIC _t_expr_parsear_completa
    EXTERN _expr_parsear_completa
_t_expr_parsear_completa:
    ld a,47
    ld hl,_expr_parsear_completa
    jp __tramp

; void t_stmt_analizar(void) -> banco 47: stmt_analizar
    PUBLIC _t_stmt_analizar
    EXTERN _stmt_analizar
_t_stmt_analizar:
    ld a,47
    ld hl,_stmt_analizar
    jp __tramp

; uint8_t t_stmt_elemento(void) -> banco 47: stmt_elemento
    PUBLIC _t_stmt_elemento
    EXTERN _stmt_elemento
_t_stmt_elemento:
    ld a,47
    ld hl,_stmt_elemento
    jp __tramp

; void t_stmt_analizar_interna(void) -> banco 47: stmt_analizar_interna
    PUBLIC _t_stmt_analizar_interna
    EXTERN _stmt_analizar_interna
_t_stmt_analizar_interna:
    ld a,47
    ld hl,_stmt_analizar_interna
    jp __tramp

; void t_posicion_fuente(uint16_t offset, uint16_t *linea, uint16_t *columna) -> banco 46: posicion_fuente
    PUBLIC _t_posicion_fuente
    EXTERN _posicion_fuente
_t_posicion_fuente:
    ld a,46
    ld hl,_posicion_fuente
    jp __tramp

; void t_sem_iniciar(void) -> banco 45: sem_iniciar
    PUBLIC _t_sem_iniciar
    EXTERN _sem_iniciar
_t_sem_iniciar:
    ld a,45
    ld hl,_sem_iniciar
    jp __tramp

; void t_sem_sentencia(void) -> banco 45: sem_sentencia
    PUBLIC _t_sem_sentencia
    EXTERN _sem_sentencia
_t_sem_sentencia:
    ld a,45
    ld hl,_sem_sentencia
    jp __tramp

; void t_sem_fin_fichero(uint16_t linea) -> banco 45: sem_fin_fichero
    PUBLIC _t_sem_fin_fichero
    EXTERN _sem_fin_fichero
_t_sem_fin_fichero:
    ld a,45
    ld hl,_sem_fin_fichero
    jp __tramp

; uint16_t t_sim_cuenta(void) -> banco 45: sim_cuenta
    PUBLIC _t_sim_cuenta
    EXTERN _sim_cuenta
_t_sim_cuenta:
    ld a,45
    ld hl,_sim_cuenta
    jp __tramp

; void t_msg_lex(uint8_t e, char *dst) -> banco 46: msg_lex
    PUBLIC _t_msg_lex
    EXTERN _msg_lex
_t_msg_lex:
    ld a,46
    ld hl,_msg_lex
    jp __tramp

; void t_msg_expr(uint8_t e, char *dst) -> banco 47: msg_expr
    PUBLIC _t_msg_expr
    EXTERN _msg_expr
_t_msg_expr:
    ld a,47
    ld hl,_msg_expr
    jp __tramp

; void t_msg_stmt(uint8_t e, char *dst) -> banco 47: msg_stmt
    PUBLIC _t_msg_stmt
    EXTERN _msg_stmt
_t_msg_stmt:
    ld a,47
    ld hl,_msg_stmt
    jp __tramp

; void t_msg_sem(uint8_t e, char *dst) -> banco 47: msg_sem
    PUBLIC _t_msg_sem
    EXTERN _msg_sem
_t_msg_sem:
    ld a,47
    ld hl,_msg_sem
    jp __tramp

; void t_gen_activar(uint8_t si) -> banco 44: gen_activar
    PUBLIC _t_gen_activar
    EXTERN _gen_activar
_t_gen_activar:
    ld a,44
    ld hl,_gen_activar
    jp __tramp

; void t_gen_iniciar(void) -> banco 44: gen_iniciar
    PUBLIC _t_gen_iniciar
    EXTERN _gen_iniciar
_t_gen_iniciar:
    ld a,44
    ld hl,_gen_iniciar
    jp __tramp

; void t_gen_etiqueta(uint32_t etiq) -> banco 44: gen_etiqueta
    PUBLIC _t_gen_etiqueta
    EXTERN _gen_etiqueta
_t_gen_etiqueta:
    ld a,44
    ld hl,_gen_etiqueta
    jp __tramp

; void t_gen_sentencia(void) -> banco 44: gen_sentencia
    PUBLIC _t_gen_sentencia
    EXTERN _gen_sentencia
_t_gen_sentencia:
    ld a,44
    ld hl,_gen_sentencia
    jp __tramp

; void t_gen_print_item(void) -> banco 44: gen_print_item
    PUBLIC _t_gen_print_item
    EXTERN _gen_print_item
_t_gen_print_item:
    ld a,44
    ld hl,_gen_print_item
    jp __tramp

; uint8_t t_stmt_dato(void) -> banco 47: stmt_dato
    PUBLIC _t_stmt_dato
    EXTERN _stmt_dato
_t_stmt_dato:
    ld a,47
    ld hl,_stmt_dato
    jp __tramp

; void t_gen_param_item(void) -> banco 44: gen_param_item
    PUBLIC _t_gen_param_item
    EXTERN _gen_param_item
_t_gen_param_item:
    ld a,44
    ld hl,_gen_param_item
    jp __tramp

; void t_gen_read(uint8_t item) -> banco 44: gen_read
    PUBLIC _t_gen_read
    EXTERN _gen_read
_t_gen_read:
    ld a,44
    ld hl,_gen_read
    jp __tramp

; void t_gen_goto_etq(uint32_t etiq, uint16_t pos) -> banco 44: gen_goto_etq
    PUBLIC _t_gen_goto_etq
    EXTERN _gen_goto_etq
_t_gen_goto_etq:
    ld a,44
    ld hl,_gen_goto_etq
    jp __tramp

; void t_gen_fin_sentencia(void) -> banco 44: gen_fin_sentencia
    PUBLIC _t_gen_fin_sentencia
    EXTERN _gen_fin_sentencia
_t_gen_fin_sentencia:
    ld a,44
    ld hl,_gen_fin_sentencia
    jp __tramp

; uint16_t t_sim_buscar(const char *nombre) -> banco 45: sim_buscar
    PUBLIC _t_sim_buscar
    EXTERN _sim_buscar
_t_sim_buscar:
    ld a,45
    ld hl,_sim_buscar
    jp __tramp

; void t_sim_leer(uint16_t id, Simbolo *s) -> banco 45: sim_leer
    PUBLIC _t_sim_leer
    EXTERN _sim_leer
_t_sim_leer:
    ld a,45
    ld hl,_sim_leer
    jp __tramp

; void t_sim_escribir(uint16_t id, const Simbolo *s) -> banco 45: sim_escribir
    PUBLIC _t_sim_escribir
    EXTERN _sim_escribir
_t_sim_escribir:
    ld a,45
    ld hl,_sim_escribir
    jp __tramp

; void t_sim_dim_leer(uint16_t indice, Dim *d) -> banco 45: sim_dim_leer
    PUBLIC _t_sim_dim_leer
    EXTERN _sim_dim_leer
_t_sim_dim_leer:
    ld a,45
    ld hl,_sim_dim_leer
    jp __tramp

; uint16_t t_sem_etq_indice(uint32_t etiq) -> banco 45: sem_etq_indice
    PUBLIC _t_sem_etq_indice
    EXTERN _sem_etq_indice
_t_sem_etq_indice:
    ld a,45
    ld hl,_sem_etq_indice
    jp __tramp

; uint32_t t_sem_etq_valor(uint16_t indice) -> banco 45: sem_etq_valor
    PUBLIC _t_sem_etq_valor
    EXTERN _sem_etq_valor
_t_sem_etq_valor:
    ld a,45
    ld hl,_sem_etq_valor
    jp __tramp

; void t_back_c(uint8_t cual, const TacInst *t, uint16_t x, uint16_t y) -> banco 48: back_c_entrada
    PUBLIC _t_back_c
    EXTERN _back_c_entrada
_t_back_c:
    ld a,48
    ld hl,_back_c_entrada
    jp __tramp

; uint16_t t_gen_expr(uint16_t n, uint16_t dst) -> banco 44: gen_expr
    PUBLIC _t_gen_expr
    EXTERN _gen_expr
_t_gen_expr:
    ld a,44
    ld hl,_gen_expr
    jp __tramp

; uint16_t t_gen_desplazamiento(uint16_t n, const Simbolo *s, const Dim *dm) -> banco 44: gen_desplazamiento
    PUBLIC _t_gen_desplazamiento
    EXTERN _gen_desplazamiento
_t_gen_desplazamiento:
    ld a,44
    ld hl,_gen_desplazamiento
    jp __tramp

; uint16_t t_gen_subcadena(uint16_t base, uint16_t cap_base, uint16_t rango, uint8_t asum) -> banco 44: gen_subcadena
    PUBLIC _t_gen_subcadena
    EXTERN _gen_subcadena
_t_gen_subcadena:
    ld a,44
    ld hl,_gen_subcadena
    jp __tramp

; uint16_t t_gen_funcion(uint16_t n, uint8_t u) -> banco 42: gen_funcion
    PUBLIC _t_gen_funcion
    EXTERN _gen_funcion
_t_gen_funcion:
    ld a,42
    ld hl,_gen_funcion
    jp __tramp

; void t_gen_llamada_emitir(void) -> banco 42: gen_llamada_emitir
    PUBLIC _t_gen_llamada_emitir
    EXTERN _gen_llamada_emitir
_t_gen_llamada_emitir:
    ld a,42
    ld hl,_gen_llamada_emitir
    jp __tramp

; void t_tac_iniciar(void) -> banco 42: tac_iniciar
    PUBLIC _t_tac_iniciar
    EXTERN _tac_iniciar
_t_tac_iniciar:
    ld a,42
    ld hl,_tac_iniciar
    jp __tramp

; void t_tac_emitir(uint8_t op, uint8_t tipo, uint16_t a, uint16_t b, uint16_t c) -> banco 42: tac_emitir
    PUBLIC _t_tac_emitir
    EXTERN _tac_emitir
_t_tac_emitir:
    ld a,42
    ld hl,_tac_emitir
    jp __tramp

; uint16_t t_tac_entero(int16_t v) -> banco 42: tac_entero
    PUBLIC _t_tac_entero
    EXTERN _tac_entero
_t_tac_entero:
    ld a,42
    ld hl,_tac_entero
    jp __tramp

; uint16_t t_tac_logico(uint8_t v) -> banco 42: tac_logico
    PUBLIC _t_tac_logico
    EXTERN _tac_logico
_t_tac_logico:
    ld a,42
    ld hl,_tac_logico
    jp __tramp

; uint16_t t_tac_cadena(const char *s, uint8_t len) -> banco 42: tac_cadena
    PUBLIC _t_tac_cadena
    EXTERN _tac_cadena
_t_tac_cadena:
    ld a,42
    ld hl,_tac_cadena
    jp __tramp

; uint16_t t_tac_real(uint32_t bits) -> banco 42: tac_real
    PUBLIC _t_tac_real
    EXTERN _tac_real
_t_tac_real:
    ld a,42
    ld hl,_tac_real
    jp __tramp

; void t_back_salida_pantalla(uint8_t si) -> banco 43: back_salida_pantalla
    PUBLIC _t_back_salida_pantalla
    EXTERN _back_salida_pantalla
_t_back_salida_pantalla:
    ld a,43
    ld hl,_back_salida_pantalla
    jp __tramp

; void t_z_imagen_leer(uint16_t desp, uint8_t *dst, uint16_t n) -> banco 43: z_imagen_leer
    PUBLIC _t_z_imagen_leer
    EXTERN _z_imagen_leer
_t_z_imagen_leer:
    ld a,43
    ld hl,_z_imagen_leer
    jp __tramp

; uint8_t t_decl_entrada(uint8_t op, uint8_t tipo, uint16_t e) -> banco 42: decl_entrada
    PUBLIC _t_decl_entrada
    EXTERN _decl_entrada
_t_decl_entrada:
    ld a,42
    ld hl,_decl_entrada
    jp __tramp

; uint16_t t_sim_alta(const char *nombre, uint8_t tipo, uint8_t clase) -> banco 45: sim_alta
    PUBLIC _t_sim_alta
    EXTERN _sim_alta
_t_sim_alta:
    ld a,45
    ld hl,_sim_alta
    jp __tramp

; uint16_t t_sim_dim_nueva(const Dim *d) -> banco 45: sim_dim_nueva
    PUBLIC _t_sim_dim_nueva
    EXTERN _sim_dim_nueva
_t_sim_dim_nueva:
    ld a,45
    ld hl,_sim_dim_nueva
    jp __tramp

; uint8_t t_sf_entrada(uint8_t op, uint16_t n, uint8_t destino) -> banco 48: sf_entrada
    PUBLIC _t_sf_entrada
    EXTERN _sf_entrada
_t_sf_entrada:
    ld a,48
    ld hl,_sf_entrada
    jp __tramp

; uint16_t t_sem_cb(uint8_t op, uint16_t a, uint16_t b) -> banco 45: sem_cb
    PUBLIC _t_sem_cb
    EXTERN _sem_cb
_t_sem_cb:
    ld a,45
    ld hl,_sem_cb
    jp __tramp

; uint8_t t_nex_generar(const MfSalida *s) -> banco 42: nex_generar
    PUBLIC _t_nex_generar
    EXTERN _nex_generar
_t_nex_generar:
    ld a,42
    ld hl,_nex_generar
    jp __tramp

; void t_msg_gen(uint8_t e, char *dst) -> banco 47: msg_gen
    PUBLIC _t_msg_gen
    EXTERN _msg_gen
_t_msg_gen:
    ld a,47
    ld hl,_msg_gen
    jp __tramp

; uint8_t t_rt_real_leer(uint8_t r, uint16_t off, uint8_t *dst, uint8_t n) -> banco 46: rt_real_leer
    PUBLIC _t_rt_real_leer
    EXTERN _rt_real_leer
_t_rt_real_leer:
    ld a,46
    ld hl,_rt_real_leer
    jp __tramp

; uint8_t t_rt_ent_leer(uint8_t r, uint16_t off, uint8_t *dst, uint8_t n) -> banco 48: rt_ent_leer
    PUBLIC _t_rt_ent_leer
    EXTERN _rt_ent_leer
_t_rt_ent_leer:
    ld a,48
    ld hl,_rt_ent_leer
    jp __tramp

; uint8_t t_real_de_texto(const char *texto, uint32_t *bits) -> banco 46: real_de_texto
    PUBLIC _t_real_de_texto
    EXTERN _real_de_texto
_t_real_de_texto:
    ld a,46
    ld hl,_real_de_texto
    jp __tramp

; uint32_t t_real_de_entero(int16_t v) -> banco 46: real_de_entero
    PUBLIC _t_real_de_entero
    EXTERN _real_de_entero
_t_real_de_entero:
    ld a,46
    ld hl,_real_de_entero
    jp __tramp

; void t_gen_unidad(uint8_t clase, uint8_t uid) -> banco 44: gen_unidad
    PUBLIC _t_gen_unidad
    EXTERN _gen_unidad
_t_gen_unidad:
    ld a,44
    ld hl,_gen_unidad
    jp __tramp

; void t_gen_llamada_inicio(uint8_t uid) -> banco 42: gen_llamada_inicio
    PUBLIC _t_gen_llamada_inicio
    EXTERN _gen_llamada_inicio
_t_gen_llamada_inicio:
    ld a,42
    ld hl,_gen_llamada_inicio
    jp __tramp

; void t_gen_llamada_arg(uint8_t k) -> banco 42: gen_llamada_arg
    PUBLIC _t_gen_llamada_arg
    EXTERN _gen_llamada_arg
_t_gen_llamada_arg:
    ld a,42
    ld hl,_gen_llamada_arg
    jp __tramp

; void t_sem_sentencia_erronea(void) -> banco 45: sem_sentencia_erronea
    PUBLIC _t_sem_sentencia_erronea
    EXTERN _sem_sentencia_erronea
_t_sem_sentencia_erronea:
    ld a,45
    ld hl,_sem_sentencia_erronea
    jp __tramp

; void t_msg_uni(uint8_t e, char *dst) -> banco 47: msg_uni
    PUBLIC _t_msg_uni
    EXTERN _msg_uni
_t_msg_uni:
    ld a,47
    ld hl,_msg_uni
    jp __tramp

; void t_uni_iniciar(void) -> banco 47: uni_iniciar
    PUBLIC _t_uni_iniciar
    EXTERN _uni_iniciar
_t_uni_iniciar:
    ld a,47
    ld hl,_uni_iniciar
    jp __tramp

; uint8_t t_uni_cuenta(void) -> banco 47: uni_cuenta
    PUBLIC _t_uni_cuenta
    EXTERN _uni_cuenta
_t_uni_cuenta:
    ld a,47
    ld hl,_uni_cuenta
    jp __tramp

; uint8_t t_uni_buscar(const char *nombre) -> banco 47: uni_buscar
    PUBLIC _t_uni_buscar
    EXTERN _uni_buscar
_t_uni_buscar:
    ld a,47
    ld hl,_uni_buscar
    jp __tramp

; uint8_t t_uni_alta(const char *nombre, uint16_t linea, uint16_t col) -> banco 47: uni_alta
    PUBLIC _t_uni_alta
    EXTERN _uni_alta
_t_uni_alta:
    ld a,47
    ld hl,_uni_alta
    jp __tramp

; void t_uni_leer(uint8_t u, Unidad *x) -> banco 47: uni_leer
    PUBLIC _t_uni_leer
    EXTERN _uni_leer
_t_uni_leer:
    ld a,47
    ld hl,_uni_leer
    jp __tramp

; void t_uni_escribir(uint8_t u, const Unidad *x) -> banco 47: uni_escribir
    PUBLIC _t_uni_escribir
    EXTERN _uni_escribir
_t_uni_escribir:
    ld a,47
    ld hl,_uni_escribir
    jp __tramp

; uint16_t t_uni_dir_get(uint8_t u) -> banco 47: uni_dir_get
    PUBLIC _t_uni_dir_get
    EXTERN _uni_dir_get
_t_uni_dir_get:
    ld a,47
    ld hl,_uni_dir_get
    jp __tramp

; void t_uni_dir_set(uint8_t u, uint16_t d) -> banco 47: uni_dir_set
    PUBLIC _t_uni_dir_set
    EXTERN _uni_dir_set
_t_uni_dir_set:
    ld a,47
    ld hl,_uni_dir_set
    jp __tramp

; uint8_t t_uni_llamada(uint8_t llamador, uint8_t llamado, uint16_t linea, uint8_t col, uint8_t n, const uint8_t *kind, const uint8_t *lens, uint8_t ftipo, uint8_t flen) -> banco 47: uni_llamada
    PUBLIC _t_uni_llamada
    EXTERN _uni_llamada
_t_uni_llamada:
    ld a,47
    ld hl,_uni_llamada
    jp __tramp

; void t_uni_comprobar(void) -> banco 47: uni_comprobar
    PUBLIC _t_uni_comprobar
    EXTERN _uni_comprobar
_t_uni_comprobar:
    ld a,47
    ld hl,_uni_comprobar
    jp __tramp

; uint8_t t_ed_ejecutar(void) -> banco 41: ed_ejecutar
    PUBLIC _t_ed_ejecutar
    EXTERN _ed_ejecutar
_t_ed_ejecutar:
    ld a,41
    ld hl,_ed_ejecutar
    jp __tramp

; int t_ed_linea(uint16_t n, char *dst) -> banco 41: ed_linea (la llama source.c, banco 46; ver e1_banco46.c)
    PUBLIC _t_ed_linea
    EXTERN _ed_linea
_t_ed_linea:
    ld a,41
    ld hl,_ed_linea
    jp __tramp
