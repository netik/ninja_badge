// I need strlen, memset, memcpy, and rand
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <tchar.h>
#include <conio.h>
#include "badge.h"

static byte screen[HEIGHT][WIDTH];

// fake screen for now
byte* text_get_screen() 
{ 
	return &screen[3][0]; 
}

// redraw
byte text_draw_full()
{
	system("cls");
	for( int i = 0; i < HEIGHT; i++ )
	{
		for( int j = 0; j < WIDTH; j++ )
		{
			printf( "%c", screen[i][j] );
		}
		printf( "\n" );
	}
	return 0;
}

byte text_update_line( unsigned y )
{
	return text_draw_full();
}

byte text_scroll_vert( int nlines )
{
	if( nlines >= HEIGHT )
		nlines = HEIGHT - 1;
	int keep = nlines * WIDTH;
	int clear = sizeof(screen)-keep;

	memmove( screen[0], screen[nlines], keep );
	memset( screen[HEIGHT-nlines], ' ', clear );
	return 0;
}

// win
void grant_CC_item()
{
	printf( "VICTORY!!!" );
	getchar();
}

// input
unsigned next_button_event(unsigned usMax)
{
	// unused
	usMax |= 0;

	char ch = _getch();
	switch( ch )
	{
	case '8': return BUTT_UP;
	case '6': return BUTT_RIGHT;
	case '4': return BUTT_LEFT;
	case '2': return BUTT_DOWN;
	case 'a': return BUTT_A;
	case 'b': return BUTT_B;
	}
	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	CC_main_loop(&screen[0][0]);
}

