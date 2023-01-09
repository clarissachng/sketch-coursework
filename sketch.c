// Basic program skeleton for a Sketch File (.sk) Viewer
#include "displayfull.h"
#include "sketch.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Allocate memory for a drawing state and initialise it
state *newState() {
  state *s = malloc(sizeof(state));
  *s = (state) {0, 0, 0, 0, 1, 0, 0, 0};
  return s;
}

// Release all memory associated with the drawing state
void freeState(state *s) {
  free (s);
}

// Extract an opcode from a byte (two most significant bits).
int getOpcode(byte b) {
  // get the last 2 bits == 000000xx
  int newOpcode = (b >> 6);
  return newOpcode;
}

// Extract an operand (-32..31) from the rightmost 6 bits of a byte.
int getOperand(byte b) {
  // masking with 00111111 -> get first 2 bits
  int newOperand = b & 0x3F;

  // masking to 10000000, 01111111, logic AND them together
  int result = -(newOperand & 32) + (newOperand & 31);
  return result;
}

// Execute the next byte of the command sequence.
void obey(display *d, state *s, byte op) {
  int opcode = getOpcode(op);
  int operand = getOperand(op);

  // if opcode calls tool --> update respective tool 
  // except for targetx, targety, show, pause and nextframe
  if (opcode == TOOL) {
     switch (operand) {
      case NONE: s -> tool = NONE; break;
      case LINE: s -> tool = LINE; break;
      case COLOUR: 
        s -> tool = COLOUR; 
        colour(d, s -> data);
        break;
      case BLOCK: s -> tool = BLOCK; break;
      case TARGETX: s -> tx = s -> data; break;
      case TARGETY: s -> ty = s -> data; break;
      case SHOW: show(d); break;
      case PAUSE: pause(d, s -> data); break;
      case NEXTFRAME: s -> end = true; break;
    }
    // reset the tool after using the tool
    s -> data = 0;
  }
  
  // if opcode doesnt call tool
  else if (opcode == DX) {
    s -> tx += operand;
  }
  
  else if (opcode == DY) {
    s -> ty += operand;

    // if tool is enabled, get tool to draw to the targeted coordinate
    switch (s -> tool) {
      case LINE: 
      line(d, s -> x, s -> y, s -> tx, s -> ty); 
      break;
      case BLOCK: 
      block(d, s -> x, s -> y, (s -> tx) - (s -> x), (s -> ty) - (s -> y)); 
      break;
    }
    // update current to target
    s -> x = s -> tx;
    s -> y = s -> ty;
  }

  // if opcode calls data
  else if (opcode == DATA) {
    // shift 6 position to the left
    // replaces the 6 empty bits with right most operand bits
    // example:
    // data << 6 = 0000 0011 -> 1100 0000
    // operand = 0010 1011 & 0011 1111 -> 0011 1111
    // OR = 1111 1111
    s -> data = (s -> data << 6) | (operand & 0x3F);
  }
}

// Draw a frame of the sketch file. For basic and intermediate sketch files
// this means drawing the full sketch whenever this function is called.
// For advanced sketch files this means drawing the current frame whenever
// this function is called.
bool processSketch(display *d, const char pressedKey, void *data) {
  // check data has been initialised
  if (data == NULL) return (pressedKey == 27);

  // access drawing state
  state *s = (state*) data;

  // get filename
  char *filename = getName(d);

  // read out the file and then draw the file
  // lines 112 to 115 are from lecture 19: I/O
  FILE *file = fopen(filename, "rb"); // open the file
  fseek(file, 0, SEEK_END); // moving the pointer to the end of the file 
  long len = ftell(file); // get the length of the file
  fseek(file, 0, SEEK_SET); // goes back to the beginning of the file 

  char *store = malloc(len * sizeof(char)); // store the read data
  fread(store, len, 1, file); // get the content out 
  fclose(file); // close the read file

  // define a new variable for starting coordinate
  // store s -> start  to variable when calling NEXTFRAME (ie: s -> end == true)
  int start = 0;
  if (s -> end == true) {
    start = s -> start;
  }

  // reset s -> end
  s -> end = false;

  // use for loop -> read file (byte by byte) -> put it in store variable
  // so we can use it on obey function as byte

  // read the command byte from wherever the drawing starts 
  for(int i = start; i < len; i++) {
    obey(d,s,store[i]);

    // when NEXTFRAME is called, (ie: when s -> end == true) 
    // starting coordinate will be the + 1 of the ending coordinate 
    //  ie: doesnt start from 0

    if (s -> end == true) {
      s -> start = i + 1;
      break;
    }
  }

  // finished reading the command byte
  free(store);

  // use show to display
  show(d);

  // reset drawing state (except for start, end and data)
  s -> x = s -> y = s -> tx = s -> ty = 0;
  s -> tool = LINE;

  return pressedKey;
}

// View a sketch file in a 200x200 pixel window given the filename
void view(char *filename) {
  display *d = newDisplay(filename, 200, 200);
  state *s = newState();
  run(d, s, processSketch);
  freeState(s);
  freeDisplay(d);
}

// Include a main function only if we are not testing (make sketch),
// otherwise use the main function of the test.c file (make test).
#ifndef TESTING
int main(int n, char *args[n]) {
  if (n != 2) { // return usage hint if not exactly one argument
    printf("Use ./sketch file\n");
    exit(1);
  } else view(args[1]); // otherwise view sketch file in argument
  return 0;
}
#endif
