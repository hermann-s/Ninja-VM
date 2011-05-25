//
// execution framework
//
_start:
	call	main
	call	exit
_stop:
	jmp	_stop

//
// integer read()
//
read:
	asf	0
	rdint
	popr
	rsf
	ret

//
// void write(integer)
//
write:
	asf	0
	pushl	-3
	wrint
	rsf
	ret

//
// void exit()
//
exit:
	asf	0
	halt
	rsf
	ret

//
// void f(boolean)
//
f:
	asf	2
	pushl	-3
	brf	_L1
	pushc	2
	popl	0
	pushc	22
	popl	1
	jmp	_L2
_L1:
	pushc	3
	popl	0
	pushc	33
	popl	1
_L2:
_L0:
	rsf
	ret

//
// void main()
//
main:
	asf	0
_L3:
	rsf
	ret
