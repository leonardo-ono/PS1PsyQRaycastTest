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

#define POLYS_COUNT 4000

POLY_FT4 polys[POLYS_COUNT];
SVECTOR vvm01[POLYS_COUNT];
SVECTOR vvm02[POLYS_COUNT];
SVECTOR vvm03[POLYS_COUNT];
SVECTOR vvm12[POLYS_COUNT];
SVECTOR vvm32[POLYS_COUNT];

SVECTOR stm01[POLYS_COUNT];
SVECTOR stm02[POLYS_COUNT];
SVECTOR stm03[POLYS_COUNT];
SVECTOR stm12[POLYS_COUNT];
SVECTOR stm32[POLYS_COUNT];


void initPolys(u_short tpage)
{
    for (int i = 0; i < POLYS_COUNT; i++)
    {
        setPolyFT4(&polys[i]);
        setRGB0(&polys[i], 128, 128, 128); //&polys[i], rand() % 255, rand() % 255, rand() % 255);
        setSemiTrans(&polys[i], 1);
        polys[i].tpage = tpage;
        //setUV4(&polys[i], 0, 0, 64, 0, 0, 64, 64, 64);
        //setUV4(&polys[i], st0->vx, st0->vy, st1->vx, st1->vy, st2->vx, st2->vy, st3->vx, st3->vy);
    }
}

int pi;
const int k[3] = { 280, 200, 120 };

static void drawQuadRec(int faceType, u_long *ot, int level, SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, SVECTOR *st0, SVECTOR *st1, SVECTOR *st2, SVECTOR *st3)
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
    
    // h/2
    u_long f = ((u_long) flag) & 0b1000000000000000000;

    if (nclip <=0 || (level >= 2 && flag < 0)) //&& otz <= 0))
    {
        return;
    }


    if (level >= 2 || otz >= k[level])
    {
        int uv = 65 >> level;
        //setUV4(&polys[pi], 0, 0, uv, 0, 0, uv, uv, uv);
        setUV4(&polys[pi], st0->vx, st0->vy, st1->vx, st1->vy, st3->vx, st3->vy, st2->vx, st2->vy);
        if (faceType == WALL_V)
        {
            setRGB0(&polys[pi], 164, 164, 164);
        }
        else if (faceType == WALL_H)
        {
            setRGB0(&polys[pi], 92, 92, 92);
        }
        else if (faceType == CEIL)
        {
            setRGB0(&polys[pi], 48, 48, 48);
        }
        else 
            setRGB0(&polys[pi], 32, 32, 32);
        {

        }
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


        SVECTOR *sstm01 = &stm01[pi];
        SVECTOR *sstm02 = &stm02[pi];
        SVECTOR *sstm03 = &stm03[pi];
        SVECTOR *sstm12 = &stm12[pi];
        SVECTOR *sstm32 = &stm32[pi];
        sstm01->vx = (st0->vx + st1->vx) >> 1;
        sstm01->vy = (st0->vy + st1->vy) >> 1;
        sstm02->vx = (st0->vx + st2->vx) >> 1;
        sstm02->vy = (st0->vy + st2->vy) >> 1;
        sstm03->vx = (st0->vx + st3->vx) >> 1;
        sstm03->vy = (st0->vy + st3->vy) >> 1;
        sstm12->vx = (st1->vx + st2->vx) >> 1;
        sstm12->vy = (st1->vy + st2->vy) >> 1;
        sstm32->vx = (st3->vx + st2->vx) >> 1;
        sstm32->vy = (st3->vy + st2->vy) >> 1;
        drawQuadRec(faceType, ot, level + 1, v0, vm01, vm02, vm03, st0, sstm01, sstm02, sstm03);
        drawQuadRec(faceType, ot, level + 1, vm01, v1, vm12, vm02, sstm01, st1, sstm12, sstm02);
        drawQuadRec(faceType, ot, level + 1, vm03, vm02, vm32, v3, sstm03, sstm02, sstm32, st3);
        drawQuadRec(faceType, ot, level + 1, vm02, vm12, v2, vm32, sstm02, sstm12, st2, sstm32);
    }
}

void drawQuad(int faceType, u_long *ot, SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3)
{
    SVECTOR st0 = { 0, 0, 0 };
    SVECTOR st1 = { 64, 0, 0 };
    SVECTOR st2 = { 64, 64, 0 };
    SVECTOR st3 = { 0, 64, 0 };
    drawQuadRec(faceType, ot, 0, v0, v1, v2, v3, &st0,  &st1,  &st2,  &st3);
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
