#ifndef _SUBDIV_H_
#define _SUBDIV_H_

#include <libgte.h>
#include <stdlib.h>

#define WALL_H 1
#define WALL_V 2
#define FLOOR  3
#define CEIL   4

extern void clearOT();
extern void drawOT();

extern void drawQuad(int faceType, u_long *ot, SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3);
extern void initPolys(u_short tpage);


#endif /* _SUBDIV_H_ */