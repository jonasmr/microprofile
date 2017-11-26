PUBLIC microprofile_tramp_enter
PUBLIC microprofile_tramp_leave
PUBLIC microprofile_tramp_code_begin
PUBLIC microprofile_tramp_code_end
PUBLIC microprofile_tramp_end
PUBLIC microprofile_tramp_exit
PUBLIC microprofile_tramp_intercept0
PUBLIC microprofile_tramp_enter_patch
;PUBLIC microprofile_tramp_get_rsp_loc
PUBLIC microprofile_tramp_arg0
;PUBLIC microprofile_tramp_arg1
PUBLIC microprofile_tramp_call_patch_pop
PUBLIC microprofile_tramp_call_patch_push
PUBLIC microprofile_tramp_trunk
.data
.code	


;RCX, RDX,  R8, R9, XMM0-XMM3, YMM0-YMM3, ZMM0-ZMM
;RAX, ST(0), XMM0, YMM0, ZMM0 RAX
microprofile_tramp_enter:
	mov r11, [rsp]

	push rcx
	push rdx
	push r8
	push r9

	;sub $88h, %rsp
	sub rsp, 68h

	movdqu [rsp + 050h], xmm3
	movdqu [rsp + 040h], xmm2
	movdqu [rsp + 030h], xmm1
	movdqu [rsp + 020h], xmm0

	mov rcx, r11	
microprofile_tramp_call_patch_push:
	mov rax, 0102030405060708h
 	call rax
 	test rax, rax
 	jz microprofile_tramp_fail  ;if push fails, skip to call code, and dont patch return adress.

microprofile_tramp_enter_patch:
; PATCH 1 TRAMP EXIT
 	mov rax, 0102030405060708h ;patch to tramp_code_end
 	mov [rsp+088h], rax
microprofile_tramp_arg0:
	mov	rcx, 42
microprofile_tramp_intercept0:
;PATCH 2 INTERCEPT0
	mov rax, 0102030405060708h
	call rax
microprofile_tramp_fail:
	movdqu xmm3, [rsp + 050h]
	movdqu xmm2, [rsp + 040h]
	movdqu xmm1, [rsp + 030h]
	movdqu xmm0, [rsp + 020h]
	add rsp, 68h
	pop r9
	pop r8
	pop rdx
	pop rcx

microprofile_tramp_code_begin:
;;insert code here
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
microprofile_tramp_code_end:
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
microprofile_tramp_exit:
	mov rcx, 117
	push rax
	sub rsp, 30h
	movdqu [rsp + 020h], xmm0

microprofile_tramp_leave:
	;PATCH 3 INTERCEPT1
	mov rax, 0102030405060708h
	call rax ;jump to proxy

microprofile_tramp_call_patch_pop:
	mov rax, 0102030405060708h
	call rax
	mov r11, rax

	movdqu xmm0, [rsp + 020h]
	add rsp, 30h


	pop rax
	jmp r11

	int 3
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
 	int 3
microprofile_tramp_trunk: ;: #used for moved constants.
; todo: move it to the beginning so it can be aligned by default.
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
microprofile_tramp_end:
end
