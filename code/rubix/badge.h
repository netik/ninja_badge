// shit I think the badge code provides me

#include <memory.h>
#include <string.h>
#include <stdlib.h>

typedef unsigned char byte;

#define HEIGHT 8
#define WIDTH  25

#define BUTT_UP    0x01
#define BUTT_DOWN  0x02
#define BUTT_LEFT  0x04
#define BUTT_RIGHT 0x08
#define BUTT_A     0x10 
#define BUTT_B     0x20

byte* text_get_screen();

// redraw
byte text_draw_full();
byte text_update_line( unsigned y );
byte text_scroll_vert( int nlines );

// win
void grant_CC_item();

// input
unsigned next_button_event(unsigned usMax);

// run
void CC_main_loop( byte* screen );