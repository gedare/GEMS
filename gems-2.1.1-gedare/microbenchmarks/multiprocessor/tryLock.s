.seg "text"
	.align 4

	.global tryLock
	.type tryLock, #function
	
tryLock:
	/* %o0 contains addr of the lock; word-aligned so we add 3 bytes before
	doing our ldstub, so we only write to the least significant byte */
ldstub [%o0+3], %o0 /* %o0 serves as both the input param & the result */
retl
nop
