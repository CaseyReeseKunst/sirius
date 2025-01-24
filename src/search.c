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
CVSID("$Id: search.c,v 1.12 2003/06/17 07:18:00 ohman Exp $");

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "sirius.h"
#include "board.h"
#include "search.h"
#include "timer.h"
#include "evaluation.h"
#include "book.h"

#ifdef DEBUG
long num, numhash, prun, numtot, timecnt;
#endif

static int alphabeta(board *b, int depth, int alpha, int beta);
static int rootsearch(board *b, int maxdepth);


int computer_move(board *b, int fixply, int fixtime, int usebook) {
	int move, maxdepth = 0;

#ifdef DEBUG
	if(b->color_to_move == BLACK) {
		printf("\nBLACK SEARCH #%d:\n", b->half_move);
	} else {
		printf("\nWHITE SEARCH #%d:\n", b->half_move);
	}
	numtot = 0;
	timecnt   = b->time_left[b->color_to_move];
#endif

	start_timer(fixtime, b->color_to_move);

	legal_moves(b,0);

	if(b->pass || b->game_over) {
		return (0);
	}

	/* openingbook lookup */
	if(usebook && b->half_move <= 9) {
		int val;
		if((val = openingbook_lookup(b)) != 0) {
			int i;
			
			/* make sure this is a legel move */
			for(i=0; i<b->num_legal_moves; i++) {
				if(b->legal_move[i] == val) {
					stop_timer(b);
#ifdef DEBUG
					printf("opening book lookup: %s\n", int2pos(val));
#endif				
					return (val);
				}
			}
		}
	}        

	if(b->num_legal_moves > 1) {
		if((b->half_move < 10) && (fixply == 0)) {
			maxdepth = 9 - b->half_move;
		} else {
			maxdepth = 0;
		}

		do {
			move = rootsearch(b, ++maxdepth);

			if(fixply == 0) {
				if(!more_time(b)) break;
			} else {
				if(fixtime == 0) {
					if(maxdepth >= fixply) break;
				} else {
					if((!more_time(b)) || (maxdepth >= fixply)) break;
				}
			}

	        } while(b->half_move + maxdepth < 61);
	} else {
		move = b->legal_move[0];
	}
	
	stop_timer(b);
	
#ifdef DEBUG
	timecnt -= b->time_left[b->color_to_move];
	printf("time used: %ld and total %ld nodes searched\n", timecnt, numtot);
	if(timecnt>0) printf("k nodes per sec: %ld\n", (numtot/timecnt)); 
#endif	

	return (move);
}



static int rootsearch(board *b, int maxdepth) {
	undo_info ui;
	int eval, sec_eval;
	int foundmove, sec_move;
	int diff;
	short int *move;

	
#ifdef DEBUG
	num     = 0;
        numhash = 0;
	prun    = 0;
#endif
	
	foundmove = 0;
	sec_move = 0;
	eval = -INFINITY;
	sec_eval = -INFINITY;

/*	for(i=0; i<b->num_legal_moves; i++) { */
	move = (short int *) &(b->legal_move);
	while((*move) != -1) {
		int score;
	/*	int m = b->legal_move[i]; */

		pthread_testcancel();

		b = do_move(b, (*move), &ui);
		score = -alphabeta(b, maxdepth - 1, -INFINITY, INFINITY);  
		b = undo_move(b, &ui);
		
		if(score > eval) {
			sec_move = foundmove;
			sec_eval = eval;
			
			eval = score;
			foundmove = (*move);
		}
		*move++;
	}
	
	/* add some randomness */
	diff = eval - sec_eval;
	if(b->half_move < 45 && diff < 5 && diff > 0) {
		int ran = (int)(10.0*rand()/(RAND_MAX+1.0));
		if(ran < 5) {
			eval = sec_eval;
			foundmove = sec_move;
		}
	}
	
#ifdef DEBUG
	numtot += num; 
	
	printf("depth: %d  \tnode searched: %ld  \tpruned: %ld \t\thash retrieve: %ld  \tmove: %s  \teval: %d\n", maxdepth,num,prun,numhash, int2pos(foundmove), eval);
#endif

	return (foundmove);
}


static int alphabeta(board *b, int depth, int alpha, int beta) {
	transpositiontable * t;
        undo_info            ui;
	int                  exact_eval;
	int                  bestmove = 0;
	int 	             score;
	short int             *move;
	
#ifdef DEBUG
        num++;
#endif

	t = &(b->tt[b->x]);
	
	/* hashtable lookup */
	if((t->checksum == b->y) && (t->depth == depth)) {
#ifdef DEBUG
                numhash++;
#endif

		switch(t->entry_type) {
			case exact : return (t->eval);
			case lower_bound : {
				if(t->eval >= beta) {
	                                return (t->eval);
        	                }
				break;
			}
			case upper_bound : {
				if(t->eval <= alpha) {
                                	return (t->eval);
				}
				break;
			}
		}

		bestmove = t->best;
		
#ifdef DEBUG
                numhash--;
#endif
        }

	if(depth <= 0) {
		int eval;

		if(b->half_move >= 58) {
			eval = (numbits(b->black) - numbits(b->white));

	  		if(b->color_to_move == BLACK) {
				return (eval);
	 		} else {
				return (-eval);
	   		}
		} else {
			eval = (b->color_to_move == BLACK) ? evaluate(b) : -evaluate(b);

	                return (eval);
		} 
	}

	legal_moves(b, bestmove);
	if(b->num_legal_moves == 0) {
		if(b->game_over) {
			int eval = (numbits(b->black) - numbits(b->white));

			if(b->color_to_move == BLACK) {
                                return (eval);
                        } else {
                                return (-eval);
                        }
		}

		do_pass(b, &ui);
		return (-alphabeta(b, depth, -beta, -alpha));
	}

	exact_eval = 0;
	bestmove   = 0;
/*	for(i=0; i<b->num_legal_moves; i++) {  */
	move = (short int *) &(b->legal_move);
	while((*move) != -1) {
		b = do_move(b, (*move), &ui);
   		score = -alphabeta(b, depth - 1, -beta, -alpha);
   		b = undo_move(b, &ui);

		if (score >= beta) {
                        t->checksum   = b->y;
                        t->depth      = depth;
                        t->entry_type = lower_bound;
                        t->eval       = score;
			t->best       = (*move);
#ifdef DEBUG
			/*prun += (b->num_legal_moves - i);*/
#endif
                        return (score);
                }
                if (score > alpha) {
                        alpha      = score;
                        exact_eval = 1;
			bestmove = move - b->legal_move; 
                }
		*move++;
	}

	if (exact_eval == 1) {
		t->entry_type = exact;
	} else {
		t->entry_type = upper_bound;
	}
	t->checksum = b->y;
	t->depth    = depth;
	t->best     = b->legal_move[bestmove];
	t->eval     = alpha;
		
	return (alpha);
}





