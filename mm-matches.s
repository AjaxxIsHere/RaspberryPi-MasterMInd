@ This ARM Assembler code should implement a matching function, for use in the MasterMind program, as
@ described in the CW2 specification. It should produce as output 2 numbers, the first for the
@ exact matches (peg of right colour and in right position) and approximate matches (peg of right
@ color but not in right position). Make sure to count each peg just once!
	
@ Example (first sequence is secret, second sequence is guess):
@ 1 2 1
@ 3 1 3 ==> 0 1
@ You can return the result as a pointer to two numbers, or two values
@ encoded within one number
@
@ -----------------------------------------------------------------------------

.text
@ this is the matching fct that should be called from the C part of the CW	
.global         matches


 
@ use the name `main` here, for standalone testing of the assembler code
@ when integrating this code into `master-mind.c`, choose a different name
@ otw there will be a clash with the main function in the C code
.global         main_asm
main_asm: 
	LDR  R2, =secret	@ pointer to secret sequence
	LDR  R3, =guess		@ pointer to guess sequence

	@ you probably need to initialise more values here

	@ ... COMPLETE THE CODE BY ADDING YOUR CODE HERE, you should use sub-routines to structure your code
    @ Modified by Leressa
    @ Initialize any additional variables or registers needed for the matching function
    MOV  R4, #0      @ Counter for exact matches
    MOV  R5, #0      @ Counter for approximate matches

    @ Loop through the sequences
outer_loop:
    CMP  R2, #0      @ Check if secret pointer is NULL (end of sequence)
    BEQ  end_counting

    @ Load secret[i] into R6
    LDR  R6, [R2], #4    @ Load value from secret and increment pointer
    CMP  R6, #0          @ Check if secret[i] is 0 (end of sequence)
    BEQ  end_counting

    @ Load guess[i] into R7
    LDR  R7, [R3], #4    @ Load value from guess and increment pointer

    @ Compare secret[i] and guess[i]
    CMP  R6, R7
    BEQ  exact_match

    @ No exact match, check for approximate match
    @ Loop through secret to find match for guess[i]
    MOV  R8, #0          @ Initialize j = 0
inner_loop:
    CMP  R8, #3          @ Check if j >= LEN (assumed to be 3)
    BEQ  outer_loop      @ If j >= LEN, exit inner loop

    @ Load secret[j] into R9
    LDR  R9, [R2, R8, LSL #2]    @ Load value from secret[j]

    @ Compare secret[j] and guess[i]
    CMP  R9, R7
    BEQ  approximate_match

    @ No match found, increment j and repeat inner loop
    ADD  R8, R8, #1      @ Increment j
    B    inner_loop      @ Repeat inner loop

    @ Exact match found
exact_match:
    ADD  R4, R4, #1      @ Increment exact counter
    B    outer_loop      @ Continue outer loop

    @ Approximate match found
approximate_match:
    ADD  R5, R5, #1      @ Increment approximate counter
    B    outer_loop      @ Continue outer loop

end_counting:
    @ Combine exact and approximate counters into one value
    MOV  R0, R4, LSL #4  @ Shift left by 4 bits to put exact matches in tens place
    ADD  R0, R0, R5      @ Add approximate matches to combine result

    @ Function epilogue if needed
    BX   LR


@ show the sequence in R0, use a call to printf in libc to do the printing, a useful function when debugging 
showseq: 			@ Input: R0 = pointer to a sequence of 3 int values to show
	@ COMPLETE THE CODE HERE (OPTIONAL)
    
    @ Modified by Leressa
    @ Print the sequence using printf
    PUSH {LR}           @ Save the return address
    LDR  R1, =f4str     @ Load the format string
    LDR  R2, [R0]       @ Load the first element of the sequence
    LDR  R3, [R0, #4]   @ Load the second element of the sequence
    LDR  R4, [R0, #8]   @ Load the third element of the sequence
    BL   printf         @ Call printf to print the sequence
    POP  {PC}           @ Restore the return address and return
	
	
@ =============================================================================

.data

@ constants about the basic setup of the game: length of sequence and number of colors	
.equ LEN, 3
.equ COL, 3
.equ NAN1, 8
.equ NAN2, 9

@ a format string for printf that can be used in showseq
f4str: .asciz "Seq:    %d %d %d\n"

@ a memory location, initialised as 0, you may need this in the matching fct
n: .word 0x00
	
@ INPUT DATA for the matching function
.align 4
secret: .word 1 
	.word 2 
	.word 1 

.align 4
guess:	.word 3 
	.word 1 
	.word 3 

@ Not strictly necessary, but can be used to test the result	
@ Expect Answer: 0 1
.align 4
expect: .byte 0
	.byte 1

.align 4
secret1: .word 1 
	 .word 2 
	 .word 3 

.align 4
guess1:	.word 1 
	.word 1 
	.word 2 

@ Not strictly necessary, but can be used to test the result	
@ Expect Answer: 1 1
.align 4
expect1: .byte 1
	 .byte 1

.align 4
secret2: .word 2 
	 .word 3
	 .word 2 

.align 4
guess2:	.word 3 
	.word 3 
	.word 1 

@ Not strictly necessary, but can be used to test the result	
@ Expect Answer: 1 0
.align 4
expect2: .byte 1
	 .byte 0