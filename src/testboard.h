/*
 *  Copyright (C) 2002-2003 Henrik �hman
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
 *  Author: Henrik �hman <henrik@bitvis.nu>
 *
 */


#ifndef __TESTBOARD_H
#define __TESTBOARD_H

#include <sirius.h>

#define TESTSUITESIZE 6


testboard ffotest[TESTSUITESIZE] = {
	{
		2,0,0,0,0,2,2,2,
		2,2,1,1,1,0,0,2,
		2,2,2,1,1,2,0,2,
		2,2,1,0,0,0,0,2,
		2,1,2,1,1,2,2,2,
		2,2,2,2,1,2,2,2,
		2,2,2,2,2,2,2,2,
		2,2,2,2,2,2,2,2,0
	},
	{
		0,2,2,0,0,0,0,1 ,
		2,0,0,0,0,0,0,1 ,
		0,0,1,1,0,0,0,1 ,
		0,0,1,0,0,0,1,1 ,
		0,0,0,0,0,0,1,1 ,
		2,2,2,0,0,0,0,1 ,
		2,2,2,2,0,2,2,1 ,
		2,2,2,2,2,2,2,2 ,1
	},

	{
		2,0,0,0,0,0,2,2 ,
		2,2,0,0,0,0,1,2 ,
		2,0,0,0,0,0,0,2 ,
		1,1,1,1,1,0,0,2 ,
		2,1,1,0,0,1,2,2 ,
		0,0,1,0,1,1,2,2 ,
		2,2,0,1,1,0,2,2 ,
		2,0,0,0,2,2,0,2 ,1
	},
	
	{
		2,2,0,0,0,2,2,2 ,
		2,2,2,2,1,1,2,0 ,
		0,0,0,0,0,1,0,0 ,
		2,0,0,0,0,1,0,0 ,
		1,2,0,0,0,1,1,0 ,
		2,2,2,0,0,1,0,0 ,
		2,2,2,0,0,0,1,0 ,
		2,2,0,0,0,0,2,2 ,1
	},
	
	{
		2,2,1,1,1,1,1,2 ,
		2,2,1,1,1,1,2,2 ,
		2,0,0,0,1,1,2,2 ,
		2,0,0,1,1,1,1,2 ,
		2,0,0,1,1,1,0,2 ,
		0,0,0,0,1,0,0,2 ,
		2,2,2,1,0,1,2,2 ,
		2,2,1,1,1,1,1,2 ,0
	},

	{
		2,2,0,2,1,2,0,2 ,
		2,2,0,2,1,0,2,0 ,
		2,0,0,1,1,1,0,0 ,
		0,0,0,0,1,1,1,0 ,
		0,0,0,0,1,1,2,2 ,
		1,1,0,0,1,0,2,2 ,	     
		2,2,1,1,1,1,2,2 ,
		2,2,2,1,1,1,2,2 ,0
	}
};


#endif
