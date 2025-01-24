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
CVSID("$Id: hashtable.c,v 1.3 2003/01/14 12:28:59 ohman Exp $");

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sirius.h"
#include "hashtable.h"

#define hashsize(n) ((ub4)1<<(n))
#define hashmask(n) (hashsize(n)-1)
#define mix(a,b,c) \
{ \
  a -= b; a -= c; a ^= (c>>12); \
  b -= c; b -= a; b ^= (a<<16); \
  c -= a; c -= b; c ^= (b>>5);  \
}

ub4 hash(register ub4 *k, register ub4 length, register ub4 initval) {
	register ub4 a,b,c,len;
	
	len = length;
	a = b = 0x9e3779b9;
	c = initval;

	while (len >= 3) {
		a += k[0];
		b += k[1];
		c += k[2];
		mix(a,b,c);
		k += 3; len -= 3;
	}

	c += length;
	switch(len) {
		case 2 : b+=k[1];
		case 1 : a+=k[0];
	}
	mix(a,b,c);

	return (c);
}

static hash_node* find(hash_node* root, unsigned long hashvalue) {
	hash_node *  curr;

	curr = root;

	while (curr != NULL){
		if (hashvalue == curr->obj->hashvalue) {
			return (curr);
		}
		curr = curr->nextentry;
	}

	return (NULL);
}

static hash_table *create_symtab(unsigned int size, unsigned int logsize){
	int                  i;
	hash_table*          newhashtable;
	hash_node **         head;

	newhashtable = (hash_table *) malloc(sizeof(hash_table));
	head         = (hash_node **) malloc(size * sizeof(hash_node));

	newhashtable->size = logsize;
	newhashtable->head = head;

	for (i=0; i<size; i++) {
		newhashtable->head[i] = NULL;
	}

	return (newhashtable);
}


hash_table *hash_table_create(int size){
	hash_table *  ret;
	ub4 len = ((ub4)1<<size);

	ret = create_symtab(len,size);
	return (ret);
}


float hash_table_find(hash_table *root, u64 black, u64 white, int stage){
	hash_node *    se;
	unsigned int   index;
	unsigned long  hashvalue;     
	u64            key[3];

	key[0] = black;
	key[1] = white;
	key[2] = stage;
	
	hashvalue = hash((ub4*)&key, 6, stage);
	index     = (hashvalue & hashmask(root->size));
	
	se = find(root->head[index], hashvalue);
	if (se == NULL) {
		return (0);
	} else {
 		return (se->obj->eval);
	}
}

void hash_table_add(hash_table *root, u64 black, u64 white, int stage, float eval) {
	hash_node *    se;
	eval_context * ec;
	unsigned int   index;
	unsigned long  hashvalue;
	u64            key[3];
	
	key[0] = black;
	key[1] = white;
	key[2] = stage;
	
	hashvalue = hash((ub4*)&key, 6, stage); 
	index     = (hashvalue & hashmask(root->size));
	
	ec = (eval_context *) malloc(sizeof(eval_context));
	ec->hashvalue = hashvalue;  
	ec->eval      = eval;

	se = find(root->head[index], hashvalue);
	if(se == NULL) {
		se = (hash_node *) malloc(sizeof(hash_node));
		se->obj           = ec;
		se->nextentry     = root->head[index];
		root->head[index] = se;
	} else {
		se->obj = ec;
	}
}
















