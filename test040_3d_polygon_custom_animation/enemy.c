#include <libgpu.h>
#include <libgte.h>
#include <stdlib.h>

#include "run.h"
#include "run_anim.h"
#include "enemy.h"

static POLY_FT3 polys[3000];

static int enemyIndex;
static Enemy enemies[MAX_ENEMIES];

Enemy* createEnemy(int col, int row) //long vx, long vy, long vz)
{
    if (enemyIndex >= MAX_ENEMIES)
    {
        return NULL;
    }
    Enemy *enemy = &enemies[enemyIndex];
    enemy->col = col;
    enemy->row = row;
    enemy->dstCol = col;
    enemy->dstRow = row;

    enemy->position.vx = col * 720 + 360;
    enemy->position.vy = 300;
    enemy->position.vz = row * 720 + 360;

    enemy->rotation.vx = 2800;
    enemy->rotation.vy = ENEMY_DOWN;
    enemy->rotation.vz = 0;

    enemy->lastDirection = ENEMY_DOWN;

    enemy->animFrame = rand() % FRAMES_COUNT;
    enemyIndex++;
    return enemy;
}

#define mapRows 16
#define mapCols 16
extern int map[mapRows][mapCols];

void updateEnemy(Enemy *enemy)
{
    int speed = 20;
    enemy->animFrame = (enemy->animFrame + 1) % FRAMES_COUNT;
    long dstX = enemy->dstCol * 720 + 360;
    long dstZ = enemy->dstRow * 720 + 360;
    long dx = dstX - enemy->position.vx;
    long dz = dstZ - enemy->position.vz;
    long vx = 0;
    long vz = 0;
    if (dx > 0) vx = speed;
    if (dx < 0) vx = -speed;
    if (dz > 0) vz = speed;
    if (dz < 0) vz = -speed;
    if (ABS(dx) <= speed)
    {
        enemy->position.vx = dstX;
    }
    else 
    {
        enemy->position.vx += vx;
    }
    if (ABS(dz) <= speed)
    {
        enemy->position.vz = dstZ;
    }
    else 
    {
        enemy->position.vz += vz;
    }

    // update angle
    int da = enemy->lastDirection - enemy->rotation.vy;
    int da2 = (enemy->lastDirection - 4096) - enemy->rotation.vy;
    int da3 = (enemy->lastDirection + 4096) - enemy->rotation.vy;
    int daMin = da;
    if (ABS(daMin) > ABS(da2))
    {
        daMin = da2;
    }
    if (ABS(daMin) > ABS(da3))
    {
        daMin = da3;
    }

    int rotSpeed = 48;
    if (ABS(daMin) < rotSpeed)
    {
        enemy->rotation.vy = enemy->lastDirection;
    }
    else if (daMin > 0)
    {
        enemy->rotation.vy += rotSpeed;
    }
    else 
    {
        enemy->rotation.vy -= rotSpeed;
    }


    // reached target position 
    if (ABS(dx) <= speed && ABS(dz) <= speed)
    {
        enemy->col = enemy->dstCol;
        enemy->row = enemy->dstRow;
        int ok = 0;
        while (!ok)
        {
            int newDirection = rand() % 4;
            switch (newDirection)
            {
                // up
                case 0:
                    if (enemy->lastDirection != ENEMY_DOWN && map[enemy->row - 1][enemy->col] == 0)
                    {
                        enemy->dstRow--;
                        enemy->lastDirection = ENEMY_UP;
                        //enemy->rotation.vy = ENEMY_UP;
                        ok = 1;
                    }
                    break;
                // down
                case 1:
                    if (enemy->lastDirection != ENEMY_UP && map[enemy->row + 1][enemy->col] == 0)
                    {
                        enemy->dstRow++;
                        enemy->lastDirection = ENEMY_DOWN;
                        //enemy->rotation.vy = ENEMY_DOWN;
                        ok = 1;
                    }
                    break;
                // left
                case 2:
                    if (enemy->lastDirection != ENEMY_RIGHT && map[enemy->row][enemy->col - 1] == 0)
                    {
                        enemy->dstCol--;
                        enemy->lastDirection = ENEMY_LEFT;
                        //enemy->rotation.vy = ENEMY_LEFT;
                        ok = 1;
                    }
                    break;
                // right
                case 3:
                    if (enemy->lastDirection != ENEMY_LEFT && map[enemy->row][enemy->col + 1] == 0)
                    {
                        enemy->dstCol++;
                        enemy->lastDirection = ENEMY_RIGHT;
                        //enemy->rotation.vy = ENEMY_RIGHT;
                        ok = 1;
                    }
                    break;
                
                default:
                    break;
            }
        }
    }
}

void renderEnemy(u_long *ot, int OT_LEN, u_short tpage, MATRIX *cameraMatrix, Enemy *enemy, int polyStartIndex)
{
    MATRIX modelTransformation;
    SVECTOR modelRotation = { enemy->rotation.vx, enemy->rotation.vy, enemy->rotation.vz };
    VECTOR modelTranslation = { enemy->position.vx, enemy->position.vy, enemy->position.vz };
    VECTOR modelScale = { 224, 224, 224 };

    RotMatrixYXZ(&modelRotation, &modelTransformation);
    ScaleMatrix(&modelTransformation, &modelScale);
    TransMatrix(&modelTransformation, &modelTranslation);

    MATRIX modelViewTransformation;
    CompMatrix(cameraMatrix, &modelTransformation, &modelViewTransformation);

    SetRotMatrix(&modelViewTransformation);
    SetTransMatrix(&modelViewTransformation);
    
    //ClearOTagR(&ot[0], OT_LEN);

    long p;
    long otz;
    long flag;

    for (int f = 0; f < FACES_COUNT; f++)
    {
        int i0 = enemyFaces[f][0][0];
        int i1 = enemyFaces[f][1][0];
        int i2 = enemyFaces[f][2][0];
        SVECTOR *v0 = &anim[enemy->animFrame][i2];
        SVECTOR *v1 = &anim[enemy->animFrame][i1];
        SVECTOR *v2 = &anim[enemy->animFrame][i0];
        
        //POLY_FT3 poly;
        int pi = polyStartIndex + f;
        setPolyFT3(&polys[pi]);

        long nclip = RotAverageNclip3(v0, v1, v2, (long*) &polys[pi].x0, (long*) &polys[pi].x1, (long*) &polys[pi].x2, &p, &otz, &flag);
        if (nclip <= 0 || flag < 0)
        {
            continue;
        }
        CVECTOR c0 = { 128, 128, 128 };
        CVECTOR c1;
        DpqColor(&c0, p, &c1);


        //CVECTOR *color = &colors[f];
        //setRGB0(&poly, color->r, color->g, color->b);
        setRGB0(&polys[pi], c1.r, c1.g, c1.g);
        //setRGB0(&polys[pi], 128, 128, 128);

        setSemiTrans(&polys[pi], 1);

        int uvi0 = enemyFaces[f][0][1];
        int uvi1 = enemyFaces[f][1][1];
        int uvi2 = enemyFaces[f][2][1];
        SVECTOR *uv0 = &enemyUvs[uvi2];
        SVECTOR *uv1 = &enemyUvs[uvi1];
        SVECTOR *uv2 = &enemyUvs[uvi0];
        setUV3(&polys[pi], uv0->vx, uv0->vy, uv1->vx, uv1->vy, uv2->vx, uv2->vy);
        polys[pi].tpage = tpage;

        //DrawPrim(&poly);
        //DrawSync(0);
        AddPrim(&ot[otz], &polys[pi]);
    }

    //DrawOTag2(&ot[OT_LEN - 1]);    
}
