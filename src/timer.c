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
CVSID("$Id: timer.c,v 1.7 2003/06/17 07:18:00 ohman Exp $");

#include <sys/time.h>
#include <stdio.h>
#include <glib.h>

#include "sirius.h"

GTimer *timer = NULL;
long 	fix;
int     time_color;


long get_time_left(board *b, int color) {
	long t = b->time_left[color];
	if(timer != NULL && time_color == color) {
	        double elaps = 0;

		elaps = g_timer_elapsed(timer, NULL);
		elaps *= 1000;
		t -= (long)elaps;
	}
		
	return (t);
}


void start_timer(long fixtime, int color) {
	fix        = fixtime;
	time_color = color;
	if(timer == NULL) {
		timer = g_timer_new();
	} else {
		g_timer_start(timer);
	}
}


void stop_timer(board *b) {
	double elaps;

	g_timer_stop(timer);
	elaps = g_timer_elapsed(timer, NULL);
	elaps *= 1000;
	b->time_left[b->color_to_move] -= (long)elaps; 
	g_timer_reset(timer);
}

int more_time(board *b) {
        double sec;
	long time_to_use;
	
	sec = g_timer_elapsed(timer, NULL);
	sec *= 1000;
	
	if(fix > 0) {
		time_to_use = fix;
	} else {
		if(b->half_move < 45) {
			time_to_use = b->time_left[b->color_to_move] / (60 - b->half_move);
		} else {
			time_to_use = b->time_left[b->color_to_move] / 8;
		}
	}

	if (((long)(sec)) > time_to_use) {
		return (0);
	} else {
		return (1);
	}
}


void deinit_timer() {
	g_timer_destroy(timer);
	timer = NULL;
}


