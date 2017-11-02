.section __TEXT,__text
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
	mov (%rsp), %r12
	# mov (%rsp), %(rax)

	pushq %rdi
	pushq %rsi
	pushq %rdx
	pushq %rcx
	pushq %r8
	pushq %r9
	sub $88h, %rsp

	movdqu %xmm7, 70h(%rsp)
	movdqu %xmm6, 60h(%rsp)
	movdqu %xmm5, 50h(%rsp)
	movdqu %xmm4, 40h(%rsp)
	movdqu %xmm3, 30h(%rsp)
	movdqu %xmm2, 20h(%rsp)
	movdqu %xmm1, 10h(%rsp)
	movdqu %xmm0, (%rsp)

	#call _microprofile_tramp_get_rsp_loc
	mov %r12, %rdi
_microprofile_tramp_call_patch_push:
	movq $0102030405060708h, %rax
	call *%rax
	test %rax, %rax
	jz _microprofile_tramp_fail  #if push fails, skip to call code, and dont patch return adress.
	##todo check ret val
	# movq %r12, (%rax)

_microprofile_tramp_enter_patch:
	# PATCH 1 TRAMP EXIT
	movq $0102030405060708h, %rax #patch to tramp_code_end
	movq %rax, 0xb8(%rsp)




_microprofile_tramp_arg0:
	movq $42, %rdi  ## patch 1.5 : arg
_microprofile_tramp_intercept0:
#PATCH 2 INTERCEPT0
	movq $0102030405060708h, %rax
	call *%rax
_microprofile_tramp_fail:
	movdqu 70h(%rsp), %xmm7
	movdqu 60h(%rsp), %xmm6
	movdqu 50h(%rsp), %xmm5
	movdqu 40h(%rsp), %xmm4
	movdqu 30h(%rsp), %xmm3
	movdqu 20h(%rsp), %xmm2
	movdqu 10h(%rsp), %xmm1
	movdqu (%rsp), %xmm0
	add $88h, %rsp
	popq %r9
	popq %r8
	popq %rcx
	popq %rdx
	popq %rsi
	popq %rdi
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
	movq $117, %rdi ## patch 2.5 : arg
	pushq %rax
	pushq %rdx
	sub $18h, %rsp
	movdqu %xmm0, (%rsp)

_microprofile_tramp_leave:
	#PATCH 3 INTERCEPT1
	movq $0102030405060708h, %rax 
	call *%rax #jump to proxy

_microprofile_tramp_call_patch_pop:
	movq $0102030405060708h, %rax
	call *%rax
	movq %rax, %r11

	movdqu (%rsp), %xmm0
	add $18h, %rsp

	popq %rdx
	popq %rax
	jmp *%r11

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
