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

#ifndef __HASHTABLE_H
#define __HASHTABLE_H

#include "sirius.h"

typedef struct _eval_context {
	ub4   hashvalue;
        float eval;
} eval_context;

typedef struct _hash_node {
        struct _hash_node *  nextentry;
        eval_context *       obj;
} hash_node;

typedef struct _hash_table{
	hash_node **         head;
	int                  size;
} hash_table;


hash_table* hash_table_create(int size);
float hash_table_find(hash_table *root, u64 black, u64 white, int stage);
void hash_table_add(hash_table *root, u64 black, u64 white, int stage, float eval);

ub4 hash(register ub4 *k, register ub4 length, register ub4 initval);



void print_stat();

#endif
