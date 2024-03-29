/* ***************************************************************************** */
/* You can use this file to define the low-level hardware control fcts for       */
/* LED, button and LCD devices.                                                  */ 
/* Note that these need to be implemented in Assembler.                          */
/* You can use inline Assembler code, or use a stand-alone Assembler file.       */
/* Alternatively, you can implement all fcts directly in master-mind.c,          */  
/* using inline Assembler code there.                                            */
/* The Makefile assumes you define the functions here.                           */
/* ***************************************************************************** */


#ifndef	TRUE
#  define	TRUE	(1==1)
#  define	FALSE	(1==2)
#endif

#define	PAGE_SIZE		(4*1024)
#define	BLOCK_SIZE		(4*1024)

#define	INPUT			 0
#define	OUTPUT			 1

#define	OFF			 0
#define	ON			 1


// APP constants   ---------------------------------

// Wiring (see call to lcdInit in main, using BCM numbering)
// NB: this needs to match the wiring as defined in master-mind.c

#define STRB_PIN 24
#define RS_PIN   25
#define DATA0_PIN 23
#define DATA1_PIN 10
#define DATA2_PIN 27
#define DATA3_PIN 22

// -----------------------------------------------------------------------------
// includes 
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>

// -----------------------------------------------------------------------------
// prototypes

int failure (int fatal, const char *message, ...);

// -----------------------------------------------------------------------------
// Functions to implement here (or directly in master-mind.c)

/* this version needs gpio as argument, because it is in a separate file */
void digitalWriteAsm (uint32_t *gpio, int pin, int value) {
  /* ***  COMPLETE the code here, using inline Assembler  ***  */
  asm volatile(
    "ldr r3, [%[gpio], #0x1C]\n"   // Load the GPFSEL register address into r3
    "ldr r4, [%[gpio], #0x28]\n"   // Load the GPSET register address into r4
    "ldr r5, [%[gpio], #0x34]\n"   // Load the GPCLR register address into r5
    "mov r6, %[pin]\n"             // Move the pin value into r6
    "mov r7, %[value]\n"           // Move the value into r7

    "and r0, r6, #0x1F\n"          // Mask the pin value to ensure it is within range
    "lsr r1, r6, #5\n"             // Shift the pin value right by 5 to get the register index
    "lsl r2, r0, #2\n"             // Shift the pin value left by 2 to get the bit offset

    "ldr r8, [r3, r1, lsl #2]\n"   // Load the value of the GPFSEL register for the corresponding register index
    "bic r8, r8, #7\n"             // Clear the bits for the pin in the GPFSEL register
    "orr r8, r8, #1\n"             // Set the bits for the pin to output mode in the GPFSEL register
    "str r8, [r3, r1, lsl #2]\n"   // Store the modified value back into the GPFSEL register

    "cmp r7, #0\n"                 // Compare the value with 0
    "beq .clear\n"                 // Branch to .clear if the value is 0
    "str r2, [r4, r1, lsl #2]\n"   // Store the bit offset into the GPSET register
    "b .end\n"                     // Branch to .end

    ".clear:\n"
    "str r2, [r5, r1, lsl #2]\n"   // Store the bit offset into the GPCLR register

    ".end:\n"
    :
    : [gpio] "r" (gpio), [pin] "r" (pin), [value] "r" (value)
    : "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8"
  );
}

// adapted from setPinMode
void pinModeAsm(uint32_t *gpio, int pin, int mode /*, int fSel, int shift */) {
  /* ***  COMPLETE the code here, using inline Assembler  ***  */
  asm volatile(
    "ldr r3, [%[gpio], #0x1C]\n"   // Load the GPFSEL register address into r3
    "mov r4, %[pin]\n"             // Move the pin value into r4
    "mov r5, %[mode]\n"            // Move the mode value into r5

    "and r0, r4, #0x1F\n"          // Mask the pin value to ensure it is within range
    "lsr r1, r4, #5\n"             // Shift the pin value right by 5 to get the register index
    "lsl r2, r0, #2\n"             // Shift the pin value left by 2 to get the bit offset

    "ldr r6, [r3, r1, lsl #2]\n"   // Load the value of the GPFSEL register for the corresponding register index
    "bic r6, r6, #7\n"             // Clear the bits for the pin in the GPFSEL register
    "orr r6, r6, r5\n"             // Set the bits for the pin to the mode value in the GPFSEL register
    "str r6, [r3, r1, lsl #2]\n"   // Store the modified value back into the GPFSEL register
    :
    : [gpio] "r" (gpio), [pin] "r" (pin), [mode] "r" (mode)
    : "r0", "r1", "r2", "r3", "r4", "r5", "r6"
  );
}

void writeLEDAsm(uint32_t *gpio, int led, int value)
{
  asm volatile(
    "ldr r0, [%[gpio]]\n\t"
    "mov r1, %[led]\n\t"
    "mov r2, %[value]\n\t"
    "cmp r2, #0\n\t"
    "beq clear\n\t"
    "orrs r0, r0, #1 << r1\n\t"
    "str r0, [%[gpio]]\n\t"
    "b end\n\t"
    "clear:\n\t"
    "bics r0, r0, #1 << r1\n\t"
    "str r0, [%[gpio]]\n\t"
    "end:\n\t"
    :
    : [gpio] "r" (gpio), [led] "r" (led), [value] "r" (value)
    : "r0", "r1", "r2"
  );
}

int readButtonAsm(uint32_t *gpio, int button) {
  int state;
  asm volatile(
    "ldr r3, [%[gpio], #0x34]\n"   // Load the GPLEV register address into r3
    "mov r4, %[button]\n"          // Move the button value into r4

    "and r0, r4, #0x1F\n"          // Mask the button value to ensure it is within range
    "lsr r1, r4, #5\n"             // Shift the button value right by 5 to get the register index
    "lsl r2, r0, #2\n"             // Shift the button value left by 2 to get the bit offset

    "ldr r5, [r3, r1, lsl #2]\n"   // Load the value of the GPLEV register for the corresponding register index
    "and r5, r5, r2\n"             // Mask the bit offset in the GPLEV register
    "cmp r5, #0\n"                 // Compare the value with 0
    "moveq %[state], #1\n"         // Move 1 into state if the value is 0
    "movne %[state], #0\n"         // Move 0 into state if the value is not 0
    : [state] "=r" (state)
    : [gpio] "r" (gpio), [button] "r" (button)
    : "r0", "r1", "r2", "r3", "r4", "r5"
  );
  return state;
}

int waitForButtonAsm(uint32_t *gpio, int button)
{
  // fprintf(stderr, "int state = readButton(gpio, button);Waiting for button\n");
  // int state = readButton(gpio, button);
  while (1)
  {
    int state = readButton(gpio, button);

    fprintf(stderr, "Button state: %d\n", state);
    if (state == ON)
    {
      fprintf(stderr, "Button pressed\n");
      return 1;
      break;
    }
    else
    {
      // state = ON;
      struct timespec sleeper, dummy;
      sleeper.tv_sec = 0;
      sleeper.tv_nsec = 100000000;
      nanosleep(&sleeper, &dummy);
      break;
    }
  }
}
