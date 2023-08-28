
#include <libetc.h>
#include <libgpu.h>
#include <libgte.h>
#include <stdlib.h>

#include "tiles.h"
#include "subdiv.h"
#include "enemy.h"

DISPENV disp[2];
DRAWENV draw[2];
int db = 0;

#define OT_LEN 32768
u_long ot[OT_LEN];



TIM_IMAGE image;
u_short tpage;

int framesCount;

Enemy *enemy1;
Enemy *enemy2;
Enemy *enemy3;

void loadTexture()
{
    OpenTIM((u_long*) tiles);
    ReadTIM(&image);
    tpage = getTPage(2, 3, image.prect->x, image.prect->y);
    LoadImage(image.prect, (u_long*) image.paddr);
    DrawSync(0);
}


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

#define mapRows 16
#define mapCols 16
int map[mapRows][mapCols] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
    {1,0,0,0,0,0,0,0,0,1,1,0,0,0,1,0},
    {1,0,1,0,0,1,0,1,0,1,1,0,1,0,1,0},
    {1,0,1,0,0,0,0,1,0,1,0,0,1,0,1,0},
    {1,0,0,0,0,0,0,0,0,1,0,0,1,0,1,0},
    {1,0,1,0,0,1,0,0,0,1,1,0,1,0,1,0},
    {1,0,0,0,0,0,0,0,0,1,0,0,0,0,1,0},
    {1,0,0,1,0,0,1,0,0,1,0,0,0,0,1,0},
    {1,0,0,0,0,0,0,0,0,1,1,0,0,0,1,0},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0},
    {1,1,1,0,0,0,0,1,1,1,1,1,1,0,1,0},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0},
    {1,0,1,0,0,1,1,1,0,0,0,0,1,0,1,0},
    {1,0,1,0,0,0,0,0,0,0,0,0,1,0,1,0},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};

int getMapTileId(int row, int col)
{
    if (row < 0 || col < 0 || row > mapRows - 1 || col > mapCols - 1) {
        return -1;
    }
    return map[row][col];
}

#define FACE_SIZE 720

SVECTOR vertices[mapRows][mapCols][2];

void initAllVertices()
{
    for (int row = 0; row < mapRows; row++)
    {
        for (int col = 0; col < mapCols; col++)
        {
            vertices[row][col][0].vx = col * FACE_SIZE;
            vertices[row][col][0].vz = row * FACE_SIZE;
            vertices[row][col][0].vy = -FACE_SIZE / 2;
            vertices[row][col][1].vx = col * FACE_SIZE;
            vertices[row][col][1].vz = row * FACE_SIZE;
            vertices[row][col][1].vy = FACE_SIZE / 2;
        }
    }
}

#define drawRows 10
#define drawCols 10

SVECTOR* faces[mapRows * mapCols * 2][4];
int facesType[mapRows * mapCols * 2];
int facesCount;

void initAllFaces()
{
    facesCount = 0;
    for (int row = 0; row < drawRows; row++)
    {
        for (int col = 0; col < drawCols; col++)
        {
            int tileId = getMapTileId(row, col);
            if (tileId == 1)
            {
                //horizontal
                if (getMapTileId(row - 1, col) <= 0)
                {
                    faces[facesCount][0] = &vertices[row][col + 0][0];
                    faces[facesCount][1] = &vertices[row][col + 1][0];
                    faces[facesCount][2] = &vertices[row][col + 1][1];
                    faces[facesCount][3] = &vertices[row][col + 0][1];
                    facesType[facesCount] = WALL_H;
                    facesCount++;
                }

                //vertical
                if (getMapTileId(row, col - 1) <= 0)
                {
                    faces[facesCount][0] = &vertices[row + 0][col][1];
                    faces[facesCount][1] = &vertices[row + 1][col][1];
                    faces[facesCount][2] = &vertices[row + 1][col][0];
                    faces[facesCount][3] = &vertices[row + 0][col][0];
                    facesType[facesCount] = WALL_V;
                    facesCount++;
                }
            }
            else if (tileId == 0)
            {
                //horizontal
                if (getMapTileId(row - 1, col) == 1)
                {
                    faces[facesCount][0] = &vertices[row][col + 0][1];
                    faces[facesCount][1] = &vertices[row][col + 1][1];
                    faces[facesCount][2] = &vertices[row][col + 1][0];
                    faces[facesCount][3] = &vertices[row][col + 0][0];
                    facesType[facesCount] = WALL_H;
                    facesCount++;
                }

                //vertical
                if (getMapTileId(row, col - 1) == 1)
                {
                    faces[facesCount][0] = &vertices[row + 0][col][0];
                    faces[facesCount][1] = &vertices[row + 1][col][0];
                    faces[facesCount][2] = &vertices[row + 1][col][1];
                    faces[facesCount][3] = &vertices[row + 0][col][1];
                    facesType[facesCount] = WALL_V;
                    facesCount++;
                }

                // ceil
                faces[facesCount][0] = &vertices[row + 0][col + 0][0];
                faces[facesCount][1] = &vertices[row + 0][col + 1][0];
                faces[facesCount][2] = &vertices[row + 1][col + 1][0];
                faces[facesCount][3] = &vertices[row + 1][col + 0][0];
                facesType[facesCount] = CEIL;
                facesCount++;

                // floor
                faces[facesCount][0] = &vertices[row + 0][col + 0][1];
                faces[facesCount][1] = &vertices[row + 1][col + 0][1];
                faces[facesCount][2] = &vertices[row + 1][col + 1][1];
                faces[facesCount][3] = &vertices[row + 0][col + 1][1];
                facesType[facesCount] = FLOOR;
                facesCount++;

            }
        }
    }
    
}


VECTOR playerPosition = { 1120, -80, 970 };
SVECTOR playerDirection = { 0, 0, 0 };

void movePlayer(long speed, short strafe)
{
    long vz = (speed * cos(playerDirection.vy + strafe)) >> 12;
    long vx = (speed * sin(playerDirection.vy + strafe)) >> 12;
    playerPosition.vx += vx;
    playerPosition.vz += vz;
}

void update()
{
    updateEnemy(enemy1);
    updateEnemy(enemy2);
    //updateEnemy(enemy3);
}

SVECTOR cameraRotation;
VECTOR cameraTranslation;
MATRIX cameraMatrix;

void render()
{

    long speed = 64;
    long padInfo = PadRead(0);

    if (padInfo & _PAD(0,PADLup))
    {
        FntPrint("UP\n");
        movePlayer(speed, 0);
    }

    if (padInfo & _PAD(0,PADLdown))
    {
        FntPrint("DOWN\n");
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
    
    cameraRotation.vx = -playerDirection.vx;
    cameraRotation.vy = -playerDirection.vy;
    cameraRotation.vz = -playerDirection.vz;

    cameraTranslation.vx = -playerPosition.vx;
    cameraTranslation.vy = -playerPosition.vy;
    cameraTranslation.vz = -playerPosition.vz;

    RotMatrixZXY(&cameraRotation, &cameraMatrix);
    ApplyMatrixLV(&cameraMatrix, &cameraTranslation, &cameraTranslation);
    TransMatrix(&cameraMatrix, &cameraTranslation);

    //MATRIX modelTransformation;
    //SVECTOR modelRotation = { 0, -playerDirection.vy, 0 };
    //VECTOR modelTranslation = { -(playerPosition.vx >> 8), 0, -(playerPosition.vz >> 8) };
    
    //RotMatrix(&modelRotation, &modelTransformation);
    //TransMatrix(&modelTransformation, &modelTranslation);

    SetRotMatrix(&cameraMatrix);
    SetTransMatrix(&cameraMatrix);

    //ClearOTagR(ot, OT_LEN);
    clearOT();

    for (int f = 0; f < facesCount; f++)
    {
        // SVECTOR v1 = { 0,  80, -80 };
        // SVECTOR v2 = { 0,  80,  80 };
        // SVECTOR v3 = { 0, -80,  80 };
        // SVECTOR v0 = { 0, -80, -80 };

        SVECTOR *v0 = faces[f][0];
        SVECTOR *v1 = faces[f][1];
        SVECTOR *v2 = faces[f][2];
        SVECTOR *v3 = faces[f][3];
        drawQuad(facesType[f], ot, v0, v1, v2, v3); 
    }

    #define OOT_LEN 32768
    extern u_long oot[OOT_LEN];

    renderEnemy(oot, OOT_LEN, tpage, &cameraMatrix, enemy1, 0);
    //renderEnemy(oot, OOT_LEN, tpage, &cameraMatrix, enemy2, 1000);
    //renderEnemy(oot, OOT_LEN, tpage, &cameraMatrix, enemy3, 2000);
    
    //DrawOTag2(&ot[OT_LEN - 1]);
    drawOT();

    //FntPrint("objZ = %d\n", objZ);    
    //FntPrint("flag = %d\n", flag);
}

int main()
{

    ResetGraph(0);
    
    SetFogNearFar(1000, 4000, 160);
    SetFarColor(32, 32, 32);

    loadTexture();

    initPolys(tpage);
    initAllVertices();
    initAllFaces();

    PadInit(0);

    InitGeom(0);
    SetGeomOffset(160, 120);
    SetGeomScreen(160);

    SetDefDispEnv(&disp[0], 0, 0, 320, 240);
    SetDefDispEnv(&disp[1], 0, 240, 320, 240);
    SetDefDrawEnv(&draw[0], 0, 240, 320, 240);
    SetDefDrawEnv(&draw[1], 0, 0, 320, 240);
    draw[0].isbg = 1;
    draw[1].isbg = 1;
    setRGB0(&draw[0], 32, 32, 32);
    setRGB0(&draw[1], 32, 32, 32);
    SetDispMask(1);
    
    FntLoad(960, 0);
    SetDumpFnt(FntOpen(32, 32, 256, 64, 0, 1024));
    
    enemy1 = createEnemy(1, 4);
    enemy2 = createEnemy(5, 5);
    //enemy3 = createEnemy(6, 6);

    while (1)
    {
        update();
        render();

        dumpVector("player", &playerPosition);

        FntFlush(-1);
        DrawSync(0);
        VSync(2);
        db = !db;
        PutDispEnv(&disp[db]);
        PutDrawEnv(&draw[db]);
        framesCount++;
    }
    
    return 0;
}