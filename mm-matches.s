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
@ use the name main here, for standalone testing of the assembler code
@ when integrating this code into master-mind.c, choose a different name
@ otw there will be a clash with the main function in the C code
.global         main
main:
    LDR  R2, =secret  @ pointer to secret sequence
    LDR  R3, =guess   @ pointer to guess sequence

    @ initialize any other values here if needed

    BL matches        @ call the matching function

    @ handle the result if needed

    @ exit the program
    MOV  R7, #1       @ load system call code
    SWI  0            @ return this value


@ -----------------------------------------------------------------------------
@ sub-routines

@ this is the matching fct that should be callable from C  
.matches:
    MOV R2, #0         @ exactMatches = 0
    MOV R3, #0         @ approximateMatches = 0
    MOV R4, #0         @ used array to mark elements already matched
    LDR R5, =LEN       @ load length of sequence
    LDR R6, =COL       @ load number of colors
    LDR R7, =secret    @ load pointer to secret sequence
    LDR R8, =guess     @ load pointer to guess sequence

    @ Loop through each element in the sequences
    MOV R9, #0         @ index i = 0
    B   loop_condition

loop:
    CMP R9, R5         @ check if i < length
    BGE end_loop       @ exit loop if i >= length

loop_condition:
    LDR R10, [R7, R9, LSL #2]  @ load secret[i]
    LDR R11, [R8, R9, LSL #2]  @ load guess[i]
    CMP R10, R11       @ compare secret[i] and guess[i]
    BEQ exact_match    @ if they are equal, it's an exact match
    MOV R12, #0        @ j = 0
    B   inner_loop_condition

inner_loop:
    CMP R12, R5        @ check if j < length
    BGE inner_end_loop @ exit inner loop if j >= length

inner_loop_condition:
    LDR R13, [R7, R12, LSL #2] @ load secret[j]
    CMP R13, R11       @ compare secret[j] and guess[i]
    BEQ approx_match   @ if they are equal, it's an approximate match
    ADD R12, R12, #1   @ increment j
    B   inner_loop

approx_match:
    CMP R4, R12        @ check if guess[i] is already marked as used
    BNE inner_end_loop @ if not marked, continue
    B   inner_loop      @ if marked, skip to next iteration

exact_match:
    ADD R2, R2, #1     @ increment exactMatches
    MOV R4, R9          @ mark guess[i] as used
    B   end_inner_loop  @ exit inner loop

end_inner_loop:
    ADD R9, R9, #1     @ increment i
    B   loop

end_loop:
    MOV R0, R2, LSL #4  @ move exactMatches to higher 4 bits of R0
    ORR R0, R0, R3      @ combine exactMatches and approximateMatches
    BX  LR              @ return

.end


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
guess:  .word 3 
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
guess1: .word 1 
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
guess2: .word 3 
    .word 3 
    .word 1 

@ Not strictly necessary, but can be used to test the result    
@ Expect Answer: 1 0
.align 4
expect2: .byte 1
     .byte 0










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
	LDR R2, =seq1       @ pointer to secret sequence
	LDR R3, =seq2		@ pointer to guess sequence
    LDR R5, =SEQL       @ load length of sequence
    LDR R6, =COL        @ load number of colors
 

	@ you probably need to initialise more values here

	@ ... COMPLETE THE CODE BY ADDING YOUR CODE HERE, you should use sub-routines to structure your code

	@ Call the matching function
	BL matches

exit:	@MOV	 R0, R4		@ load result to output register
	    MOV 	 R7, #1		@ load system call code
	    SWI 	 0		    @ return this value

@ -----------------------------------------------------------------------------
@ sub-routines

@ this is the matching fct that should be callable from C	
matches:			@ Input: R0, R1 ... ptr to int arrays to match @ Output: R0 ... exact matches (10s) and approx matches (1s) of base COLORS
	@ COMPLETE THE CODE HERE

	@Modified by larry

    @ Initialize exact and approximate matches counters
    MOV  R2, #0      @ exact = 0
    MOV  R3, #0      @ approximate = 0

    @ Loop through the sequences
outer_loop:
    CMP  R1, #0      @ Check if seq1 pointer is NULL (end of sequence)
    BEQ  end_counting

    @ Load seq1[i] into R4
    LDR  R4, [R0], #4    @ Load value from seq1 and increment pointer
    CMP  R4, #0          @ Check if seq1[i] is 0 (end of sequence)
    BEQ  end_counting

    @ Load seq2[i] into R5
    LDR  R5, [R1], #4    @ Load value from seq2 and increment pointer

    @ Compare seq1[i] and seq2[i]
    CMP  R4, R5
    BEQ  exact_match

    @ No exact match, check for approximate match
    @ Loop through seq1 to find match for seq2[i]
    MOV  R6, #0          @ Initialize j = 0
inner_loop:
    CMP  R6, #3          @ Check if j >= SEQL (assumed to be 3)
    BEQ  outer_loop      @ If j >= SEQL, exit inner loop

    @ Load seq1[j] into R7
    LDR  R7, [R0, R6, LSL #2]    @ Load value from seq1[j]

    @ Compare seq1[j] and seq2[i]
    CMP  R7, R5
    BEQ  approximate_match

    @ No match found, increment j and repeat inner loop
    ADD  R6, R6, #1      @ Increment j
    B    inner_loop      @ Repeat inner loop

    @ Exact match found
exact_match:
    ADD  R2, R2, #1      @ Increment exact counter
    B    outer_loop      @ Continue outer loop

    @ Approximate match found
approximate_match:
    ADD  R3, R3, #1      @ Increment approximate counter
    B    outer_loop      @ Continue outer loop

end_counting:
    @ Combine exact and approximate counters into one value
    MOV  R0, R2, LSL #4  @ Shift left by 4 bits to put exact matches in tens place
    ADD  R0, R0, R3      @ Add approximate matches to combine result

    @ Function epilogue if needed
    BX   LR


@ show the sequence in R0, use a call to printf in libc to do the printing, a useful function when debugging 
showseq: 			@ Input: R0 = pointer to a sequence of 3 int values to show
	@ COMPLETE THE CODE HERE (OPTIONAL)
	
	
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
