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

#ifndef __BOARD_H
#define __BOARD_H 

board * init_board(board *b, int black_time, int white_time);
void    deinit_board(board *b);

board * load_board(board *b, testboard tb);

void    legal_moves(board *b, int bestmove);
board * do_move(board *b, int move, undo_info *ui);
board * do_pass(board *b, undo_info *ui);
board * undo_move(board *b, undo_info *ui);

void    mobility(board *b);
int     pos(board *b, int m);
int     legal(board *b, int move);
void    dump(board *b);
void    print_bit_pattern(u64 b);
u64     rotate_bit_pattern(u64 orig);
u64     trans_bit_pattern(u64 orig);
int     numbits(u64 orig);
char *  int2pos(int move);
char *  int2pos_mirror(int move);
void    clean_transpositiontable(board *b);

#endif
