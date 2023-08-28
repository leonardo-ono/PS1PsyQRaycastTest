#ifndef _ENEMY_H_
#define _ENEMY_H_

#include <libgte.h>

#define MAX_ENEMIES 3

#define ENEMY_UP 0
#define ENEMY_DOWN 2048
#define ENEMY_LEFT 1024
#define ENEMY_RIGHT 3072

typedef struct {
    VECTOR position;
    SVECTOR rotation;
    int animFrame;
    int col;
    int row;
    int dstCol;
    int dstRow;
    int lastDirection;
} Enemy;

extern Enemy* createEnemy(int col, int row); //long vx, long vy, long vz);
extern void updateEnemy(Enemy *enemy);
extern void renderEnemy(u_long *ot, int OT_LEN, u_short tpage, MATRIX *cameraMatrix, Enemy *enemy, int polyStartIndex);

#endif /* _ENEMY_H_ */