#ifndef _SUBDIV_H_
#define _SUBDIV_H_

#include <libgte.h>
#include <stdlib.h>

extern void clearOT();
extern void drawOT();

extern void initPolys();
extern void drawQuad(u_long *ot, SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3);

#endif /* _SUBDIV_H_ */