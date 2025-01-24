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
CVSID("$Id: evaluation.c,v 1.3 2003/01/14 12:28:59 ohman Exp $");

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "sirius.h"
#include "hashtable.h"
#include "evaluation.h"
#include "board.h"

#define CORNER1    0x0000000000070707
#define CORNER2    0x0707070000000000
#define CORNER3    0xE0E0E00000000000
#define CORNER4    0x0000000000E0E0E0

#define NEDGE1     0x00000000000000FF
#define SEDGE1     0xFF00000000000000
#define EEDGE1     0x8080808080808080
#define WEDGE1     0x0101010101010101

#define POTMOB     0x003c7e7e7e7e3c00

#define N1   0xFFFFFFFFFFFF0000
#define E1   0x3F3F3F3F3F3F3F3F
#define W1   0xFCFCFCFCFCFCFCFC
#define S1   0x0000FFFFFFFFFFFF
#define NE1  0x3F3F3F3F3F3F0000
#define SE1  0x00003F3F3F3F3F3F
#define SW1  0x0000FCFCFCFCFCFC
#define NW1  0xFCFCFCFCFCFC0000

#define E(x)   ((x >> 1) & E1)
#define W(x)   ((x << 1) & W1)
#define N(x)   ((x << 8) & N1)
#define S(x)   ((x >> 8) & S1) 
#define SE(x)  ((x >> 9) & SE1)
#define SW(x)  ((x >> 7) & SW1)
#define NE(x)  ((x << 7) & NE1)
#define NW(x)  ((x << 9) & NW1)


float mobility_coef[8] = {0.088947, 0.20782, 0.40319, 0.433412, 1.0798, 2.3535, 3.0989, 4.3};
float edge_coef[8]     = {1.1315185, 0.807301, 0.406408, 0.5252555, 0.87827, 1.18325, 0.92189, 0.9};
float corner_coef[8]   = {1.1643080, 0.935728, 0.948664, 1.0113549, 1.05460, 1.11807, 0.97812, 0.9};

hash_table *edge1x;
hash_table *corner3x3;

static hash_table *load(char *filename, int size, int n);


int evaluate(board *b) {
	float value;
	int stage = (int)((b->half_move - 10) / 6);

	value  = get_corner(b, stage) * corner_coef[stage];
	value += get_edge1x(b, stage) * edge_coef[stage];
	value += get_mobility(b) * mobility_coef[stage];
	value += get_potential_mobility(b) * 10;
	if(stage == 8) {
		value += get_parity(b) * 600;
	}

	return ((int)(value));
}

float get_corner(board *b, int stage) {
	float value;
	
	value  = hash_table_find(corner3x3, (b->black & CORNER1), (b->white & CORNER1), stage);
	value += hash_table_find(corner3x3, (b->black & CORNER2), (b->white & CORNER2), stage);
	value += hash_table_find(corner3x3, (b->black & CORNER3), (b->white & CORNER3), stage);
	value += hash_table_find(corner3x3, (b->black & CORNER4), (b->white & CORNER4), stage);

	return (value);
}

float get_edge1x(board *b, int stage) {
	float value;

	value  = hash_table_find(edge1x, (b->black & NEDGE1), (b->white & NEDGE1), stage);
	value += hash_table_find(edge1x, (b->black & SEDGE1), (b->white & SEDGE1), stage);
	value += hash_table_find(edge1x, (b->black & EEDGE1), (b->white & EEDGE1), stage);
	value += hash_table_find(edge1x, (b->black & WEDGE1), (b->white & WEDGE1), stage);
	
	return (value);
}

int get_mobility(board *b) {
	int value;
	
	if(b->color_to_move == BLACK) {
		b->color_to_move = WHITE;
                mobility(b);
                value = -b->num_legal_moves;

                b->color_to_move = BLACK;
                mobility(b);
                value += b->num_legal_moves;
        } else {
                b->color_to_move = BLACK;
                mobility(b);
                value = b->num_legal_moves;

                b->color_to_move = WHITE;
                mobility(b);
                value -= b->num_legal_moves;
	}

	return (-value);
}

int get_potential_mobility(board *b) {
        register u64 black,white,empty;

        empty = ((~(b->black | b->white)) & POTMOB);

	if(empty == 0) return (0);

	black = (empty & (N(b->black)  | E(b->black)  | S(b->black)  | W(b->black)  |
                  NE(b->black) | SE(b->black) | SW(b->black) | NW(b->black)));

        white = (empty & (N(b->white)  | E(b->white)  | S(b->white)  | W(b->white)  |
                  NE(b->white) | SE(b->white) | SW(b->white) | NW(b->white)));

        return (numbits(white) - numbits(black));
}

int get_parity(board *b) {
	if(((b->half_move % 2 == 0) && (b->color_to_move == BLACK)) || 
	   ((b->half_move % 2 != 0) && (b->color_to_move == WHITE))) {
		return (1);
	} else {
		return (-1);
	}
}



void init_evaluation(char *corner, char *edge) {
#ifdef DEBUG
	printf("Loading eval tables...\n");
	fflush(stdout);
#endif
	corner3x3  = load(corner, 18, 4);  /* 26444 - 19614 18<->265000 */
	edge1x     = load(edge, 17, 4);      /* 19382 - 13930 */

#ifdef DEBUG
	printf("done!\n");
#endif
}


static hash_table *load(char *filename, int size, int n) {
	FILE *       inputfile;
	hash_table * table;
	int          i;

	if((inputfile = fopen(filename, "rb")) != NULL) {
		int num;
		
		table = hash_table_create(size);

		fread(&num, 1, sizeof(int), inputfile);

#ifdef DEBUG
		printf("  %s including %d patterns\n", filename, num);
#endif

		for(i=0; i<num; i++) {
			u64 black,white;
			int stage,j;
			float eval;

			fread(&black, 1, sizeof(u64), inputfile);
			fread(&white, 1, sizeof(u64), inputfile);
			fread(&stage, 1, sizeof(int), inputfile);
			fread(&eval,  1, sizeof(float), inputfile);

			for(j=0; j<n; j++) {
				hash_table_add(table, black, white, stage, eval);

				black = rotate_bit_pattern(black);
				white = rotate_bit_pattern(white);
			}
		}
		fclose(inputfile);
	} else {
		printf("File not found: %s\n", filename);
		exit(1);
	}
	
	return (table);
}


