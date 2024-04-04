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
.global         main
main: 
    LDR R2, =secret       @ pointer to secret sequence
    LDR R3, =guess         @ pointer to guess sequence

    @ Initialize any other variables you might need

    BL matches             @ Call the 'matches' function 
    MOV R4, R0             @ Store the result from 'matches' in R4

exit:   
    @MOV   R0, R4          @ Load the final result to output register
    MOV   R7, #1          @ Load system call code (exit)
    SWI   0                @ Return this value (exit the program)


@ -----------------------------------------------------------------------------
@ sub-routines

@ this is the matching fct that should be callable from C	
matches:
    @ Initialize variables for counting matches 
    MOV  R4, #0  ; Initialize exact match count to 0
    MOV  R5, #0  ; Initialize approximate match count to 0  

    @ Outer loop iterates through the 'secret' sequence
    MOV  R6, #0  ; Index for outer loop (i)

outer_loop:
    CMP  R6, #LEN  ; Check if we've iterated through all elements
    BGE  end_outer_loop  

    LDR  R0, [R2, R6, LSL #2] ; Load secret[i] into R0

    @ Inner loop to compare current 'secret' element with entries in 'guess' sequence 
    MOV  R7, #0  ; Index for inner loop (j)

inner_loop:
    CMP  R7, #LEN
    BGE  end_inner_loop

    LDR  R1, [R3, R7, LSL #2]  ; Load guess[j] into R1

    @ Check for exact match
    CMP  R0, R1
    BEQ  exact_match  

    @ Check for approximate match (needs logic to avoid double-counting)
    ... ; Implement approximate match logic

    ADD  R7, R7, #1 ; Increment inner loop index
    B    inner_loop

exact_match:
    ADD  R4, R4, #1  ; Increment exact match counter
    B    end_inner_loop  ; Skip the rest of the inner loop comparisons 

end_inner_loop:
    ADD  R6, R6, #1  ; Increment outer loop index
    B    outer_loop

end_outer_loop:
    @ Combine the counts into a single result
    MOV  R0, R4
    LSL  R0, R0, #4   ; Shift exact match count to the tens digit
    ORR  R0, R0, R5   ; Combine with approximate match count
    BX   LR           ; Return from the function


@ show the sequence in R0, use a call to printf in libc to do the printing, a useful function when debugging 
showseq: 
    PUSH {LR}  ; Save the return address

    @ Load values and prepare for `printf` call
    LDR  R1, =f4str  ; Load format string address
    MOV  R2, #LEN    ; Number of elements to print is sequence length

    @ Loop to load and print each element
    MOV R6, #0  ; Index
print_loop:
      LDR R0, [R0, R6, LSL #2]  ; Load value
      BL  printf
      ADD R6, R6, #1
      CMP R6, R2
      BLT print_loop 

    POP {PC}  ; Restore return address and return

	
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
