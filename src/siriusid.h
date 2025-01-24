#ifndef __SIRIUSID_H
#define __SIRIUSID_H 
#ifndef NO_CVSID
#define CVSID(s) static char cvsid[] __attribute__ ((unused)) = (s)
#else
#define CVSID(s) 
#endif
#endif
