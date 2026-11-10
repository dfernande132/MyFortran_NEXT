    org 0x8000
    include "tramp_gen.asm"

_mapear:    ret                  ; el arnes intercepta este PC
_banco_actual: defb 0xFF
_mmu_bank:  defb 0x77            ; lo que hay mapeado ahora (lo escribe el arnes)
_seen1:     defb 0
_seen2:     defb 0
_seen3:     defb 0
_res1:      defw 0
_res2:      defw 0
_res3:      defw 0
_res4:      defw 0
_ixsave:    defw 0
_iysave:    defw 0

; ---- funciones "de banco" (convencion sccz80: args empujados de izq. a der.; f(a,b): [ret][b][a]) ----
_suma:                           ; HL = a + b
    ld hl,2
    add hl,sp
    ld e,(hl)
    inc hl
    ld d,(hl)                    ; DE = b
    inc hl
    ld c,(hl)
    inc hl
    ld b,(hl)                    ; BC = a
    ld a,(_mmu_bank)
    ld (_seen1),a
    ld h,b
    ld l,c
    add hl,de
    ret

_anida:                          ; anida(a) = suma(a,5) + 100 ; mide el banco antes y despues de la llamada anidada
    ld hl,2
    add hl,sp
    ld e,(hl)
    inc hl
    ld d,(hl)                    ; DE = a
    ld a,(_mmu_bank)
    ld (_seen2),a
    push de
    ld hl,5
    push hl
    call _t_suma
    pop bc
    pop bc
    ld de,100
    add hl,de
    ld a,(_mmu_bank)
    ld (_seen3),a
    ret

_largo:                          ; devuelve DEHL = (b:a)  -> HL = a, DE = b
    ld hl,2
    add hl,sp
    ld e,(hl)
    inc hl
    ld d,(hl)                    ; DE = b
    inc hl
    ld c,(hl)
    inc hl
    ld b,(hl)                    ; BC = a
    ld h,b
    ld l,c
    ret

_prof:                           ; prof(n) = n==0 ? 0 : 1 + prof(n-1) (alternando bancos 44/45)
    ld hl,2
    add hl,sp
    ld e,(hl)
    inc hl
    ld d,(hl)                    ; DE = n
    ld a,d
    or e
    jr nz,prof1
    ld hl,0
    ret
prof1:
    dec de
    push de
    call _t_prof_b
    pop bc
    inc hl
    ret
_prof2:
    ld hl,2
    add hl,sp
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld a,d
    or e
    jr nz,prof21
    ld hl,0
    ret
prof21:
    dec de
    push de
    call _t_prof_a
    pop bc
    inc hl
    ret

_t_suma:   ld a,44
           ld hl,_suma
           jp __tramp
_t_anida:  ld a,45
           ld hl,_anida
           jp __tramp
_t_largo:  ld a,46
           ld hl,_largo
           jp __tramp
_t_prof_a: ld a,44
           ld hl,_prof
           jp __tramp
_t_prof_b: ld a,45
           ld hl,_prof2
           jp __tramp

; ---- llamadores (como los generados por sccz80: push args, call, pop) ----
_test_suma:
    ld ix,0x1234
    ld iy,0x5678
    ld hl,1000
    push hl
    ld hl,234
    push hl
    call _t_suma
    pop bc
    pop bc
    ld (_res1),hl
    ld (_ixsave),ix
    ld (_iysave),iy
    ret
_test_anida:
    ld hl,7
    push hl
    call _t_anida
    pop bc
    ld (_res2),hl
    ret
_test_largo:
    ld hl,0x1111
    push hl
    ld hl,0x2222
    push hl
    call _t_largo
    pop bc
    pop bc
    ld (_res3),hl
    ld (_res4),de
    ret
_test_prof:                      ; HL de entrada ya empujado por el arnes? no: n en _nprof
    ld hl,(_nprof)
    push hl
    call _t_prof_a
    pop bc
    ld (_res1),hl
    ret
_nprof: defw 0
