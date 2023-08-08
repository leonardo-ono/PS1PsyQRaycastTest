
#include <libetc.h>
#include <libgpu.h>
#include <libgte.h>
#include <stdlib.h>

#include "tiles.h"

DISPENV disp[2];
DRAWENV draw[2];
int db = 0;
int vsyncTime = 0;

#define OT_LEN 32768
u_long ot[OT_LEN];



TIM_IMAGE image;
u_short tpage;

void loadTexture()
{
    OpenTIM((u_long*) tiles);
    ReadTIM(&image);
    tpage = getTPage(image.mode & 3, 3, image.prect->x, image.prect->y);
    LoadImage(image.prect, (u_long*) image.paddr);
    DrawSync(0);
}



VECTOR playerPosition = { 980, 0, 1925 };
SVECTOR playerDirection = { 0, 2700, 0 };

int sin(int n)
{
    n = n % 4096;
    n = n < 0 ? n + 4096 : n;
    int q = n / 1024;
    int sign = q > 1 ? -1 : 1;
    if ((q % 2) == 0)
    {
        return sign * rsin(n % 1024);
    }
    return sign * rsin(1024 - (n % 1024));
}

int cos(int n)
{
    return sin(n + 1024);
}

int step = 0;
void movePlayer(long speed, short strafe)
{
    long vz = (speed * cos(playerDirection.vy + strafe)) >> 12;
    long vx = (speed * sin(playerDirection.vy + strafe)) >> 12;
    playerPosition.vx += vx;
    playerPosition.vz += vz;
    step += 256;
}

void update()
{
    long speed = 16;
    long padInfo = PadRead(0);

    if (padInfo & _PAD(0,PADLup))
    {
        FntPrint("UP\n");
        //playerPosition.vz += speed;
        movePlayer(speed, 0);
    }

    if (padInfo & _PAD(0,PADLdown))
    {
        FntPrint("DOWN\n");
        //playerPosition.vz -= speed;
        movePlayer(-speed, 0);
    }

    if (padInfo & _PAD(0,PADLleft))
    {
        FntPrint("LEFT\n");
        playerDirection.vy -= 64;
    }

    if (padInfo & _PAD(0,PADLright))
    {
        FntPrint("RIGHT\n");
        playerDirection.vy += 64;
    }

    if (padInfo & _PAD(0,PADL1))
    {
        FntPrint("PADL1\n");
        movePlayer(-speed, 1024);
    }

    if (padInfo & _PAD(0,PADR1))
    {
        FntPrint("PADR1\n");
        movePlayer(speed, 1024);
    }

    // look up down
    if (padInfo & _PAD(0,PADL2))
    {
        FntPrint("PADL1\n");
        playerDirection.vx -= 64;
    }

    if (padInfo & _PAD(0,PADR2))
    {
        FntPrint("PADR1\n");
        playerDirection.vx += 64;
    }

    dumpVector("PLAYER POSITION", &playerPosition);
    dumpVector("PLAYER DIRECTION", &playerDirection);
}

CVECTOR colors[4] = {
    { 255,   0,   0 },
    {   0, 255,   0 },
    { 255,   0, 255 },
    {   0, 255, 255 },
};

short faceAngles[4] = { 0, 1024, 2048, 3072 };

#define mapRows 15
#define mapCols 15
int map[mapRows][mapCols] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,1,1,0,0,0,1},
    {1,0,1,0,0,1,1,1,0,1,1,0,1,0,1},
    {1,0,1,0,0,0,0,0,0,0,0,0,1,0,1},
    {1,0,1,0,0,0,0,0,0,0,0,0,1,0,1},
    {1,0,1,0,0,1,0,0,0,1,1,0,1,0,1},
    {1,0,0,0,0,1,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,1,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,1,1,0,0,0,1},
    {1,1,1,0,0,0,0,1,1,1,1,1,1,0,1},
    {1,1,1,0,0,0,0,1,1,1,1,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,1,0,0,1,1,1,0,0,0,0,1,0,1},
    {1,0,1,0,0,0,0,0,0,0,0,0,1,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
};

long maxOTZ;
POLY_F4 polys[1024];
POLY_FT4 polysTex[1024];

void render()
{
    long dy = 16 + ((8 * cos(step) + 8 * sin(step)) >> 12);

    ClearOTagR(&ot[0], OT_LEN);

    // view transformation 

    SVECTOR cameraRotation = { -playerDirection.vx, -playerDirection.vy, -playerDirection.vz };
    VECTOR cameraTranslation = { -playerPosition.vx, -playerPosition.vy, -playerPosition.vz };
    MATRIX cameraMatrix;

    RotMatrixZXY(&cameraRotation, &cameraMatrix);
    ApplyMatrixLV(&cameraMatrix, &cameraTranslation, &cameraTranslation);
    TransMatrix(&cameraMatrix, &cameraTranslation);

    // model transformation 
    
    SVECTOR vertices[4] = {
        { -128, -128, 0 },
        {  128, -128, 0 },
        { -128,  128, 0 },
        {  128,  128, 0 }
    };
    
    maxOTZ = 0;
    int polyIndex = 0;

    for (int row = 0; row < mapRows; row++) 
    {
        for (int col = 0; col < mapCols; col++) 
        {
            
            if (map[row][col] == 0)
            {

                // draw floor and ceiling
                for (int fc = 0; fc < 2; fc++)
                {
                    short yoffset[2] = { 128, -128 };
                    short xangle[2] = { -1024, 1024 };
                    SVECTOR modelRotation = { xangle[fc], 0, 0 };
                    VECTOR modelTranslation = { col * 256, dy + yoffset[fc], row * 256 };
                    MATRIX modelMatrix;

                    RotMatrixYXZ(&modelRotation, &modelMatrix); // YXZ
                    TransMatrix(&modelMatrix, &modelTranslation);

                    // combine view model matrix

                    MATRIX viewModelMatrix;
                    CompMatrix(&cameraMatrix, &modelMatrix, &viewModelMatrix);

                    SetRotMatrix(&viewModelMatrix);
                    SetTransMatrix(&viewModelMatrix);
                    
                    long p;
                    long flag;
                    long otz;
                    //POLY_F4 poly;
                    setPolyFT4(&polysTex[polyIndex]);
                    long nclip = RotAverageNclip4(&vertices[0], &vertices[1], &vertices[2], &vertices[3]
                                , (long*) &(polysTex[polyIndex].x0), (long*) &(polysTex[polyIndex].x1), (long*) &(polysTex[polyIndex].x2), (long*) &(polysTex[polyIndex].x3), &p, &otz, &flag);

                    if (nclip <=0 || otz <= 0)
                    {
                        continue;
                    }
                    if (otz > maxOTZ)
                    {
                        maxOTZ = otz;
                    }

                    setRGB0(&polysTex[polyIndex], 128, 128, 128);


                    if (fc == 0) {
                        setRGB0(&polysTex[polyIndex], 255, 255, 255);
                    }

                    setUV4(&polysTex[polyIndex], 1, 65, 63, 65, 1, 127, 63, 127);
                    polysTex[polyIndex].tpage = tpage;
                    //DrawPrim(&poly);
                    //DrawSync(0);
                    AddPrim(&ot[OT_LEN - 2], &polysTex[polyIndex]);

                    polyIndex++;
                }

                continue;
            }

            for (int f = 0; f < 4; f++)
            {
                int tx[4] = { 0, -128, 0, 128 }; 
                int tz[4] = { -128, 0, 128, 0 };
                SVECTOR modelRotation = { 0, faceAngles[f], 0 };
                VECTOR modelTranslation = { col * 256 + tx[f], dy, row * 256 + tz[f] };
                MATRIX modelMatrix;

                RotMatrixYXZ(&modelRotation, &modelMatrix); // YXZ
                TransMatrix(&modelMatrix, &modelTranslation);

                // combine view model matrix

                MATRIX viewModelMatrix;
                CompMatrix(&cameraMatrix, &modelMatrix, &viewModelMatrix);

                SetRotMatrix(&viewModelMatrix);
                SetTransMatrix(&viewModelMatrix);
                
                long p;
                long flag;
                long otz;
                //POLY_F4 poly;
                setPolyFT4(&polysTex[polyIndex]);
                long nclip = RotAverageNclip4(&vertices[0], &vertices[1], &vertices[2], &vertices[3]
                            , (long*) &(polysTex[polyIndex].x0), (long*) &(polysTex[polyIndex].x1), (long*) &(polysTex[polyIndex].x2), (long*) &(polysTex[polyIndex].x3), &p, &otz, &flag);

                if (nclip <=0 || otz <= 0)
                {
                    continue;
                }
                if (otz > maxOTZ)
                {
                    maxOTZ = otz;
                }

                setRGB0(&polysTex[polyIndex], 192, 192, 192);

                if (f == 0 || f == 2) {
                    setRGB0(&polysTex[polyIndex], 64, 64, 64);
                }

                setUV4(&polysTex[polyIndex], 0, 0, 64, 0, 0, 64, 64, 64);
                polysTex[polyIndex].tpage = tpage;
                //DrawPrim(&poly);
                //DrawSync(0);
                AddPrim(&ot[otz], &polysTex[polyIndex]);

                polyIndex++;
            }

        }
    }

    DrawOTag2(&ot[OT_LEN - 1]);
    FntPrint("poly count = %d \n", polyIndex);
    FntPrint("maxOTZ = %d \n", maxOTZ);
}

int main()
{
    ResetGraph(0);
    loadTexture();
    PadInit(0);
    InitGeom();
    SetGeomOffset(160, 120);
    SetGeomScreen(160);

    SetDefDispEnv(&disp[0], 0, 0, 320, 240);
    SetDefDispEnv(&disp[1], 0, 240, 320, 240);
    SetDefDrawEnv(&draw[0], 0, 240, 320, 240);
    SetDefDrawEnv(&draw[1], 0, 0, 320, 240);
    draw[0].isbg = 1;            
    draw[1].isbg = 1;
    setRGB0(&draw[0], 0, 0, 0);
    setRGB0(&draw[1], 0, 0, 0);
    SetDispMask(1);

    FntLoad(960, 0);
    SetDumpFnt(FntOpen(32, 32, 256, 64, 0, 1024));

    while (1)
    {
        update();
        render();

        FntPrint("HSYNC = %d", vsyncTime);
        FntFlush(-1);
        DrawSync(0);
        vsyncTime = VSync(0);
        db ^= 1;
        PutDispEnv(&disp[db]);
        PutDrawEnv(&draw[db]);
    }

    return 0;
}