/*
 *  Copyright (C) 2002-2003 Henrik Öhman
 *
 *  This file is part of Sirius.
 *
 *  Sirius is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Sirius is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Sirius; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Author: Henrik Öhman <henrik@bitvis.nu>
 *
 */

#ifndef __SIRIUS_H
#define __SIRIUS_H

#define APPNAME "sirius"

#define BLACK 1
#define WHITE 0
#define EMPTY 2

#define COMPUTER_PLAYER 2
#define HUMAN_PLAYER 3

#define MAXDEPTH 10
#define INFINITY 1000000

#define GAMETIME 60000
#define TIMESLICE 8000

#define u64 unsigned long long

#define TRANSPOSITION_TABLE_SIZE  262144 /* 2^^18 */  
			       /* 1048576   2^^20 */ 

typedef int testboard[65];
typedef  unsigned long  int  ub4;
typedef  unsigned       char ub1;

typedef struct _transpositiontable {
        ub4 checksum;
  	int depth;
  	int eval;
	int best;
  	enum { exact, lower_bound, upper_bound } entry_type;
} transpositiontable;


typedef struct _board {
	u64 black;
	u64 white;

	/* to the transposition table */
	unsigned int x;
	ub4 y;

	short int found_end;
	short int game_over;
	short int pass;
	short int player_black;
	short int player_white;
	short int color_to_move;
	short int half_move;
	short int level;
	short int num_legal_moves;
	short int legal_move[25];
	long time_left[2];
	transpositiontable *tt;
} board;


typedef struct _undo_info {
        short int undo_num_legal_moves;
        short int undo_legal_move[25];
        short int undo_color;
	short int pass;
	short int game_over;
        unsigned int undo_x;
        u64 undo_y;
        u64 undo_mask;
        u64 undo_pattern;
} undo_info;

#endif
