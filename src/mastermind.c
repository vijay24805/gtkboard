/*  This file is a part of gtkboard, a board games system.
    Copyright (C) 2003, Arvind Narayanan <arvindn@users.sourceforge.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111 USA

*/

/** \file mastermind.c */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>

#include "game.h"
#include "aaball.h"
#include "../pixmaps/misc.xpm"

#define MASTERMIND_CELL_SIZE 40
#define MASTERMIND_NUM_PIECES 26

#define MASTERMIND_BOARD_WID 8
#define MASTERMIND_BOARD_HEIT 11

#define MASTERMIND_EMPTY 0
#define MASTERMIND_MAIN_COL_START 2
#define MASTERMIND_MAIN_COL_END 5

#define MASTERMIND_GET_SELECTION(x,y) (((x)==7&&(y)>1&&(y)<8)?(y)-1:-1)
#define MASTERMIND_IS_MAIN_COL(x) ((x)>1&&(x)<6)

char mastermind_colors[9] = {200, 200, 200, 200, 200, 200, 0, 0, 0};

int mastermind_initpos [MASTERMIND_BOARD_WID*MASTERMIND_BOARD_HEIT] = 
{
	0, 0, 9, 9, 9, 9, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 6,
	0, 0, 0, 0, 0, 0, 0, 5,
	0, 0, 0, 0, 0, 0, 0, 4,
	0, 0, 0, 0, 0, 0, 0, 3,
	0, 0, 0, 0, 0, 0, 0, 2,
	0, 0, 0, 0, 0, 0, 0, 1,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
};


SCORE_FIELD mastermind_score_fields[] = {SCORE_FIELD_USER, SCORE_FIELD_SCORE, SCORE_FIELD_TIME, SCORE_FIELD_DATE, SCORE_FIELD_NONE};
char *mastermind_score_field_names[] = {"User", "Tries", "Time", "Date", NULL};

void mastermind_init ();
char ** mastermind_get_pixmap (int idx, int color);

Game Mastermind = { MASTERMIND_CELL_SIZE, 
	MASTERMIND_BOARD_WID, MASTERMIND_BOARD_HEIT, 
	MASTERMIND_NUM_PIECES,	mastermind_colors,  
	NULL, 	NULL, "Mastermind", mastermind_init};


static ResultType mastermind_who_won (Pos *, Player, char **);
static void mastermind_setinitpos (Pos *pos);
int mastermind_getmove (Pos *, int, int, GtkboardEventType, Player, byte**);
int mastermind_getmove_kb (Pos *, int , Player, byte **);
void mastermind_reset_uistate ();
byte * mastermind_movegen (Pos *, int);
float mastermind_eval (Pos *, int);
int mastermind_get_cur_row (byte *);


void mastermind_init ()
{
	game_getmove = mastermind_getmove;
	game_getmove_kb = mastermind_getmove_kb;
	game_who_won = mastermind_who_won;
	game_setinitpos = mastermind_setinitpos;
	game_get_pixmap = mastermind_get_pixmap;
	game_single_player = TRUE;
	game_reset_uistate = mastermind_reset_uistate;
	game_draw_cell_boundaries = TRUE;
	game_scorecmp = game_scorecmp_def_iscore;
	game_score_fields = mastermind_score_fields;
	game_score_field_names = mastermind_score_field_names;
	game_doc_about = 
		"Mastermind\n"
		"Single player game\n"
		"Status: Completely implemented\n"
		"URL: "GAME_DEFAULT_URL ("mastermind");
	game_doc_rules = 
		"Mastermind rules\n"
		"\n"
		"The objective is to find the colors of 4 hidden squares in as few tries as possible.\n\n"
		"Select a color by clicking on one of the balls on the extreme right. Place any 4 colors of your choice on the middle 4 squares of the bottom row and hit enter. You will get two numbers on the left. The number of black balls indicates how many balls you've got in the correct position. The number of white balls indicates how many balls you've got n the wrong position. Now try again on the second row. Repeat until you get all four black balls.";
	
	// TODO: complete this
}

ResultType mastermind_who_won (Pos *pos, Player to_play, char **commp)
{
	static char comment[32];
	int j;
	gboolean over = 
		pos->board [(board_heit - 1) * board_wid+ MASTERMIND_MAIN_COL_START] 
			<= 8 ? TRUE : FALSE;
	char *scorestr = over ? "You won! Tries:" : "Tries:";
	if (!over && mastermind_get_cur_row (pos->board) == board_heit - 1)
	{
		snprintf (comment, 32, "You lost. Tries: %d", board_heit - 1);
		*commp = comment;
		return RESULT_LOST;
	}
	for (j=0; j<board_heit-1; j++)
		if (!pos->board[j * board_wid])
			break;
	snprintf (comment, 32, "%s %d", scorestr, j);
	*commp = comment;
	return over ? RESULT_WON : RESULT_NOTYET;

}

int mastermind_get_cur_row (byte *board)
{
	int j;
	for (j=board_heit-2; j>=0; j--)
	{
		if (board [j * board_wid + 0] || board [j * board_wid + 1])
			return j+1;
	}
	return 0;
}

void mastermind_setinitpos (Pos *pos)
{
	int i, j;
	for (i=0; i<board_wid; i++)
	for (j=0; j<board_heit; j++)
		pos->board [j * board_wid + i] = 
			mastermind_initpos [(board_heit - 1 - j) * board_wid + i];
	for (i=MASTERMIND_MAIN_COL_START; i<=MASTERMIND_MAIN_COL_END; i++)
		pos->board [(board_heit - 1) * board_wid + i] = 9 + rand() % 6;
}

char ** mastermind_get_pixmap (int idx, int color)
{
	int fg = 0, bg = 0, i;
	char *colors;
	static char pixbuf[MASTERMIND_CELL_SIZE*(MASTERMIND_CELL_SIZE)+1];
	static char dice_pixbuf[MASTERMIND_CELL_SIZE*(MASTERMIND_CELL_SIZE)+1];
	static gboolean first = TRUE;
	colors = mastermind_colors;
	if (idx < 16)
	{
		fg += (idx & 1);
		fg += (idx & 2 ? 256 : 0);
		fg += (idx & 4 ? 65536 : 0);
		fg *= 255;
		if (idx >= 9 && idx <= 14) fg = 100 * 0x10101;
		if (color == BLACK) colors += 3;
		for(i=0, bg=0;i<3;i++) 
		{ int col = colors[i]; if (col<0) col += 256; bg += col * (1 << (16-8*i));}
		if (first)
		{
			first = FALSE;
			return pixmap_ball_gen
				(MASTERMIND_CELL_SIZE, pixbuf, fg, bg, 10.0, 30.0);
		}
		return  pixmap_ball_header_gen 
			(MASTERMIND_CELL_SIZE, pixbuf, fg, bg, 10.0, 30.0);
	}
	else if (idx < 25)
	{
		int num = (idx <= 20 ? idx - 16 : idx - 20);
		for(i=0, bg=0;i<3;i++) 
		{ int col = colors[i]; if (col<0) col += 256; bg += col * (1 << (16-8*i));}
		fg = (idx <= 20 ? 0 : 0xffffff);
		return pixmap_die_gen(MASTERMIND_CELL_SIZE, dice_pixbuf, fg, bg, 3.0, 30.0, num);
	}
	else return red_X_40_xpm;
	return NULL;
}

static int active = -1;

void mastermind_reset_uistate ()
{
	active = -1;
}

int mastermind_getmove 
  (Pos *pos, int x, int y, GtkboardEventType type, Player to_play, byte **movp)
{
	int tmp;
	static byte move[4];
	if (type != GTKBOARD_BUTTON_RELEASE)
		return 0;
	tmp = MASTERMIND_GET_SELECTION(x,y);
	if (tmp > 0) { active = tmp; return 0; }
	if (active < 1) return -1;
	if (!MASTERMIND_IS_MAIN_COL(x)) return -1;
	if (y == board_heit -1) return -1;
	if (y != mastermind_get_cur_row (pos->board)) return -1;
	move[0] = x;
	move[1] = y;
	move[2] = active;
	move[3] = -1;
	if (movp)
		*movp = move;	
	return 1;
}


int mastermind_getmove_kb (Pos *pos, int key, Player glob_to_play, byte **movp)
{
	static byte move[32];
	byte *mp = move;
	int i, j, nblack = 0, nwhite = 0, seen[4] = { 0 };
	byte *board = pos->board;
	int cur_row;
	if (key != GDK_Return) return 0;
	if (active < 0) return 0;
	cur_row = mastermind_get_cur_row (board);
	if (cur_row == board_heit - 1) return -1;
	for (i=MASTERMIND_MAIN_COL_START;i<=MASTERMIND_MAIN_COL_END;i++)
	{
		if (board [cur_row * board_wid + i] == MASTERMIND_EMPTY)
			return 0;
	}
	nblack = 0;
	for (i=MASTERMIND_MAIN_COL_START;i<=MASTERMIND_MAIN_COL_END;i++)
		if (board [cur_row * board_wid + i] == 
				(board [(board_heit - 1) * board_wid + i] & 7))
			nblack++;
	for (i=MASTERMIND_MAIN_COL_START;i<=MASTERMIND_MAIN_COL_END;i++)
	{
		for (j=MASTERMIND_MAIN_COL_START;j<=MASTERMIND_MAIN_COL_END;j++)
			if (board [cur_row * board_wid + i] == 
					(board [(board_heit - 1) * board_wid + j] & 7)
					&& !seen[j - MASTERMIND_MAIN_COL_START])
			{
				nwhite++;
				seen[j - MASTERMIND_MAIN_COL_START] = 1;
				break;
			}
	}
	assert (nwhite >= nblack);
	nwhite -= nblack;
	*mp++ = 0; *mp++ = cur_row; *mp++ = (nblack ? nblack + 16 : 25);
	*mp++ = 1; *mp++ = cur_row; *mp++ = (nwhite ? nwhite + 20 : 25);
	if (nblack == 4)
		for (i=MASTERMIND_MAIN_COL_START;i<=MASTERMIND_MAIN_COL_END;i++)
		{
			*mp++ = i; *mp++ = board_heit - 1; 
			*mp++ = board [(board_heit - 1) * board_wid + i] & 7;
		}
	*mp++ = -1;
	if (movp) *movp = move;
	cur_row++;
	return 1;
}

