#include <libgpu.h>

#include "subdiv.h"

#define OOT_LEN 32768
u_long oot[OOT_LEN];


static void drawWireframeQuad(POLY_F4 *poly)
{
    LINE_F2 line[4];
    short points[8] = { poly->x0, poly->y0, poly->x1, poly->y1, poly->x3, poly->y3, poly->x2, poly->y2 };
    for (int i = 0; i < 4; i++)
    {
        setLineF2(&line[i]);
        setRGB0(&line[i], poly->r0, poly->g0, poly->b0);
        int x0 = points[(2 * i + 0) % 8];
        int y0 = points[(2 * i + 1) % 8];
        int x1 = points[(2 * i + 2) % 8];
        int y1 = points[(2 * i + 3) % 8];
        setXY2(&line[i], x0, y0, x1, y1);
        DrawPrim(&line[i]);
    }
    DrawSync(0);
}

#define POLYS_COUNT 24000

POLY_F4 polys[POLYS_COUNT];
SVECTOR vvm01[POLYS_COUNT];
SVECTOR vvm02[POLYS_COUNT];
SVECTOR vvm03[POLYS_COUNT];
SVECTOR vvm12[POLYS_COUNT];
SVECTOR vvm32[POLYS_COUNT];

void initPolys()
{
    for (int i = 0; i < POLYS_COUNT; i++)
    {
        setPolyF4(&polys[i]);
        setRGB0(&polys[i], rand() % 255, rand() % 255, rand() % 255);
        setSemiTrans(&polys[i], 0);
    }
}

int pi;
const int k[3] = { 280, 200, 120 };

static void drawQuadRec(u_long *ot, int level, SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3)
{
    long p;
    long flag;
    
    pi++;
    //setPolyF4(&polys[pi]);
    //setRGB0(&polys[pi], rand() % 255, rand() % 255, rand() % 255);

    //POLY_F4 poly;
    //setXY4(&polys[pi], 0, 0, 0, 0, 0, 0, 0, 0);
    long otz;
    long nclip = RotAverageNclip4(v0, v1, v2, v3, (long*) &polys[pi].x0, (long*) &polys[pi].x1, (long*) &polys[pi].x3, (long*) &polys[pi].x2, &p, &otz, &flag);
    
    if (nclip <=0 || (level >= 2 && otz <= 0))
    {
        return;
    }

    // h/2
    u_long f = ((u_long) flag) & 0b100000000000000000;

    if (level >= 2 || otz >= k[level])
    {
        AddPrim(&oot[otz], &polys[pi]);
        // drawWireframeQuad(&polys[pi]);
    }
    else if (otz < k[level])
    {
        SVECTOR *vm01 = &vvm01[pi];
        SVECTOR *vm02 = &vvm02[pi];
        SVECTOR *vm03 = &vvm03[pi];
        SVECTOR *vm12 = &vvm12[pi];
        SVECTOR *vm32 = &vvm32[pi];
        vm01->vx = (v0->vx + v1->vx) >> 1;
        vm01->vy = (v0->vy + v1->vy) >> 1;
        vm01->vz = (v0->vz + v1->vz) >> 1;
        vm02->vx = (v0->vx + v2->vx) >> 1;
        vm02->vy = (v0->vy + v2->vy) >> 1;
        vm02->vz = (v0->vz + v2->vz) >> 1;
        vm03->vx = (v0->vx + v3->vx) >> 1;
        vm03->vy = (v0->vy + v3->vy) >> 1;
        vm03->vz = (v0->vz + v3->vz) >> 1;
        vm12->vx = (v1->vx + v2->vx) >> 1;
        vm12->vy = (v1->vy + v2->vy) >> 1;
        vm12->vz = (v1->vz + v2->vz) >> 1;
        vm32->vx = (v3->vx + v2->vx) >> 1;
        vm32->vy = (v3->vy + v2->vy) >> 1;
        vm32->vz = (v3->vz + v2->vz) >> 1;

        drawQuadRec(ot, level + 1, v0, vm01, vm02, vm03);
        drawQuadRec(ot, level + 1, vm01, v1, vm12, vm02);
        drawQuadRec(ot, level + 1, vm03, vm02, vm32, v3);
        drawQuadRec(ot, level + 1, vm02, vm12, v2, vm32);
    }
}

void drawQuad(u_long *ot, SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3)
{
    drawQuadRec(ot, 0, v0, v1, v2, v3);
}

void clearOT()
{
    pi = 0;
    ClearOTagR(oot, OOT_LEN);
}

void drawOT()
{
    DrawOTag2(&oot[OOT_LEN - 1]);
}
