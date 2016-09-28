// shit I think the badge code provides me

typedef unsigned char byte;

#define HEIGHT 8
#define WIDTH  25

byte* text_get_screen(void);

// redraw
byte text_draw_full(void);
byte text_update_line(void);
byte text_scroll_vert( int nlines );

// win
void grant_CC_item(void);


// run
unsigned int CC_main_loop(void);
