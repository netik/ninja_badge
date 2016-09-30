typedef unsigned char uint8_t;

#include <stdio.h>
#include <string.h>
#include "code/blobs/ext/boot_up_screen.h"
#include "code/blobs/ext/splash_screen1.h"
#include "code/blobs/ext/credits_woz.h"
#include "code/tests/shuriken.h"
#include "code/blobs/base/boss_skull.h"

uint8_t display_fbuf[1024];      // frame buffer
#define LINE_NUM

void display_init()
{
  memset(display_fbuf, 0, sizeof(display_fbuf));
}

void erase_bottom(void) {
    for(int i = 0; i < 128; i++) {
      /* what does this tell us about the screen?
       * 
       * 0x07 = 0000 0111 = so on the row just above the bottom of the
       * screen we are painting three pixels dark
       *
       * So this means that MSB is UP, LSB is DOWN
       *
       * and that we are filling 8 pixels at a time per 'page'.  They
       * are using a mask on row 6, because I think there was a line
       * below all of the characters.
       *
       * We also fill row 7 with black (0xfF)
       */
      display_fbuf[128*6+i] |= 0x07;
      display_fbuf[128*7+i] = 0xff;
    }
}

void dump_xybuf(char xybuf[])
{
  char display[1024];
  bzero(display,1024);
  

  for (int y = 0; y < 7; y++) {  // the byte we are working on 
    for (int x = 0; x < 128; x++) {
     
    }
  }
  
  // now dump the raster. 
  for (int y = 0; y < 64; y++) { 
    for (int x = 0; x < 15; x++) {
      for (unsigned char i = 0; i < 8 ; i++) {
	if ((display[(15*y)+x] >> i) & 1) {
	  printf("*");
	} else {
	  printf(".");
	}
	}
    }
    printf("\n");
  }
}
void dump_buf(char xybuf[])
{
  unsigned char *c;
  int x,q,ln;
  ln=0;
  q=0;

  int start=0;

#ifdef LINE_NUM
  printf("%04d: ", start);
#endif
  for (x=0; x<1024; x++) {
    for (unsigned char i = 0; i <8 ; i++) {
      if ((xybuf[x] >> i) & 1) {
	printf("*");
      } else {
	printf(".");
      }
      q += 1;
    }

    if ( q >= 127 ) {
      printf("\n");

#ifdef LINE_NUM
      printf("%04d: ", x);
#endif
      q=0;
      ln++;
    }
  }

  printf("lines: %d\n", ln);
}

void chartobin(char c) { 
    for (unsigned char i = 8; i >0 ; i--) {
      if ((c >> i) & 1) {
	printf("*");
      } else {
	printf(".");
      }
    }
}

int main() {
  display_init();
  // display_copy((char *)boot_up_screen, 0, 0, 7, 0, 113, 27);
  //  display_copy((char *)buttonb, 0, 0, 79, 55, 7, 7);
  //display_copy((char *)splash_screen1+2, 0, 0, 0, 0, 128, 64);
  //  dump_display();
  erase_bottom();

  dump_buf((char *)display_fbuf);
  dump_xybuf((char *)display_fbuf);
}
