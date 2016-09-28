// Cryptix.cpp : Defines the entry point for the console application.
//
#include <mc1322x.h>
#include <board.h>
#include <ext.h>
#include <syscalls.h>
#include <pinio.h>
#include <pinout.h>
#include <buzzer.h>
#include "config.h"
#include <rfio.h>
#include <globals.h>
#include <misc.h>
#include <display.h>
#include "put.h"
#include <lock.h>
#include <debug.h>
#include <misc.h>
#include "rubix.h"

int stop;
int fail;
int round;
int cylinder;
typedef
struct puzzle {
	const char* hint;
	const char* answer;
} puzzle;
puzzle puzzles[] = {
	{ "CAEZARS", "FKDOOHQJH" },
	{ "012345678", "DEFGHIJKL" },
	{ "10000111", "65556555" },
	{ "PASSWORD", "VGYY2UXJ" },
	{ "112358132", "8ABCCFG8BB" },
	{ "4841434B", "PIKS" },
	{ "CURQ ROT", "YQNM" },
	{ "NOP SLED", "JAJAJAJAJA" },
	{ "3OPGETEIP", "UX0NLWW0Z0" },
};

#define ELEMENTS (26+10)
#define PUZZLES  ((int)(sizeof(puzzles)/sizeof(puzzle)))
#define EMPTY (0xff)

char getLetter( unsigned char symbol )
{
	if( symbol >= 36 )
		return ' ';
	else if( symbol < 10 )
		return '0' + symbol;
	else 
		return 'A' + (symbol - 10);
}


void resetCylinders(unsigned char *cylinders)
{
	int len = strlen(puzzles[round].answer);
	int pad = (WIDTH - len)/2;

	memset( cylinders, EMPTY, WIDTH );
	for( int i = 0; i < len; i++ )
	{
		cylinders[pad+i] =(((unsigned int)rand()) % 36);
	}

	cylinder = pad;
}

int matchesAnswer(int rnd, unsigned char *cylinders)
{
	const char* answer = puzzles[rnd].answer;
	int pad = (WIDTH - strlen(answer))/2;

	for( unsigned i = 0; i < strlen(answer); i++ )
	{
		int offset = (cylinders[i+pad] + (rnd + 3)) % ELEMENTS;
		if( getLetter(offset) != answer[i] )
			return 0;
	}
	return 1;
}

void computeState(char* line1, char* line2, char* line3, unsigned char *cylinders)
{
	// show hint
	const char* string = puzzles[round].hint;
	int len = strlen(string);
	int pad = (WIDTH - len)/2;
	memcpy( line1 + pad, (char *)string, len );

	// show answer
	pad = (WIDTH - strlen(puzzles[round].answer))/2;
	for( int i = 0; i < WIDTH; i++ )
	{
		line2[i] = getLetter(cylinders[i]);
	}

	// show cursor
	line3[cylinder] = '*';
}
void redraw_screen(unsigned char *screen) {
    display_clear();
    for(int i = 0; i < HEIGHT; i++) {
        smallfont_write((char *)&screen[i*WIDTH], WIDTH, 1, (i * 7));
    }
    display_refresh();
}


unsigned int CC_main_loop(void)
{
    stop = 0;
    fail = 1;
    round = 0;
    cylinder = 0;
    unsigned char screen[HEIGHT*WIDTH];

    // all the state I need
    unsigned char cylinders[WIDTH];


    memset(screen, ' ', sizeof(screen));
	char* line1 = (char*)(screen + 3*WIDTH);
	char* line2 = (char*)(screen + 4*WIDTH);
	char* line3 = (char*)(screen + 5*WIDTH);

	// starts the answer randomly
	resetCylinders(cylinders);
    redraw_screen(screen);

    unsigned int retval = RET_BUTTON;
	while(!stop)
	{
        if(retval != RET_TIMEOUT) {
            computeState( line1, line2, line3, cylinders );
            redraw_screen(screen);
        }
        retval = chill(
                CHILL_TIMEOUT | CHILL_BUTTON,
                500000,
                NULL);
        line3[cylinder] = ' ';
        if(retval == RET_BUTTON) {
            if( buttons_down & (1 << BUTTON_UP) )
            {
                if( cylinders[cylinder] == 0 )
                    cylinders[cylinder] = ELEMENTS-1;
                else
                    cylinders[cylinder]--; 
            }
            else if( buttons_down & (1 << BUTTON_DOWN ))
            {
                if( cylinders[cylinder] == ELEMENTS-1 )
                    cylinders[cylinder] = 0;
                else
                    cylinders[cylinder]++; 
            }
            else if( buttons_down & (1 << BUTTON_LEFT))
            {
                if( cylinder > 0 && cylinders[cylinder-1] != EMPTY )
                    cylinder--;
            }
            else if( buttons_down & (1 << BUTTON_RIGHT))
            {
                if( cylinder < WIDTH-1 && cylinders[cylinder+1] != EMPTY )
                    cylinder++;
            }
            else if( buttons_down & (1 << BUTTON_A))
            {
                if( matchesAnswer(round, cylinders) )
                    round++;
                if( round >= PUZZLES )
                {
                    fail = 0;
                    stop = 1;
                }
                else
                {
                    resetCylinders(cylinders);
                }
                redraw_screen(screen);
            }
            else if( buttons_down & (1 << BUTTON_B))
            {
                break;
            }
        }
	}

    if(!fail) {
        globals.togrant = 7;
        return SCREEN_GRANT;
    }
    return SCREEN_MAIN;
}

