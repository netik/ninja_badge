// Cryptix.cpp : Defines the entry point for the console application.
//
#include "badge.h"

// all the state I need
int stop = 0;
int fail = 1;
int round = 0;
int cylinder = 0;
unsigned char cylinders[WIDTH];

#include "puzzles.h"

#define ELEMENTS (26+10)
#define PUZZLES  ((int)(sizeof(puzzles)/sizeof(puzzle)))
#define EMPTY (byte)(-1)

char getLetter( unsigned symbol )
{
	if( symbol == -1 || symbol >= 36 )
		return ' ';
	else if( symbol < 10 )
		return '0' + symbol;
	else 
		return 'A' + (symbol - 10);
}


void resetCylinders()
{
	int len = strlen(puzzles[round].answer);
	int pad = (WIDTH - len)/2;

	memset( cylinders, EMPTY, WIDTH );
	for( int i = 0; i < len; i++ )
	{
		cylinders[pad+i] = rand() % ELEMENTS;
	}

	cylinder = pad;
}

bool matchesAnswer(int round)
{
	const char* answer = puzzles[round].answer;
	int pad = (WIDTH - strlen(answer))/2;

	for( unsigned i = 0; i < strlen(answer); i++ )
	{
		int offset = (cylinders[i+pad] + (round + 3)) % ELEMENTS;
		if( getLetter(offset) != answer[i] )
			return false;
	}
	return true;
}

void computeState(char* line1, char* line2, char* line3)
{
	memset(line1, ' ', WIDTH);
	memset(line2, ' ', WIDTH);
	memset(line3, ' ', WIDTH);

	// show hint
	const char* string = puzzles[round].hint;
	int len = strlen(string);
	int pad = (WIDTH - len)/2;
	memcpy( line1 + pad, string, len );

	// show answer
	pad = (WIDTH - strlen(puzzles[round].answer))/2;
	for( int i = 0; i < WIDTH; i++ )
	{
		line2[i] = getLetter(cylinders[i]);
	}

	// show cursor
	line3[cylinder] = '^';
}

void processInput( byte move )
{
	if( move & BUTT_UP )
	{
		if( cylinders[cylinder] == 0 )
			cylinders[cylinder] = ELEMENTS-1;
		else
			cylinders[cylinder]--; 
	}
	else if( move & BUTT_DOWN )
	{
		if( cylinders[cylinder] == ELEMENTS-1 )
			cylinders[cylinder] = 0;
		else
			cylinders[cylinder]++; 
	}
	else if( move & BUTT_LEFT )
	{
		if( cylinder > 0 && cylinders[cylinder-1] != EMPTY )
			cylinder--;
	}
	else if( move & BUTT_RIGHT )
	{
		if( cylinder < WIDTH-1 && cylinders[cylinder+1] != EMPTY )
			cylinder++;
	}
	else if( move & BUTT_A )
	{
		if( matchesAnswer(round) )
			round++;
		if( round >= PUZZLES )
		{
			fail = 0;
			stop = 1;
		}
		else
		{
			resetCylinders();
		}
		text_draw_full();
	}
	else if( move & BUTT_B )
	{
		stop = 1;
	}
}

void CC_main_loop( byte* screen )
{
	char* line1 = (char*)(screen + 3*WIDTH);
	char* line2 = (char*)(screen + 4*WIDTH);
	char* line3 = (char*)(screen + 5*WIDTH);

	// draw the first challenge text
	text_update_line(3);

	// starts the answer randomly
	resetCylinders();
	text_update_line(5);

	while(!stop)
	{
		computeState( line1, line2, line3 );
		text_update_line(4);
		processInput( (byte)next_button_event(-1) );
	}

	if( !fail )
		grant_CC_item();
}

