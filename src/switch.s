.align	4
.globl	uthread_build_stack
.globl	_uthread_build_stack
uthread_build_stack:
_uthread_build_stack:
	movl 4(%esp), %eax 
	movl %ebp, 0(%eax)
	movl %esp, 4(%eax)
	movl %ebx, 8(%eax)
	movl %edi, 12(%eax)
	movl %esi, 16(%eax)
	ret
.align	4
.globl	uthread_switch
.globl	_uthread_switch
uthread_switch:
_uthread_switch: ##arg from to
	movl 4(%esp), %eax 
	movl %ebp, 0(%eax)
	movl %esp, 4(%eax)
	movl %ebx, 8(%eax)
	movl %edi, 12(%eax)
	movl %esi, 16(%eax)
	movl 8(%esp), %eax
	movl 12(%esp),%edx
	movl 0(%eax), %ebp 
	movl 4(%eax), %esp
	movl 8(%eax), %ebx
	movl 12(%eax),%edi
	movl 16(%eax),%esi
	movl %edx,%eax
	ret	
.align	4
.globl	uthread_start_run
.globl	_uthread_start_run
uthread_start_run:
_uthread_start_run:
	movl 4(%esp),%eax
	movl 8(%esp),%ecx
	movl 12(%esp),%edi
	movl %esp,%edx
	movl %ebp,%ebx
	movl 0(%eax),%ebp
	movl %ebp,%esp
	pushl %edx
	pushl %ebx
	pushl %edi
	pushl %eax
	call *%ecx
	popl %eax
	popl %edi
	popl %ebx
	popl %edx
	movl %edx,%esp
	movl %ebx,%ebp
	ret
	