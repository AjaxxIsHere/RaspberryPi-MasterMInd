/*
 * MasterMind implementation: template; see comments below on which parts need to be completed
 * CW spec: https://www.macs.hw.ac.uk/~hwloidl/Courses/F28HS/F28HS_CW2_2022.pdf
 * This repo: https://gitlab-student.macs.hw.ac.uk/f28hs-2021-22/f28hs-2021-22-staff/f28hs-2021-22-cwk2-sys

 * Compile:
 gcc -c -o lcdBinary.o lcdBinary.c
 gcc -c -o master-mind.o master-mind.c
 gcc -o master-mind master-mind.o lcdBinary.o
 * Run:
 sudo ./master-mind

 OR use the Makefile to build
 > make all
 and run
 > make run
 and test
 > make test

 ***********************************************************************
 * The Low-level interface to LED, button, and LCD is based on:
 * wiringPi libraries by
 * Copyright (c) 2012-2013 Gordon Henderson.
 ***********************************************************************
 * See:
 *	https://projects.drogon.net/raspberry-pi/wiringpi/
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with wiringPi.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
*/

/* ======================================================= */
/* SECTION: includes                                       */
/* ------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h> // added for timer

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <bits/getopt_core.h>
// #include <asm-generic/fcntl.h>

/* --------------------------------------------------------------------------- */
/* Config settings */
/* you can use CPP flags to e.g. print extra debugging messages */
/* or switch between different versions of the code e.g. digitalWrite() in Assembler */
#define DEBUG
#undef ASM_CODE

// =======================================================
// Tunables
// PINs (based on BCM numbering)
// For wiring see CW spec: https://www.macs.hw.ac.uk/~hwloidl/Courses/F28HS/F28HS_CW2_2022.pdf

#define GREEN_LED 13 // GPIO pin for green LED
#define RED_LED 5    // GPIO pin for red LED
#define BUTTON 19    // GPIO pin for button
// =======================================================
// delay for loop iterations (mainly), in ms
#define DELAY 200       // in mili-seconds: 0.2s
#define TIMEOUT 3000000 // in micro-seconds: 3s
// =======================================================
// APP constants   ---------------------------------
#define COLS 3 // Number of colours
#define SEQL 3 // Number of the length of the sequence
// =======================================================

// generic constants

#ifndef TRUE
#define TRUE (1 == 1)
#define FALSE (1 == 2)
#endif

#define PAGE_SIZE (4 * 1024)
#define BLOCK_SIZE (4 * 1024)

#define INPUT 0
#define OUTPUT 1

#define OFF 0
#define ON 1

// =======================================================
// Wiring (see inlined initialisation routine)

#define STRB_PIN 24
#define RS_PIN 25
#define DATA0_PIN 23
#define DATA1_PIN 18
#define DATA2_PIN 27
#define DATA3_PIN 22

/* ======================================================= */
/* SECTION: constants and prototypes                       */
/* ------------------------------------------------------- */

// =======================================================
// char data for the CGRAM, i.e. defining new characters for the display

static unsigned char newChar[8] =
    {
        0b11111,
        0b10001,
        0b10001,
        0b10101,
        0b11111,
        0b10001,
        0b10001,
        0b11111,
};

/* Constants */

static const int colors = COLS; // Store the number of colours in the sequence
static const int seqlen = SEQL; // Store the length of the sequence

static char *color_names[] = {"red", "green", "blue"}; // Store the names of the colours

static int *theSeq = NULL; // Store the secret sequence

static int *seq1, *seq2, *cpy1, *cpy2; // Store the guess sequence and copies for use in countMatches

/* --------------------------------------------------------------------------- */

// data structure holding data on the representation of the LCD
struct lcdDataStruct
{
  int bits, rows, cols;
  int rsPin, strbPin;
  int dataPins[8];
  int cx, cy;
};

static int lcdControl;

/* ***************************************************************************** */
/* INLINED fcts from wiringPi/devLib/lcd.c: */
// HD44780U Commands (see Fig 11, p28 of the Hitachi HD44780U datasheet)

#define LCD_CLEAR 0x01
#define LCD_HOME 0x02
#define LCD_ENTRY 0x04
#define LCD_CTRL 0x08
#define LCD_CDSHIFT 0x10
#define LCD_FUNC 0x20
#define LCD_CGRAM 0x40
#define LCD_DGRAM 0x80

// Bits in the entry register

#define LCD_ENTRY_SH 0x01
#define LCD_ENTRY_ID 0x02

// Bits in the control register

#define LCD_BLINK_CTRL 0x01
#define LCD_CURSOR_CTRL 0x02
#define LCD_DISPLAY_CTRL 0x04

// Bits in the function register

#define LCD_FUNC_F 0x04
#define LCD_FUNC_N 0x08
#define LCD_FUNC_DL 0x10

#define LCD_CDSHIFT_RL 0x04

// Mask for the bottom 64 pins which belong to the Raspberry Pi
//	The others are available for the other devices

#define PI_GPIO_MASK (0xFFFFFFC0)

static unsigned int gpiobase;
static uint32_t *gpio;

static int timed_out = 0;

/* ------------------------------------------------------- */
// misc prototypes

int failure(int fatal, const char *message, ...);
void waitForEnter(void);
int waitForButton(uint32_t *gpio, int button);

/* ======================================================= */
/* SECTION: hardware interface (LED, button, LCD display)  */
/* ------------------------------------------------------- */
/* low-level interface to the hardware */

/* ********************************************************** */
/* COMPLETE the code for all of the functions in this SECTION */
/* Either put them in a separate file, lcdBinary.c, and use   */
/* inline Assembler there, or use a standalone Assembler file */
/* You can also directly implement them here (inline Asm).    */
/* ********************************************************** */

/* These are just prototypes; you need to complete the code for each function */

/* send a @value@ (LOW or HIGH) on pin number @pin@; @gpio@ is the mmaped GPIO base address */
// Modified by AJ
void digitalWrite(uint32_t *gpio, int pin, int value)
{
  if (value == OFF)
  {
    *(gpio + 10) = 1 << pin;
  }
  else
  {
    *(gpio + 7) = 1 << pin;
  }
};

/* set the @mode@ of a GPIO @pin@ to INPUT or OUTPUT; @gpio@ is the mmaped GPIO base address */
// Modified by AJ
void pinMode(uint32_t *gpio, int pin, int mode)
{
  if (mode == OUTPUT)
  {
    *(gpio + ((pin) / 10)) = (*(gpio + ((pin) / 10)) & ~(7 << (((pin) % 10) * 3))) | (1 << (((pin) % 10) * 3));
  }
  else
  {
    *(gpio + ((pin) / 10)) = (*(gpio + ((pin) / 10)) & ~(7 << (((pin) % 10) * 3)));
  }
};

/* send a @value@ (LOW or HIGH) on pin number @pin@; @gpio@ is the mmaped GPIO base address */
/* can use digitalWrite(), depending on your implementation */
// Modified by AJ
void writeLED(uint32_t *gpio, int led, int value)
{
  if (value == ON)
  {
    *gpio |= 1 << led;
  }
  else
  {
    *gpio &= ~(1 << led);
  }
};

/* read a @value@ (OFF or ON) from pin number @pin@ (a button device); @gpio@ is the mmaped GPIO base address */
// Modified by AJ
int readButton(uint32_t *gpio, int pin)
{
  if ((pin & 0xFFFFFFC0) == 0)
  {
    if ((*(gpio + 13) & (1 << pin)) == 0)
    {
      return OFF;
    }
    else
    {
      return ON;
    }
  }
  else
  {
    fprintf(stderr, "Error: Invalid pin number\n");
    exit(EXIT_FAILURE);
  }
}

/* wait for a button input on pin number @button@; @gpio@ is the mmaped GPIO base address */
/* can use readButton(), depending on your implementation */
// Modified by AJ
int waitForButton(uint32_t *gpio, int button)
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

/* ======================================================= */
/* SECTION: game logic                                     */
/* ------------------------------------------------------- */
/* AUX fcts of the game logic */

/* ********************************************************** */
/* COMPLETE the code for all of the functions in this SECTION */
/* Implement these as C functions in this file                */
/* ********************************************************** */

/* initialise the secret sequence; by default it should be a random sequence */
// Modified by Leressa
void inititalizeSeq()
{
  srand(time(NULL)); // Seed the random number generator

  if (theSeq == NULL)
  {
    theSeq = (int *)malloc(SEQL * sizeof(int)); // Allocate memory for the sequence if not allocated already
    if (theSeq == NULL)
    {
      fprintf(stderr, "Memory allocation failed for the secret sequence\n");
      exit(EXIT_FAILURE);
    }
  }

  for (int i = 0; i < SEQL; i++)
  {
    theSeq[i] = rand() % 3 + 1; // Generate a random number between 1 and 3
  }
};

/* display the sequence on the terminal window, using the format from the sample run in the spec */
// Modified by Leressa
void showSeq(int *seq)
{
  printf("Sequence: ");
  for (int i = 0; i < SEQL; i++) // Changed SEQ_LENGTH to SEQL
  {
    printf("%d ", seq[i]);
  }
  printf("\n");
};

#define NAN1 8
#define NAN2 9

/* counts how many entries in seq2 match entries in seq1 */
/* returns exact and approximate matches, either both encoded in one value, */
/* or as a pointer to a pair of values */
// Modified by Leressa
int /* or int* */ countMatches(int *seq1, int *seq2)
{

  int exact = 0, approximate = 0;

  for (int j = 0; j < SEQL; j++ ) {
    printf("seq1[%d] = %d, seq2[%d] = %d\n", j, seq1[j], j, seq2[j]);
  }

  // Logic to count exact and approximate matches
  for (int i = 0; i < SEQL; i++)
  {
    
    if (seq1[i] == seq2[i])
    {
      exact++;
    }
    else
    {
      for (int j = 0; j < SEQL; j++)
      {
        if (seq2[i] == seq1[j] && seq1[j] != seq2[j])
        {
          approximate++;
          break;
        }
      }
    }
  }

  // Combine exact and approximate matches into one value
  int result = (exact << 4) | approximate;
  return result;
}

/* show the results from calling countMatches on seq1 and seq1 */
// Modified by Leressa
void showMatches(int /* or int* */ code, /* only for debugging */ int *seq1, int *seq2, /* optional, to control layout */ int lcd_format)
{
  int exact = code >> 4;
  int approximate = code & 0x0F;

  printf("Exact Matches: %d, Approximate Matches: %d\n", exact, approximate);

  // Additional logic to display matches, possibly on LCD
  // Modify this part based on your requirements
}

/* parse an integer value as a list of digits, and put them into @seq@ */
/* needed for processing command-line with options -s or -u            */
// Modified by Leressa
void readSeq(int *seq, int val)
{
  // Extract digits from val and store them in seq
  for (int i = SEQL - 1; i >= 0; i--)
  {
    seq[i] = val % 10;
    val /= 10;
  }
}

/* read a guess sequence fron stdin and store the values in arr */
/* only needed for testing the game logic, without button input */
// Modified by Leressa
int readNum(int max)
{
  int num;

  // Read a number from stdin
  printf("Enter a number between 0 and %d: ", max);
  scanf("%d", &num);

  return num;
}

/* ======================================================= */
/* SECTION: TIMER code                                     */
/* ------------------------------------------------------- */
/* TIMER code */

/* timestamps needed to implement a time-out mechanism */
static uint64_t startT, stopT;

/* ********************************************************** */
/* COMPLETE the code for all of the functions in this SECTION */
/* Implement these as C functions in this file                */
/* ********************************************************** */

/* you may need this function in timer_handler() below  */
/* use the libc fct gettimeofday() to implement it      */
// Modified by AJ
uint64_t timeInMicroseconds()
{
  /* ***  COMPLETE the code here  ***  */
  struct timeval currentTime;
  gettimeofday(&currentTime, NULL);
  return ((unsigned long long)currentTime.tv_sec * 1000000ULL) + (unsigned long long)currentTime.tv_usec;
}

/* this should be the callback, triggered via an interval timer, */
/* that is set-up through a call to sigaction() in the main fct. */
// Modified by AJ, incomplete!
void timer_handler(int signum)
{
  /* ***  COMPLETE the code here  ***  */
  printf("Caught timer signal: %d\n", signum);
}

/* initialise time-stamps, setup an interval timer, and install the timer_handler callback */
// Modified by AJ
void initITimer(uint64_t timeout)
{
  struct itimerval timer;
  // Set up the timer
  timer.it_value.tv_sec = timeout / 1000000;     // seconds
  timer.it_value.tv_usec = timeout % 1000000;    // microseconds
  timer.it_interval.tv_sec = timeout / 1000000;  // repeat interval seconds
  timer.it_interval.tv_usec = timeout % 1000000; // repeat interval microseconds

  // Install timer_handler as the signal handler for SIGVTALRM
  struct sigaction sa;
  sa.sa_handler = &timer_handler;
  sa.sa_flags = SA_RESTART;
  sigfillset(&sa.sa_mask);
  if (sigaction(SIGVTALRM, &sa, NULL) == -1)
  {
    perror("Error: cannot handle SIGVTALRM"); // Handle error
    exit(EXIT_FAILURE);
  }

  // Configure the timer to expire after the specified time
  if (setitimer(ITIMER_VIRTUAL, &timer, NULL) == -1)
  {
    perror("Error: cannot start virtual timer"); // Handle error
    exit(EXIT_FAILURE);
  }
}

/* ======================================================= */
/* SECTION: Aux function                                   */
/* ------------------------------------------------------- */
/* misc aux functions */

int failure(int fatal, const char *message, ...)
{
  va_list argp;
  char buffer[1024];

  if (!fatal) //  && wiringPiReturnCodes)
    return -1;

  va_start(argp, message);
  vsnprintf(buffer, 1023, message, argp);
  va_end(argp);

  fprintf(stderr, "%s", buffer);
  exit(EXIT_FAILURE);

  return 0;
}

/*
 * waitForEnter:
 *********************************************************************************
 */

void waitForEnter(void)
{
  printf("Press ENTER to continue: ");
  (void)fgetc(stdin);
}

/*
 * delay:
 *	Wait for some number of milliseconds
 *********************************************************************************
 */

void delay(unsigned int howLong)
{
  struct timespec sleeper, dummy;

  sleeper.tv_sec = (time_t)(howLong / 1000);
  sleeper.tv_nsec = (long)(howLong % 1000) * 1000000;

  nanosleep(&sleeper, &dummy);
}

/* From wiringPi code; comment by Gordon Henderson
 * delayMicroseconds:
 *	This is somewhat intersting. It seems that on the Pi, a single call
 *	to nanosleep takes some 80 to 130 microseconds anyway, so while
 *	obeying the standards (may take longer), it's not always what we
 *	want!
 *
 *	So what I'll do now is if the delay is less than 100uS we'll do it
 *	in a hard loop, watching a built-in counter on the ARM chip. This is
 *	somewhat sub-optimal in that it uses 100% CPU, something not an issue
 *	in a microcontroller, but under a multi-tasking, multi-user OS, it's
 *	wastefull, however we've no real choice )-:
 *
 *      Plan B: It seems all might not be well with that plan, so changing it
 *      to use gettimeofday () and poll on that instead...
 *********************************************************************************
 */

void delayMicroseconds(unsigned int howLong)
{
  struct timespec sleeper;
  unsigned int uSecs = howLong % 1000000;
  unsigned int wSecs = howLong / 1000000;

  /**/ if (howLong == 0)
    return;
#if 0
  else if (howLong  < 100)
    delayMicrosecondsHard (howLong) ;
#endif
  else
  {
    sleeper.tv_sec = wSecs;
    sleeper.tv_nsec = (long)(uSecs * 1000L);
    nanosleep(&sleeper, NULL);
  }
}

/* ======================================================= */
/* SECTION: LCD functions                                  */
/* ------------------------------------------------------- */
/* medium-level interface functions (all in C) */

/* from wiringPi:
 * strobe:
 *	Toggle the strobe (Really the "E") pin to the device.
 *	According to the docs, data is latched on the falling edge.
 *********************************************************************************
 */

void strobe(const struct lcdDataStruct *lcd)
{

  // Note timing changes for new version of delayMicroseconds ()
  digitalWrite(gpio, lcd->strbPin, 1);
  delayMicroseconds(50);
  digitalWrite(gpio, lcd->strbPin, 0);
  delayMicroseconds(50);
}

/*
 * sentDataCmd:
 *	Send an data or command byte to the display.
 *********************************************************************************
 */

void sendDataCmd(const struct lcdDataStruct *lcd, unsigned char data)
{
  register unsigned char myData = data;
  unsigned char i, d4;

  if (lcd->bits == 4)
  {
    d4 = (myData >> 4) & 0x0F;
    for (i = 0; i < 4; ++i)
    {
      digitalWrite(gpio, lcd->dataPins[i], (d4 & 1));
      d4 >>= 1;
    }
    strobe(lcd);

    d4 = myData & 0x0F;
    for (i = 0; i < 4; ++i)
    {
      digitalWrite(gpio, lcd->dataPins[i], (d4 & 1));
      d4 >>= 1;
    }
  }
  else
  {
    for (i = 0; i < 8; ++i)
    {
      digitalWrite(gpio, lcd->dataPins[i], (myData & 1));
      myData >>= 1;
    }
  }
  strobe(lcd);
}

/*
 * lcdPutCommand:
 *	Send a command byte to the display
 *********************************************************************************
 */

void lcdPutCommand(const struct lcdDataStruct *lcd, unsigned char command)
{
#ifdef DEBUG
  fprintf(stderr, "lcdPutCommand: digitalWrite(%d,%d) and sendDataCmd(%d,%d)\n", lcd->rsPin, 0, lcd, command);
#endif
  digitalWrite(gpio, lcd->rsPin, 0);
  sendDataCmd(lcd, command);
  delay(2);
}

void lcdPut4Command(const struct lcdDataStruct *lcd, unsigned char command)
{
  register unsigned char myCommand = command;
  register unsigned char i;

  digitalWrite(gpio, lcd->rsPin, 0);

  for (i = 0; i < 4; ++i)
  {
    digitalWrite(gpio, lcd->dataPins[i], (myCommand & 1));
    myCommand >>= 1;
  }
  strobe(lcd);
}

/*
 * lcdHome: lcdClear:
 *	Home the cursor or clear the screen.
 *********************************************************************************
 */

void lcdHome(struct lcdDataStruct *lcd)
{
#ifdef DEBUG
  fprintf(stderr, "lcdHome: lcdPutCommand(%d,%d)\n", lcd, LCD_HOME);
#endif
  lcdPutCommand(lcd, LCD_HOME);
  lcd->cx = lcd->cy = 0;
  delay(5);
}

void lcdClear(struct lcdDataStruct *lcd)
{
#ifdef DEBUG
  fprintf(stderr, "lcdClear: lcdPutCommand(%d,%d) and lcdPutCommand(%d,%d)\n", lcd, LCD_CLEAR, lcd, LCD_HOME);
#endif
  lcdPutCommand(lcd, LCD_CLEAR);
  lcdPutCommand(lcd, LCD_HOME);
  lcd->cx = lcd->cy = 0;
  delay(5);
}

/*
 * lcdPosition:
 *	Update the position of the cursor on the display.
 *	Ignore invalid locations.
 *********************************************************************************
 */

void lcdPosition(struct lcdDataStruct *lcd, int x, int y)
{
  // struct lcdDataStruct *lcd = lcds [fd] ;

  if ((x > lcd->cols) || (x < 0))
    return;
  if ((y > lcd->rows) || (y < 0))
    return;

  lcdPutCommand(lcd, x + (LCD_DGRAM | (y > 0 ? 0x40 : 0x00) /* rowOff [y] */));

  lcd->cx = x;
  lcd->cy = y;
}

/*
 * lcdDisplay: lcdCursor: lcdCursorBlink:
 *	Turn the display, cursor, cursor blinking on/off
 *********************************************************************************
 */

void lcdDisplay(struct lcdDataStruct *lcd, int state)
{
  if (state)
    lcdControl |= LCD_DISPLAY_CTRL;
  else
    lcdControl &= ~LCD_DISPLAY_CTRL;

  lcdPutCommand(lcd, LCD_CTRL | lcdControl);
}

void lcdCursor(struct lcdDataStruct *lcd, int state)
{
  if (state)
    lcdControl |= LCD_CURSOR_CTRL;
  else
    lcdControl &= ~LCD_CURSOR_CTRL;

  lcdPutCommand(lcd, LCD_CTRL | lcdControl);
}

void lcdCursorBlink(struct lcdDataStruct *lcd, int state)
{
  if (state)
    lcdControl |= LCD_BLINK_CTRL;
  else
    lcdControl &= ~LCD_BLINK_CTRL;

  lcdPutCommand(lcd, LCD_CTRL | lcdControl);
}

/*
 * lcdPutchar:
 *	Send a data byte to be displayed on the display. We implement a very
 *	simple terminal here - with line wrapping, but no scrolling. Yet.
 *********************************************************************************
 */

void lcdPutchar(struct lcdDataStruct *lcd, unsigned char data)
{
  digitalWrite(gpio, lcd->rsPin, 1);
  sendDataCmd(lcd, data);

  if (++lcd->cx == lcd->cols)
  {
    lcd->cx = 0;
    if (++lcd->cy == lcd->rows)
      lcd->cy = 0;

    // TODO: inline computation of address and eliminate rowOff
    lcdPutCommand(lcd, lcd->cx + (LCD_DGRAM | (lcd->cy > 0 ? 0x40 : 0x00) /* rowOff [lcd->cy] */));
  }
}

/*
 * lcdPuts:
 *	Send a string to be displayed on the display
 *********************************************************************************
 */

void lcdPuts(struct lcdDataStruct *lcd, const char *string)
{
  while (*string)
    lcdPutchar(lcd, *string++);
}

/* ======================================================= */
/* SECTION: aux functions for game logic                   */
/* ------------------------------------------------------- */

/* ********************************************************** */
/* COMPLETE the code for all of the functions in this SECTION */
/* Implement these as C functions in this file                */
/* ********************************************************** */

/* --------------------------------------------------------------------------- */
/* interface on top of the low-level pin I/O code */

/* blink the led on pin @led@, @c@ times */
// Modified by AJ
void blinkN(uint32_t *gpio, int led, int c)
{
  /* ***  COMPLETE the code here  ***  */
  for (int i = 0; i < c; i++)
  {
    digitalWrite(gpio, led, ON);
    delay(500);
    digitalWrite(gpio, led, OFF);
    delay(500);
  }
}

/* ======================================================= */
/* SECTION: main fct                                       */
/* ------------------------------------------------------- */

int main(int argc, char *argv[])
{ // this is just a suggestion of some variable that you may want to use
  struct lcdDataStruct *lcd;
  int bits, rows, cols;
  unsigned char func;

  int found = 0, attempts = 0, i, j, code;
  int c, d, buttonPressed, rel, foo;
  int *attSeq;

  int greenLED = GREEN_LED, redLED = RED_LED, pinButton = BUTTON;
  int fSel, shift, pin, clrOff, setOff, off, res;
  int fd;

  int exact, contained;
  char str1[32];
  char str2[32];

  struct timeval t1, t2;
  int t;

  char buf[32];

  // variables for command-line processing
  char str_in[20], str[20] = "some text";
  int verbose = 0, debug = 0, help = 0, opt_m = 0, opt_n = 0, opt_s = 0, unit_test = 0, res_matches = 0;

  // -------------------------------------------------------
  // process command-line arguments

  // see: man 3 getopt for docu and an example of command line parsing
  { // see the CW spec for the intended meaning of these options
    int opt;
    while ((opt = getopt(argc, argv, "hvdus:")) != -1)
    {
      switch (opt)
      {
      case 'v':
        verbose = 1;
        break;
      case 'h':
        help = 1;
        break;
      case 'd':
        debug = 1;
        break;
      case 'u':
        unit_test = 1;
        break;
      case 's':
        opt_s = atoi(optarg);
        break;
      default: /* '?' */
        fprintf(stderr, "Usage: %s [-h] [-v] [-d] [-u <seq1> <seq2>] [-s <secret seq>]  \n", argv[0]);
        exit(EXIT_FAILURE);
      }
    }
  }

  if (help)
  {
    fprintf(stderr, "MasterMind program, running on a Raspberry Pi, with connected LED, button and LCD display\n");
    fprintf(stderr, "Use the button for input of numbers. The LCD display will show the matches with the secret sequence.\n");
    fprintf(stderr, "For full specification of the program see: https://www.macs.hw.ac.uk/~hwloidl/Courses/F28HS/F28HS_CW2_2022.pdf\n");
    fprintf(stderr, "Usage: %s [-h] [-v] [-d] [-u <seq1> <seq2>] [-s <secret seq>]  \n", argv[0]);
    exit(EXIT_SUCCESS);
  }

  if (unit_test && optind >= argc - 1)
  {
    fprintf(stderr, "Expected 2 arguments after option -u\n");
    exit(EXIT_FAILURE);
  }

  if (verbose && unit_test)
  {
    printf("1st argument = %s\n", argv[optind]);
    printf("2nd argument = %s\n", argv[optind + 1]);
  }

  if (verbose)
  {
    fprintf(stdout, "Settings for running the program\n");
    fprintf(stdout, "Verbose is %s\n", (verbose ? "ON" : "OFF"));
    fprintf(stdout, "Debug is %s\n", (debug ? "ON" : "OFF"));
    fprintf(stdout, "Unittest is %s\n", (unit_test ? "ON" : "OFF"));
    if (opt_s)
      fprintf(stdout, "Secret sequence set to %d\n", opt_s);
  }

  seq1 = (int *)malloc(seqlen * sizeof(int));
  seq2 = (int *)malloc(seqlen * sizeof(int));
  cpy1 = (int *)malloc(seqlen * sizeof(int));
  cpy2 = (int *)malloc(seqlen * sizeof(int));

  // check for -u option, and if so run a unit test on the matching function
  if (unit_test && argc > optind + 1)
  { // more arguments to process; only needed with -u
    strcpy(str_in, argv[optind]);
    opt_m = atoi(str_in);
    strcpy(str_in, argv[optind + 1]);
    opt_n = atoi(str_in);
    // CALL a test-matches function; see testm.c for an example implementation
    readSeq(seq1, opt_m); // turn the integer number into a sequence of numbers
    readSeq(seq2, opt_n); // turn the integer number into a sequence of numbers
    if (verbose)
      fprintf(stdout, "Testing matches function with sequences %d and %d\n", opt_m, opt_n);
    res_matches = countMatches(seq1, seq2);
    showMatches(res_matches, seq1, seq2, 1);
    exit(EXIT_SUCCESS);
  }
  else
  {
    /* nothing to do here; just continue with the rest of the main fct */
  }

  if (opt_s)
  { // if -s option is given, use the sequence as secret sequence
    if (theSeq == NULL)
      theSeq = (int *)malloc(seqlen * sizeof(int));
    readSeq(theSeq, opt_s);
    if (verbose)
    {
      fprintf(stderr, "Running program with secret sequence:\n");
      showSeq(theSeq);
    }
  }

  // -------------------------------------------------------
  // LCD constants, hard-coded: 16x2 display, using a 4-bit connection
  bits = 4;
  cols = 16;
  rows = 2;
  // -------------------------------------------------------

  printf("Raspberry Pi LCD driver, for a %dx%d display (%d-bit wiring) \n", cols, rows, bits);

  if (geteuid() != 0)
    fprintf(stderr, "setup: Must be root. (Did you forget sudo?)\n");

  // init of guess sequence, and copies (for use in countMatches)
  attSeq = (int *)malloc(seqlen * sizeof(int));
  cpy1 = (int *)malloc(seqlen * sizeof(int));
  cpy2 = (int *)malloc(seqlen * sizeof(int));

  // -----------------------------------------------------------------------------
  // constants for RPi2
  gpiobase = 0x3F200000;

  // const uint32_t RPI3_GPIO_BASE = 0x3F200000; // added for fact checking

  // -----------------------------------------------------------------------------
  // memory mapping
  // Open the master /dev/memory device

  if ((fd = open("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC)) < 0)
    return failure(FALSE, "setup: Unable to open /dev/mem: %s\n", strerror(errno));

  // GPIO:
  gpio = (uint32_t *)mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, gpiobase);
  if ((int32_t)gpio == -1)
    return failure(FALSE, "setup: mmap (GPIO) failed: %s\n", strerror(errno));

  // -------------------------------------------------------
  // Configuration of LED and BUTTON
  // Modified by AJ
  /* ***  COMPLETE the code here  ***  */
  pinMode(gpio, greenLED, OUTPUT);
  pinMode(gpio, redLED, OUTPUT);
  pinMode(gpio, pinButton, INPUT);
  pinMode(gpio, STRB_PIN, OUTPUT);
  pinMode(gpio, RS_PIN, OUTPUT);
  pinMode(gpio, DATA0_PIN, OUTPUT);
  pinMode(gpio, DATA1_PIN, OUTPUT);
  pinMode(gpio, DATA2_PIN, OUTPUT);
  pinMode(gpio, DATA3_PIN, OUTPUT);

  // -------------------------------------------------------
  // INLINED version of lcdInit (can only deal with one LCD attached to the RPi):
  // you can use this code as-is, but you need to implement digitalWrite() and
  // pinMode() which are called from this code
  // Create a new LCD:
  lcd = (struct lcdDataStruct *)malloc(sizeof(struct lcdDataStruct));
  if (lcd == NULL)
    return -1;

  // hard-wired GPIO pins
  lcd->rsPin = RS_PIN;
  lcd->strbPin = STRB_PIN;
  lcd->bits = 4;
  lcd->rows = rows; // # of rows on the display
  lcd->cols = cols; // # of cols on the display
  lcd->cx = 0;      // x-pos of cursor
  lcd->cy = 0;      // y-pos of curosr

  lcd->dataPins[0] = DATA0_PIN;
  lcd->dataPins[1] = DATA1_PIN;
  lcd->dataPins[2] = DATA2_PIN;
  lcd->dataPins[3] = DATA3_PIN;
  // lcd->dataPins [4] = d4 ;
  // lcd->dataPins [5] = d5 ;
  // lcd->dataPins [6] = d6 ;
  // lcd->dataPins [7] = d7 ;

  // lcds [lcdFd] = lcd ;

  digitalWrite(gpio, lcd->rsPin, 0);
  pinMode(gpio, lcd->rsPin, OUTPUT);
  digitalWrite(gpio, lcd->strbPin, 0);
  pinMode(gpio, lcd->strbPin, OUTPUT);

  for (i = 0; i < bits; ++i)
  {
    digitalWrite(gpio, lcd->dataPins[i], 0);
    pinMode(gpio, lcd->dataPins[i], OUTPUT);
  }
  delay(35); // mS

  // Gordon Henderson's explanation of this part of the init code (from wiringPi):
  // 4-bit mode?
  //	OK. This is a PIG and it's not at all obvious from the documentation I had,
  //	so I guess some others have worked through either with better documentation
  //	or more trial and error... Anyway here goes:
  //
  //	It seems that the controller needs to see the FUNC command at least 3 times
  //	consecutively - in 8-bit mode. If you're only using 8-bit mode, then it appears
  //	that you can get away with one func-set, however I'd not rely on it...
  //
  //	So to set 4-bit mode, you need to send the commands one nibble at a time,
  //	the same three times, but send the command to set it into 8-bit mode those
  //	three times, then send a final 4th command to set it into 4-bit mode, and only
  //	then can you flip the switch for the rest of the library to work in 4-bit
  //	mode which sends the commands as 2 x 4-bit values.

  if (bits == 4)
  {
    func = LCD_FUNC | LCD_FUNC_DL; // Set 8-bit mode 3 times
    lcdPut4Command(lcd, func >> 4);
    delay(35);
    lcdPut4Command(lcd, func >> 4);
    delay(35);
    lcdPut4Command(lcd, func >> 4);
    delay(35);
    func = LCD_FUNC; // 4th set: 4-bit mode
    lcdPut4Command(lcd, func >> 4);
    delay(35);
    lcd->bits = 4;
  }
  else
  {
    failure(TRUE, "setup: only 4-bit connection supported\n");
    func = LCD_FUNC | LCD_FUNC_DL;
    lcdPutCommand(lcd, func);
    delay(35);
    lcdPutCommand(lcd, func);
    delay(35);
    lcdPutCommand(lcd, func);
    delay(35);
  }

  if (lcd->rows > 1)
  {
    func |= LCD_FUNC_N;
    lcdPutCommand(lcd, func);
    delay(35);
  }

  // Rest of the initialisation sequence
  lcdDisplay(lcd, TRUE);
  lcdCursor(lcd, FALSE);
  lcdCursorBlink(lcd, FALSE);
  lcdClear(lcd);

  lcdPutCommand(lcd, LCD_ENTRY | LCD_ENTRY_ID);     // set entry mode to increment address counter after write
  lcdPutCommand(lcd, LCD_CDSHIFT | LCD_CDSHIFT_RL); // set display shift to right-to-left

  // END lcdInit ------
  // -----------------------------------------------------------------------------
  // Start of game
  fprintf(stderr, "Printing welcome message on the LCD display ...\n");

  /*-------------------------------------------------------------------------------------*/
  /* ***  COMPLETE the code here  ***  */
  lcdPuts(lcd, "Welcome!");
  delay(2000);
  lcdClear(lcd);

  /*-------------------------------------------------------------------------------------*/

  /* initialise the secret sequence */
  if (!opt_s)
    inititalizeSeq();
  if (1)
    showSeq(theSeq);

  // optionally one of these 2 calls:
  // waitForEnter();
  // waitForButton(gpio, pinButton);

  // -----------------------------------------------------------------------------

  /* ******************************************************* */
  /* ***  COMPLETE the code here  ***                        */
  /* this needs to implement the main loop of the game:      */
  /* check for button presses and count them                 */
  /* store the input numbers in the sequence @attSeq@        */
  /* compute the match with the secret sequence, and         */
  /* show the result                                         */
  /* see CW spec for details                                 */
  /* ******************************************************* */
  // +++++ main loop

  // Modified by AJ & Leressa

  // Turn LED off if was on previous game
  digitalWrite(gpio, greenLED, OFF);
  digitalWrite(gpio, redLED, OFF);

  while (!found && attempts < 10)
  {
    lcdClear(lcd);

    int turn = 0;

    printf("Round %d!!!\n", attempts += 1);

    // prints the round number on the lcd
    sprintf(buf, "Round: %d", attempts);
    lcdPosition(lcd, 0, 0);
    lcdPuts(lcd, buf);
    

    while (1)
    {
      printf("Turn: %d\n", turn += 1);
      delay(3000);
      printf("Enter a sequence of %d numbers\n", SEQL);
      // Time window of 7 seconds
      time_t startTime = time(NULL);
      time_t endTime = startTime + 5;

      // Count of button presses
      int buttonPressCount = 0;

      // Blink red when time window ends
      while (time(NULL) < endTime)
      {
        // Wait for the button to be pressed
        if (waitForButton(gpio, pinButton) == 1)
        {
          buttonPressCount++;
          delay(500);
        }
        if (buttonPressCount >= 3)
        {
          buttonPressCount = 3;
          break;
        }
      }
      printf("Button pressed %d times\n", buttonPressCount);

      // Blink red LED indicating the end of the time window
      digitalWrite(gpio, redLED, ON);
      delay(2000);
      digitalWrite(gpio, redLED, OFF);

      // Blink the number of times the button was pressed on green
      blinkN(gpio, greenLED, buttonPressCount);

      // Store the number of button presses in attSeq
      attSeq[turn - 1] = buttonPressCount;
      // Repeat for a sequence of 3
      if (turn <= 3)
      {
        // Delay before starting the next attempt
        delay(1500);
      }
      if (turn == 3)
      {
        printf("%d\n", attSeq[0]);
        printf("%d\n", attSeq[1]);
        printf("%d\n", attSeq[2]);

        blinkN(gpio, redLED, 2);
        break;
      }
    }

    // Compare the sequence with the secret sequence
    code = countMatches(theSeq, attSeq);

    int exact = code >> 4; // Shift right by 4 bits to get the 'exact' value
    int approximate = code & 0xF; // Bitwise AND with 0xF (which is 15 in decimal or 1111 in binary) to get the 'approximate' value

    printf("Exact: %d\n", exact);
   
    printf("Approximate: %d\n", approximate);

    delay(1000);


    if (exact == 3) {
      digitalWrite(gpio, redLED, ON);
    }
    
    // prints exact on the lcd
    lcdClear(lcd);
    blinkN(gpio, greenLED, exact);
    sprintf(buf, "Exact: %d", exact);
    lcdPosition(lcd, 0, 1);
    lcdPuts(lcd, buf);

    if (exact == 3) {
      digitalWrite(gpio, redLED, OFF);
    }
    else {
      // separator
      blinkN(gpio, redLED, 1);
    }

    // prints approximate on the lcd
    lcdClear(lcd);
    blinkN(gpio, greenLED, approximate);
    sprintf(buf, "Approx: %d", approximate);
    lcdPosition(lcd, 0, 1);
    lcdPuts(lcd, buf);


    if (exact == 3)
    {
      found = 1;
      break;
    }
    else {
      // Clear the sequence
      for (int i = 0; i < SEQL; i++)
      {
        attSeq[i] = 0;
      }
    }

    delay(1000);
    blinkN(gpio, redLED, 3);

    delay(500);
    printf("Starting next round\n");
    delay(2000);

  }

  if (found)
  {
    /* ***  COMPLETE the code here  ***  */
    fprintf(stdout, "Sequence found\n");
    lcdClear(lcd);
    lcdPuts(lcd, "SUCCESS!");
    delay(2000);
    // prints the number of attempts done on the lcd
    
    sprintf(buf, "Attempts: %d", attempts);
    lcdPosition(lcd, 0, 0);
    lcdPuts(lcd, buf);
    delay(10000);
    lcdClear(lcd);

  }
  else
  {
    fprintf(stdout, "Sequence not found\n");
  }
  return 0;
}
