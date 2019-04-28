.text

.globl schedule
.type  schedule,"function"

schedule:
	#save old ebp
	pushl   %ebp

	#switch kernel stacks
        movl    %esp, (%ecx)
	movl    %edx, %esp

	#load new ebp
	popl   %ebp
	ret

