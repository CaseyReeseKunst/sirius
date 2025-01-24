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
CVSID("$Id: book.c,v 1.7 2003/02/10 08:02:51 ohman Exp $");

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "sirius.h"
#include "hashtable.h"
#include "board.h"

hash_table *book = NULL;
static hash_table *load(char *filename, int size);


void init_openingbook(char *filename) {
#ifdef DEBUG
	printf("Loading opening book...");
	fflush(stdout);
#endif

	book = load(filename, 12);  /* 19614 */

#ifdef DEBUG
	printf(" done\n");
#endif
}


int openingbook_lookup(board *b) {
	u64 black,white;
	int value;
	int i,j;
	
	if(book == NULL) {
		return (0);
	}

	black = b->black;
	white = b->white;
	for(j=0; j<2; j++) {
		for(i=0; i<4; i++) {
			if((value = (int) hash_table_find(book, black, white, b->half_move)) > 0) {
				u64 mask = ((u64)1 << (value-1));
				int j;
				
				if(i>0) {
					for(j=i; j<4; j++) {
						mask = rotate_bit_pattern(mask);
					}
				}
				
				for(j=0; j<64; j++) {
					if(mask & ((u64)1 << (j-1))) {
						return (j);
					}
				}
			}
			black = rotate_bit_pattern(black);
			white = rotate_bit_pattern(white);
		}
		black = trans_bit_pattern(black);
		white = trans_bit_pattern(white);
	}

	
	return (0);
}



static hash_table *load(char *filename, int size) {
	hash_table * table;
	FILE *       inputfile;
	int          i;

	if((inputfile = fopen(filename, "rb")) != NULL) {
		int num;
		
		table = hash_table_create(size);

		fread(&num, 1, sizeof(int), inputfile);

		for(i=0; i<num; i++) {
			u64 black,white;
			unsigned int stage,value;

			fread(&black, 1, sizeof(u64), inputfile);
			fread(&white, 1, sizeof(u64), inputfile);
			fread(&stage, 1, sizeof(int), inputfile);
			fread(&value, 1, sizeof(int), inputfile);

			hash_table_add(table, black, white, stage, (float)(value));
		}
		fclose(inputfile);
	} else {
		printf("File not found: %s\n", filename);
		exit(1);
	}
	
	return (table);
}
