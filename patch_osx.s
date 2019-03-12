.intel_syntax noprefix
.text
.globl _microprofile_tramp_enter
.globl _microprofile_tramp_leave
.globl _microprofile_tramp_code_begin
.globl _microprofile_tramp_code_end
.globl _microprofile_tramp_end
.globl _microprofile_tramp_exit
.globl _microprofile_tramp_intercept0
.globl _microprofile_tramp_enter_patch
.globl _microprofile_tramp_get_rsp_loc
.globl _microprofile_tramp_arg0
.globl _microprofile_tramp_arg1
.globl _microprofile_tramp_call_patch_pop
.globl _microprofile_tramp_call_patch_push
.globl _microprofile_tramp_trunk

_microprofile_tramp_enter:
	mov r12, [rsp]
	# mov (%rsp), %(rax)

	push rdi
	push rsi
	push rdx
	push rcx
	push r8
	push r9
	#sub $88h, %rsp
	sub rsp, 0x88

	movdqu 0x70[rsp], xmm7
	movdqu 0x60[rsp], xmm6
	movdqu 0x50[rsp], xmm5
	movdqu 0x40[rsp], xmm4
	movdqu 0x30[rsp], xmm3
	movdqu 0x20[rsp], xmm2
	movdqu 0x10[rsp], xmm1
	movdqu [rsp], xmm0

# 	#call _microprofile_tramp_get_rsp_loc
	mov rdi, r12
_microprofile_tramp_call_patch_push:
	mov rax, 0x0102030405060708
 	call rax
 	test rax, rax
 	jz _microprofile_tramp_fail  #if push fails, skip to call code, and dont patch return adress.
# 	##todo check ret val
# 	# movq %r12, (%rax)

_microprofile_tramp_enter_patch:
# PATCH 1 TRAMP EXIT
 	mov rax, 0x0102030405060708 #patch to tramp_code_end
 	mov [rsp + 0xb8], rax
_microprofile_tramp_arg0:
	mov	rdi, 42
_microprofile_tramp_intercept0:
#PATCH 2 INTERCEPT0
	mov rax, 0x0102030405060708
	call rax
_microprofile_tramp_fail:
	movdqu xmm7, [rsp + 0x70]
	movdqu xmm6, [rsp + 0x60]
	movdqu xmm5, [rsp + 0x50]
	movdqu xmm4, [rsp + 0x40]
	movdqu xmm3, [rsp + 0x30]
	movdqu xmm2, [rsp + 0x20]
	movdqu xmm1, [rsp + 0x10]
	movdqu xmm0, [rsp]
	add rsp, 0x88
	pop r9
	pop r8
	pop rcx
	pop rdx
	pop rsi
	pop rdi
_microprofile_tramp_code_begin:
##insert code here
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
_microprofile_tramp_code_end:
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
_microprofile_tramp_exit:
	mov rdi, 117
	push rax
	push rdx
	sub rsp, 0x18
	movdqu [rsp], xmm0

_microprofile_tramp_leave:
	#PATCH 3 INTERCEPT1
	mov rax, 0x0102030405060708
	call rax #jump to proxy

_microprofile_tramp_call_patch_pop:
	mov rax, 0x0102030405060708
	call rax
	mov r11, rax

	movdqu xmm0, [rsp]
	add rsp, 0x18

	pop rdx
	pop rax
	jmp r11

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
	ret
 	int3
_microprofile_tramp_trunk: #used for moved constants.
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
_microprofile_tramp_end:
