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

#include <siriusid.h>
CVSID("$Id: board.c,v 1.8 2003/06/17 07:18:00 ohman Exp $");

#include <stdio.h>
#include <stdlib.h>

#include "sirius.h"
#include "search.h"
#include "board.h"
#include "hashtable.h"

#define N1 0xFFFFFFFFFFFF0000
#define N2 0xFFFFFFFFFF000000
#define N3 0xFFFFFFFF00000000
#define N4 0xFFFFFF0000000000
#define N5 0xFFFF000000000000
#define N6 0xFF00000000000000

#define E1 0x3F3F3F3F3F3F3F3F
#define E2 0x1F1F1F1F1F1F1F1F
#define E3 0x0F0F0F0F0F0F0F0F
#define E4 0x0707070707070707
#define E5 0x0303030303030303
#define E6 0x0101010101010101
	
#define W1 0xFCFCFCFCFCFCFCFC
#define W2 0xF8F8F8F8F8F8F8F8
#define W3 0xF0F0F0F0F0F0F0F0
#define W4 0xE0E0E0E0E0E0E0E0
#define W5 0xC0C0C0C0C0C0C0C0
#define W6 0x8080808080808080

#define S1 0x0000FFFFFFFFFFFF
#define S2 0x000000FFFFFFFFFF
#define S3 0x00000000FFFFFFFF
#define S4 0x0000000000FFFFFF
#define S5 0x000000000000FFFF
#define S6 0x00000000000000FF

#define NE1 0x3F3F3F3F3F3F0000
#define NE2 0x1F1F1F1F1F000000
#define NE3 0x0F0F0F0F00000000
#define NE4 0x0707070000000000
#define NE5 0x0303000000000000
#define NE6 0x0100000000000000

#define SE1 0x00003F3F3F3F3F3F
#define SE2 0x0000001F1F1F1F1F
#define SE3 0x000000000F0F0F0F
#define SE4 0x0000000000070707
#define SE5 0x0000000000000303
#define SE6 0x0000000000000001

#define SW1 0x0000FCFCFCFCFCFC
#define SW2 0x000000F8F8F8F8F8
#define SW3 0x00000000F0F0F0F0
#define SW4 0x0000000000E0E0E0
#define SW5 0x000000000000C0C0
#define SW6 0x0000000000000080
	
#define NW1 0xFCFCFCFCFCFC0000
#define NW2 0xF8F8F8F8F8000000
#define NW3 0xF0F0F0F000000000
#define NW4 0xE0E0E00000000000
#define NW5 0xC0C0000000000000
#define NW6 0x8000000000000000


static const int move_priority[60] = {
					55,50,15,10,
					16,56,63,58,49,9,7,2,
					51,54,47,42,23,18,14,11,
					39,31,53,52,34,26,13,12,
					38,30,45,44,35,27,21,20,
					33,25,61,60,40,32,5,4,
					46,43,22,19,
					62,59,48,41,24,17,6,3,
					64,57,8,1
				     };


char *squares[64] =	{	
				"a1","b1","c1","d1","e1","f1","g1","h1",
				"a2","b2","c2","d2","e2","f2","g2","h2",
				"a3","b3","c3","d3","e3","f3","g3","h3",
				"a4","b4","c4","d4","e4","f4","g4","h4",
				"a5","b5","c5","d5","e5","f5","g5","h5",
          			"a6","b6","c6","d6","e6","f6","g6","h6",
				"a7","b7","c7","d7","e7","f7","g7","h7",
				"a8","b8","c8","d8","e8","f8","g8","h8" 
			};

char *squares_mirror[64] =	{	
				"a1","a2","a3","a4","a5","a6","a7","a8",
				"b1","b2","b3","b4","b5","b6","b7","b8",
				"c1","c2","c3","c4","c5","c6","c7","c8",
				"d1","d2","d3","d4","d5","d6","d7","d8",
				"e1","e2","e3","e4","e5","e6","e7","e8",
          			"f1","f2","f3","f4","f5","f6","f7","f8",
				"g1","g2","g3","g4","g5","g6","g7","g8",
				"h1","h2","h3","h4","h5","h6","h7","h8" 
			};


void legal_moves(board *b, int bestmove);
static board *transposition_hash(board *b);
int get_mobility_from_move(board *b, int move);
static u64 calculate_flips(u64 me, u64 you, u64 mask);
static u64 calculate_legal(u64 me, u64 you);
void print_bit_pattern(u64 b);
void dump(board *b);
int numbits(u64 orig);

/*
 * init_board:
 * Creates and initiates a new othelloboard.
 *
 */
board *init_board(board * b, int black_time, int white_time) {
	u64 mask = 1;

	b     = (board *) malloc(sizeof(board));
	b->tt = (transpositiontable *) malloc(sizeof(transpositiontable) * TRANSPOSITION_TABLE_SIZE);

	b->black = 0;
	b->black |= (mask << 28);
	b->black |= (mask << 35);
	b->white = 0;
	b->white |= (mask << 27);
	b->white |= (mask << 36);
	b->color_to_move = BLACK;

	b->half_move = 0;
	b->game_over = 0;
	b->pass      = 0;
	b->found_end = 0;

	b->time_left[WHITE] = white_time;
	b->time_left[BLACK] = black_time;
	
	/* stuff for the transposition table */
	b = transposition_hash(b);
	clean_transpositiontable(b);
	
	return (b);
}

void deinit_board(board *b) {
	if(b) {
		free(b);
	}
}

board *load_board(board *b, testboard tb) {
	u64 mask = 1; 
	int i;

	b     = (board *) malloc(sizeof(board));
	b->tt = (transpositiontable *) malloc(sizeof(transpositiontable) * TRANSPOSITION_TABLE_SIZE);

	b->black = 0;
	b->white = 0;	
	for(i=0; i<64; i++) {
		if(tb[i] == BLACK) {
			b->black |= (mask << (i));
		}
		if(tb[i] == WHITE) {
			b->white |= (mask << (i));
		}
	}		

	b->color_to_move = tb[64];

        b->half_move = numbits(b->black | b->white);
	b->pass      = 0;
        b->game_over = 0;
	b->found_end = 0;

        b->time_left[WHITE] = GAMETIME;
        b->time_left[BLACK] = GAMETIME;

        /* stuff for the transposition table */
	b = transposition_hash(b);
	clean_transpositiontable(b);
	
        return (b);
}


/**
 * do_move:
 * Make a move.
 */
board *do_move(board *b, int move, undo_info *ui) {
	u64 mask = 1;
	u64 flips;
	short int *to;
	short int *from;

	mask = mask << (move-1);
	if(b->color_to_move) {
		flips = calculate_flips(b->black, b->white, mask);
		b->black |= flips;
		b->white &= ~flips;
		b->color_to_move = WHITE;

		/* save undo info */
		ui->undo_pattern = flips;
		ui->undo_color = BLACK;
	} else {
		flips = calculate_flips(b->white, b->black, mask);
		b->white |= flips;
		b->black &= ~flips;
		b->color_to_move = BLACK;
		
		/* save undo info */
		ui->undo_pattern = flips;
		ui->undo_color = WHITE;
	}

	/* save more undo info */
	ui->undo_x               = b->x;
	ui->undo_y               = b->y;
	ui->undo_mask            = mask;
	ui->undo_num_legal_moves = b->num_legal_moves; 
	ui->pass                 = b->pass;
	ui->game_over            = b->game_over;
	

	to   = (short int *) &(ui->undo_legal_move);
	from = (short int *) &(b->legal_move);
	while((*from) != -1) {
		*to++ = *from++;
	}
	*to = -1;

/*
	for(i=0; i<ui->undo_num_legal_moves+1; i++) {
		printf("%d\n", ui->undo_legal_move[i]);
	}
	exit(1);
*/


/*	
	for(i=b->num_legal_moves; i--; ) {
		ui->undo_legal_move[i] = b->legal_move[i];
	}
*/

	/* update the transposition table indexes */
	b = transposition_hash(b);

	b->half_move++;
	b->pass = 0;

	return (b);
}

board *do_pass(board *b, undo_info *ui) {
		
	if(b->color_to_move == BLACK) {
		b->color_to_move = WHITE;
	} else {
		b->color_to_move = BLACK;
	}

	ui->undo_x               = b->x;
        ui->undo_y               = b->y;
        ui->undo_mask            = 0x0;
        ui->undo_num_legal_moves = b->num_legal_moves; 
        ui->pass                 = b->pass;
        ui->game_over            = b->game_over;

	b->pass = 1;

	/* update the transposition table indexes */
	b = transposition_hash(b);

	return (b);
}

/*
 * undo_move:
 * Undo a move. 
 */
board *undo_move(board *b, undo_info *ui) {
	short int *to;
	short int *from;

	if(ui->undo_color) {
		b->white |= ui->undo_pattern;
		b->black &= ~ui->undo_pattern;
		b->white &= ~ui->undo_mask;
	} else {
		b->black |= ui->undo_pattern;
		b->white &= ~ui->undo_pattern;
		b->black &= ~ui->undo_mask;
	}

	/* stuff for the transposition table */
	b->x = ui->undo_x;
	b->y = ui->undo_y;
	
	b->half_move--;
	b->num_legal_moves = ui->undo_num_legal_moves; 
	b->pass            = ui->pass;
	b->game_over       = ui->game_over;


	to   = (short int *) &(b->legal_move);
	from = (short int *) &(ui->undo_legal_move);
	while((*from) != -1) {
		*to++ = *from++;
	}
	*to = -1;

/*
	b->legal_move[ui->undo_num_legal_moves+1]=-1;
	for(i=ui->undo_num_legal_moves; i--; ) {
		b->legal_move[i] = ui->undo_legal_move[i];
	}
*/

	b->color_to_move = ui->undo_color;
	return (b);
}


/*
 * legal_moves:
 * Calculates the number of legal moves and places 
 * them in a list. 
 */
void legal_moves(board *b, int bestmove) {
	int sortnum;
	register unsigned int j;
	register unsigned int i = 1;
	short int *pnt;
	u64 mask = 1;
	u64 legal;

	if(b->color_to_move) {
                legal = calculate_legal(b->black,b->white);
        } else {
                legal = calculate_legal(b->white,b->black);
        }

	pnt = (short int *) &(b->legal_move);
	while(legal) {
		if(legal & mask) {
			*pnt++ = i; 
			legal &= ~mask;
		}
		i++;
		mask = mask << 1;
	}
	*pnt = -1;
	b->num_legal_moves = pnt - b->legal_move; 


	/* sort the movelist */
	/* put the best move first */
	if(b->num_legal_moves > 1) {
		if(bestmove != 0) {
			sortnum = 0;
			for(i = b->num_legal_moves; i--; ) {
				if(b->legal_move[i] == bestmove) {
					if(i > 0) {
						int tmp = b->legal_move[0];
						b->legal_move[0] = b->legal_move[i];
						b->legal_move[i] = tmp;
						sortnum = 1;
					}
					break;
				}
			}
			if(b->half_move <= 30) {
				for(i=59; i--; ) {
					for(j=b->num_legal_moves; j--; ) {
						if(b->legal_move[j] == move_priority[i]) {
							int tmp = b->legal_move[sortnum];
							b->legal_move[sortnum] = b->legal_move[j];
							b->legal_move[j] = tmp;
							sortnum++;
							break;
						}
					}
					if((sortnum == 3) || (b->num_legal_moves == sortnum)) break;
				}
			}
		}
	}

	if(b->num_legal_moves == 0) {
                if(b->pass == 1 || b->half_move == 60 || b->black == 0 || b->white == 0) {
                        b->game_over = 1;
                } else {
                        b->pass = 1;
                }
        } else {
                b->pass = 0;
        }
}


static board *transposition_hash(board *b) {
/*   2^^20
	b->y = hash((ub4*)&b->black, 4, 0x539eb2e7);
	b->x = (b->y & 0x0000fffff);
*/

	b->y = hash((ub4*)&b->black, 4, 0x539eb2e7);
	b->x = (b->y & 0x00000ffff);

	return (b);
}


/*
 * Returns the mobility of the board in blacks favour
 *
 */
void mobility(board *b) {
	u64 legal;

	if(b->color_to_move) {
                legal = calculate_legal(b->black,b->white);
        } else {
                legal = calculate_legal(b->white,b->black);
        }
	b->num_legal_moves = numbits(legal);
}


int get_mobility_from_move(board *b, int move) {
	u64 mask = 1;
	u64 flips;
	u64 bl,wh;

	bl = b->black;
	wh = b->white;

	mask = mask << (move-1);
	if(b->color_to_move) {
		flips = calculate_flips(bl, wh, mask);
		bl |= flips;
		wh &= ~flips;
		return (numbits(calculate_legal(bl, wh)));
	} else {
		flips = calculate_flips(wh, bl, mask);
		wh |= flips;
		bl &= ~flips;
		return (numbits(calculate_legal(wh,bl)));
	}

	return (0);
}


/*
 * pos:
 * Returnerar den pjäs som finns på efterfrågad
 * plats. 
 */
int pos(board *b, int m) {
	u64 mask = 1;
	if(b->black & (mask << (m-1))) {
		return (BLACK);
	} else if(b->white & (mask << (m-1))) {
		return (WHITE);
	} else return(EMPTY);
}


char *int2pos(int move) {
	if(move == 0) {
		return ("-");
	}
	return (squares[move - 1]);
}

char *int2pos_mirror(int move) {
	if(move == 0) {
		return ("-");
	}
	return (squares_mirror[move - 1]);
}

int legal(board *b, int move) {
	int i;
	for(i=0; i<b->num_legal_moves; i++) {
		if(b->legal_move[i] == move) {
			return (1);
		}
	}
	return (0);
}       

void clean_transpositiontable(board *b) {
	unsigned int i;

	for(i=0; i<TRANSPOSITION_TABLE_SIZE; i++) {
		b->tt[i].checksum = 0;
		b->tt[i].depth    = 0;
	}
}


/*
 * dump:
 * Dumpar brädet på stdout
 *
 */
void dump(board *b) {
	int i;
	int row=1;
	int vit=0,svart=0;
	
	printf("\n    A  B  C  D  E  F  G  H\n\n1  ");
	for(i=1; i<65; i++) {
		switch(pos(b,i)) {
		case BLACK : 
			printf(" X ");
			svart++;
			break;
		case WHITE : 
			printf(" O ");
			vit++;
			break;
		case EMPTY :
			printf(" + ");
			break;
		}
		if((i)%8 == 0) {
			if(row == 8) {
				printf("  %d\n", row);
			} else {
				printf("  %d\n%d  ", row, row+1);
				row++;
			}
		}
	}
	printf("\n    A  B  C  D  E  F  G  H\n\n");
	printf("             %d - %d\n", svart, vit);
	printf("            %ld - %ld\n", b->time_left[BLACK], b->time_left[WHITE]);
}
		  

static u64 calculate_legal(u64 me, u64 you) {
	register u64 free  = ~(me | you);
	register u64 value;

	value =  (free &
                ((N1  & (you << 8) & (me << 16))  |
                 (NW1 & (you << 9) & (me << 18))  |
                 (W1  & (you << 1) & (me << 2))   |
                 (SW1 & (you >> 7) & (me >> 14))  |
                 (S1  & (you >> 8) & (me >> 16))  |
                 (SE1 & (you >> 9) & (me >> 18))  |
                 (E1  & (you >> 1) & (me >> 2))   |
                 (NE1 & (you << 7) & (me << 14))));

	value |= (free &
                ((N2   & (you << 8) & (you << 16) & (me << 24))  |
                 (NW2  & (you << 9) & (you << 18) & (me << 27))  |
                 (W2   & (you << 1) & (you << 2)  & (me << 3))   |
                 (SW2  & (you >> 7) & (you >> 14) & (me >> 21))  |
                 (S2   & (you >> 8) & (you >> 16) & (me >> 24))  |
                 (SE2  & (you >> 9) & (you >> 18) & (me >> 27))  |
                 (E2   & (you >> 1) & (you >> 2)  & (me >> 3))   |
                 (NE2  & (you << 7) & (you << 14) & (me << 21))));

        value |= (free &
                ((N3   & (you << 8) & (you << 16) & (you << 24) & (me << 32))  |
                 (NW3  & (you << 9) & (you << 18) & (you << 27) & (me << 36))  |
                 (W3   & (you << 1) & (you << 2)  & (you << 3)  & (me << 4))   |
                 (SW3  & (you >> 7) & (you >> 14) & (you >> 21) & (me >> 28))  |
                 (S3   & (you >> 8) & (you >> 16) & (you >> 24) & (me >> 32))  |
                 (SE3  & (you >> 9) & (you >> 18) & (you >> 27) & (me >> 36))  |
                 (E3   & (you >> 1) & (you >> 2)  & (you >> 3)  & (me >> 4))   |
                 (NE3  & (you << 7) & (you << 14) & (you << 21) & (me << 28))));

	value |= (free &
                ((N4   & (you << 8) & (you << 16) & (you << 24) & (you << 32) & (me << 40))  |
                 (NW4  & (you << 9) & (you << 18) & (you << 27) & (you << 36) & (me << 45))  |
                 (W4   & (you << 1) & (you << 2)  & (you << 3)  & (you << 4)  & (me << 5))   |
                 (SW4  & (you >> 7) & (you >> 14) & (you >> 21) & (you >> 28) & (me >> 35))  |
                 (S4   & (you >> 8) & (you >> 16) & (you >> 24) & (you >> 32) & (me >> 40))  |
                 (SE4  & (you >> 9) & (you >> 18) & (you >> 27) & (you >> 36) & (me >> 45))  |
                 (E4   & (you >> 1) & (you >> 2)  & (you >> 3)  & (you >> 4)  & (me >> 5))   |
                 (NE4  & (you << 7) & (you << 14) & (you << 21) & (you << 28) & (me << 35))));

	value |= (free &
                ((N5   & (you << 8) & (you << 16) & (you << 24) & (you << 32) & (you << 40) & (me << 48))  |
                 (NW5  & (you << 9) & (you << 18) & (you << 27) & (you << 36) & (you << 45) & (me << 54))  |
                 (W5   & (you << 1) & (you << 2)  & (you << 3)  & (you << 4)  & (you << 5)  & (me << 6))   |
                 (SW5  & (you >> 7) & (you >> 14) & (you >> 21) & (you >> 28) & (you >> 35) & (me >> 42))  |
                 (S5   & (you >> 8) & (you >> 16) & (you >> 24) & (you >> 32) & (you >> 40) & (me >> 48))  |
                 (SE5  & (you >> 9) & (you >> 18) & (you >> 27) & (you >> 36) & (you >> 45) & (me >> 54))  |
                 (E5   & (you >> 1) & (you >> 2)  & (you >> 3)  & (you >> 4)  & (you >> 5)  & (me >> 6))   |
                 (NE5  & (you << 7) & (you << 14) & (you << 21) & (you << 28) & (you << 35) & (me << 42))));

        value |= (free &
                ((N6   & (you << 8) & (you << 16) & (you << 24) & (you << 32) & (you << 40) & (you << 48) & (me << 56))  |
                 (NW6  & (you << 9) & (you << 18) & (you << 27) & (you << 36) & (you << 45) & (you << 54) & (me << 63))  |
                 (W6   & (you << 1) & (you << 2)  & (you << 3)  & (you << 4)  & (you << 5)  & (you << 6)  & (me << 7))   |
                 (SW6  & (you >> 7) & (you >> 14) & (you >> 21) & (you >> 28) & (you >> 35) & (you >> 42) & (me >> 49))  |
                 (S6   & (you >> 8) & (you >> 16) & (you >> 24) & (you >> 32) & (you >> 40) & (you >> 48) & (me >> 56))  |
                 (SE6  & (you >> 9) & (you >> 18) & (you >> 27) & (you >> 36) & (you >> 45) & (you >> 54) & (me >> 63))  |
                 (E6   & (you >> 1) & (you >> 2)  & (you >> 3)  & (you >> 4)  & (you >> 5)  & (you >> 6)  & (me >> 7))   |
                 (NE6  & (you << 7) & (you << 14) & (you << 21) & (you << 28) & (you << 35) & (you << 42) & (me << 49))));

	return (value);
}

static u64 calculate_flips(u64 me, u64 you, u64 mask) {
	u64 tmp;
	register u64 value = mask;

	/* SOUTH */
	tmp = ((mask << 8) & you);
	if(tmp) {
		if((mask & S1) && ((mask << 16) & me)) {
			value |= (mask << 8);
		} else if((mask & S2) &&
			  ((mask << 16) & you) && ((mask << 24) & me)) {
			value |= ((mask << 8) | (mask << 16));
		} else if((mask & S3) &&
			  ((mask << 16) & you) && ((mask << 24) & you) &&
			  ((mask << 32) & me)) {
			value |= ((mask << 8) | (mask << 16) | (mask << 24));
		} else if((mask & S4) &&
			  ((mask << 16) & you) && ((mask << 24) & you) &&
			  ((mask << 32) & you) && ((mask << 40) & me)) {
			value |= ((mask << 8) | (mask << 16) | (mask << 24) | (mask << 32));
		} else if((mask & S5) &&
			  ((mask << 16) & you) && ((mask << 24) & you) &&
			  ((mask << 32) & you) && ((mask << 40) & you) &&
			  ((mask << 48) & me)) {
			value |= ((mask << 8) | (mask << 16) | (mask << 24) | (mask << 32) | (mask << 40));
		} else if((mask & S6) &&
			  ((mask << 16) & you) && ((mask << 24) & you) &&
			  ((mask << 32) & you) && ((mask << 40) & you) &&
			  ((mask << 48) & you) && ((mask << 56) & me)) {
			value |= ((mask << 8) | (mask << 16) | (mask << 24) | (mask << 32) | (mask << 40) | (mask << 48));
		}
	}
	/* SOUTHEAST */
	tmp = ((mask << 9) & you);
	if(tmp) {
		if((mask & SE1) && ((mask << 18) & me)) {
			value |= (mask << 9);
		} else if((mask & SE2) &&
			  ((mask << 18) & you) && ((mask << 27) & me)) {
			value |= ((mask << 9) | (mask << 18));
		} else if((mask & SE3) &&
			  ((mask << 18) & you) && ((mask << 27) & you) &&
			  ((mask << 36) & me)) {
			value |= ((mask << 9) | (mask << 18) | (mask << 27));
		} else if((mask & SE4) &&
			  ((mask << 18) & you) && ((mask << 27) & you) &&
			  ((mask << 36) & you) && ((mask << 45) & me)) {
			value |= ((mask << 9) | (mask << 18) | (mask << 27) | (mask << 36));
		} else if((mask & SE5) &&
			  ((mask << 18) & you) && ((mask << 27) & you) &&
			  ((mask << 36) & you) && ((mask << 45) & you) &&
			  ((mask << 54) & me)) {
			value |= ((mask << 9) | (mask << 18) | (mask << 27) | (mask << 36) | (mask << 45));
		} else if((mask & SE6) &&
			  ((mask << 18) & you) && ((mask << 27) & you) &&
			  ((mask << 36) & you) && ((mask << 45) & you) &&
			  ((mask << 54) & you) && ((mask << 63) & me)) {
			value |= ((mask << 9) | (mask << 18) | (mask << 27) | (mask << 36) | (mask << 45) | (mask << 54));
		}
	}
	/* EAST */
	tmp = ((mask << 1) & you);
	if(tmp) {
		if((mask & E1) && ((mask << 2) & me)) {
			value |= (mask << 1);
		} else if((mask & E2) &&
			  ((mask << 2) & you) && ((mask << 3) & me)) {
			value |= ((mask << 1) | (mask << 2));
		} else if((mask & E3) &&
			  ((mask << 2) & you) && ((mask << 3) & you) &&
			  ((mask << 4) & me)) {
			value |= ((mask << 1) | (mask << 2) | (mask << 3));
		} else if((mask & E4) &&
			  ((mask << 2) & you) && ((mask << 3) & you) &&
			  ((mask << 4) & you) && ((mask << 5) & me)) {
			value |= ((mask << 1) | (mask << 2) | (mask << 3) | (mask << 4));
		} else if((mask & E5) &&
			  ((mask << 2) & you) && ((mask << 3) & you) &&
			  ((mask << 4) & you) && ((mask << 5) & you) &&
			  ((mask << 6) & me)) {
			value |= ((mask << 1) | (mask << 2) | (mask << 3) | (mask << 4) | (mask << 5));
		} else if((mask & E6) &&
			  ((mask << 2) & you) && ((mask << 3) & you) &&
			  ((mask << 4) & you) && ((mask << 5) & you) &&
			  ((mask << 6) & you) && ((mask << 7) & me)) {
			value |= ((mask << 1) | (mask << 2) | (mask << 3) | (mask << 4) | (mask << 5) | (mask << 6));
		}
	}
	/* SOUTHWEST */
	tmp = ((mask << 7) & you);
	if(tmp) {
		if((mask & SW1) && ((mask << 14) & me)) {
			value |= (mask << 7);
		} else if((mask & SW2) &&
			  ((mask << 14) & you) && ((mask << 21) & me)) {
			value |= ((mask << 7) | (mask << 14));
		} else if((mask & SW3) &&
			  ((mask << 14) & you) && ((mask << 21) & you) &&
			  ((mask << 28) & me)) {
			value |= ((mask << 7) | (mask << 14) | (mask << 21));
		} else if((mask & SW4) &&
			  ((mask << 14) & you) && ((mask << 21) & you) &&
			  ((mask << 28) & you) && ((mask << 35) & me)) {
			value |= ((mask << 7) | (mask << 14) | (mask << 21) | (mask << 28));
		} else if((mask & SW5) &&
			  ((mask << 14) & you) && ((mask << 21) & you) &&
			  ((mask << 28) & you) && ((mask << 35) & you) &&
			  ((mask << 42) & me)) {
			value |= ((mask << 7) | (mask << 14) | (mask << 21) | (mask << 28) | (mask << 35));
		} else if((mask & SW6) &&
			  ((mask << 14) & you) && ((mask << 21) & you) &&
			  ((mask << 28) & you) && ((mask << 35) & you) &&
			  ((mask << 42) & you) && ((mask << 49) & me)) {
			value |= ((mask << 7) | (mask << 14) | (mask << 21) | (mask << 28) | (mask << 35) | (mask << 42));
		}
	}
	/* NORTH */
	tmp = ((mask >> 8) & you);
	if(tmp) {
		if((mask & N1) && ((mask >> 16) & me)) {
			value |= (mask >> 8);
		} else if((mask & N2) &&
			  ((mask >> 16) & you) && ((mask >> 24) & me)) {
			value |= ((mask >> 8) | (mask >> 16));
		} else if((mask & N3) &&
			  ((mask >> 16) & you) && ((mask >> 24) & you) &&
			  ((mask >> 32) & me)) {
			value |= ((mask >> 8) | (mask >> 16) | (mask >> 24));
		} else if((mask & N4) &&
			  ((mask >> 16) & you) && ((mask >> 24) & you) &&
			  ((mask >> 32) & you) && ((mask >> 40) & me)) {
			value |= ((mask >> 8) | (mask >> 16) | (mask >> 24) | (mask >> 32));
		} else if((mask & N5) &&
			  ((mask >> 16) & you) && ((mask >> 24) & you) &&
			  ((mask >> 32) & you) && ((mask >> 40) & you) &&
			  ((mask >> 48) & me)) {
			value |= ((mask >> 8) | (mask >> 16) | (mask >> 24) | (mask >> 32) | (mask >> 40));
		} else if((mask & N6) &&
			  ((mask >> 16) & you) && ((mask >> 24) & you) &&
			  ((mask >> 32) & you) && ((mask >> 40) & you) &&
			  ((mask >> 48) & you) && ((mask >> 56) & me)) {
			value |= ((mask >> 8) | (mask >> 16) | (mask >> 24) | (mask >> 32) | (mask >> 40) | (mask >> 48));
		}
	}
	/* NORTHWEST */
	tmp = ((mask >> 9) & you);
	if(tmp) {
		if((mask & NW1) && ((mask >> 18) & me)) {
			value |= (mask >> 9);
		} else if((mask & NW2) &&
			  ((mask >> 18) & you) && ((mask >> 27) & me)) {
			value |= ((mask >> 9) | (mask >> 18));
		} else if((mask & NW3) &&
			  ((mask >> 18) & you) && ((mask >> 27) & you) &&
			  ((mask >> 36) & me)) {
			value |= ((mask >> 9) | (mask >> 18) | (mask >> 27));
		} else if((mask & NW4) &&
			  ((mask >> 18) & you) && ((mask >> 27) & you) &&
			  ((mask >> 36) & you) && ((mask >> 45) & me)) {
			value |= ((mask >> 9) | (mask >> 18) | (mask >> 27) | (mask >> 36));
		} else if((mask & NW5) &&
			  ((mask >> 18) & you) && ((mask >> 27) & you) &&
			  ((mask >> 36) & you) && ((mask >> 45) & you) &&
			  ((mask >> 54) & me)) {
			value |= ((mask >> 9) | (mask >> 18) | (mask >> 27) | (mask >> 36) | (mask >> 45));
		} else if((mask & NW6) &&
			  ((mask >> 18) & you) && ((mask >> 27) & you) &&
			  ((mask >> 36) & you) && ((mask >> 45) & you) &&
			  ((mask >> 54) & you) && ((mask >> 63) & me)) {
			value |= ((mask >> 9) | (mask >> 18) | (mask >> 27) | (mask >> 36) | (mask >> 45) | (mask >> 54));
		}
	}
	/* WEST */
	tmp = ((mask >> 1) & you);
	if(tmp) {
		if((mask & W1) && ((mask >> 2) & me)) {
			value |= (mask >> 1);
		} else if((mask & W2) &&
			  ((mask >> 2) & you) && ((mask >> 3) & me)) {
			value |= ((mask >> 1) | (mask >> 2));
		} else if((mask & W3) &&
			  ((mask >> 2) & you) && ((mask >> 3) & you) &&
			  ((mask >> 4) & me)) {
			value |= ((mask >> 1) | (mask >> 2) | (mask >> 3));
		} else if((mask & W4) &&
			  ((mask >> 2) & you) && ((mask >> 3) & you) &&
			  ((mask >> 4) & you) && ((mask >> 5) & me)) {
			value |= ((mask >> 1) | (mask >> 2) | (mask >> 3) | (mask >> 4));
		} else if((mask & W5) &&
			  ((mask >> 2) & you) && ((mask >> 3) & you) &&
			  ((mask >> 4) & you) && ((mask >> 5) & you) &&
			  ((mask >> 6) & me)) {
			value |= ((mask >> 1) | (mask >> 2) | (mask >> 3) | (mask >> 4) | (mask >> 5));
		} else if((mask & W6) &&
			  ((mask >> 2) & you) && ((mask >> 3) & you) &&
			  ((mask >> 4) & you) && ((mask >> 5) & you) &&
			  ((mask >> 6) & you) && ((mask >> 7) & me)) {
			value |= ((mask >> 1) | (mask >> 2) | (mask >> 3) | (mask >> 4) | (mask >> 5) | (mask >> 6));
		}
	}
	/* NORTHEAST */
	tmp = ((mask >> 7) & you);
	if(tmp) {
		if((mask & NE1) && ((mask >> 14) & me)) {
			value |= (mask >> 7);
		} else if((mask & NE2) &&
			  ((mask >> 14) & you) && ((mask >> 21) & me)) {
			value |= ((mask >> 7) | (mask >> 14));
		} else if((mask & NE3) &&
			  ((mask >> 14) & you) && ((mask >> 21) & you) &&
			  ((mask >> 28) & me)) {
			value |= ((mask >> 7) | (mask >> 14) | (mask >> 21));
		} else if((mask & NE4) &&
			  ((mask >> 14) & you) && ((mask >> 21) & you) &&
			  ((mask >> 28) & you) && ((mask >> 35) & me)) {
			value |= ((mask >> 7) | (mask >> 14) | (mask >> 21) | (mask >> 28));
		} else if((mask & NE5) &&
			  ((mask >> 14) & you) && ((mask >> 21) & you) &&
			  ((mask >> 28) & you) && ((mask >> 35) & you) &&
			  ((mask >> 42) & me)) {
			value |= ((mask >> 7) | (mask >> 14) | (mask >> 21) | (mask >> 28) | (mask >> 35));
		} else if((mask & NE6) &&
			  ((mask >> 14) & you) && ((mask >> 21) & you) &&
			  ((mask >> 28) & you) && ((mask >> 35) & you) &&
			  ((mask >> 42) & you) && ((mask >> 49) & me)) {
			value |= ((mask >> 7) | (mask >> 14) | (mask >> 21) | (mask >> 28) | (mask >> 35) | (mask >> 42));
		}
	}
	return (value);
}


void print_bit_pattern(u64 b) {
	int i;
	u64 mask = 1;

	for(i=0; i<64; i++) {
		if (mask & b) {
			printf("1");
		} else {
			printf("0");
		}
		mask = mask << 1;
		if((i+1)%8 == 0) printf("\n");
	}
	printf("\n");
}

u64 rotate_bit_pattern(u64 orig) {
	int delta[8] = {7,15,23,31,39,47,55,63};
	int i,j,k;
	u64 new =  0;
	u64 mask1 = 1;
	u64 mask2 = 1;

	j=0;
	for(i=0; i<64; i++) {
		if((mask1 << i) & orig) new |= (mask2 << delta[j]);
		if(j == 7) {
			j = 0;
			for(k=0; k<8; k++) delta[k] = delta[k] - 1;
		} else {
			j++;
		}
	}
	return new;
}

u64 trans_bit_pattern(u64 orig) {
	u64 new  = 0;
	u64 mask = 1;
	int i,j,k;

	for(i=0; i<64; i++) {
		j = (int)((i) / 8);
		j *= 8;

		k = j + (8 - (i - j)) - 1;

		if((mask << (k)) & orig) {
			new |= (mask << (i));
		}
	}
	
	return (new);
}

int numbits(u64 orig) {
	orig = (orig & 0x5555555555555555) + ((orig & 0xaaaaaaaaaaaaaaaa) >> 1);
	orig = (orig & 0x3333333333333333) + ((orig & 0xcccccccccccccccc) >> 2);
	orig = (orig & 0x0f0f0f0f0f0f0f0f) + ((orig & 0xf0f0f0f0f0f0f0f0) >> 4);
	orig = (orig & 0x00ff00ff00ff00ff) + ((orig & 0xff00ff00ff00ff00) >> 8);
	orig = (orig & 0x0000ffff0000ffff) + ((orig & 0xffff0000ffff0000) >> 16);
	orig = (orig & 0x00000000ffffffff) + ((orig & 0xffffffff00000000) >> 32);

	return ((int)(orig));
}
