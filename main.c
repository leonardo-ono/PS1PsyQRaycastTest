// compiled using PSX.Dev VS Code extension

#include <libetc.h>
#include <libgpu.h>
#include <libgte.h>
#include <stdlib.h>

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

typedef struct {
    int x;
    int y;
} Point;

#define mapRows 20
#define mapCols 20

int map[mapRows][mapCols] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,1},
    {1,0,1,0,0,1,0,1,1,1,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,1,1,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,0,0,0,1,1,1,1,1,1,0,0,1,1,1,1,1},
    {1,1,1,1,0,0,0,1,1,1,1,1,1,0,0,1,1,1,1,1},
    {1,0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,1},
    {1,0,1,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,1,1,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
};
    

// return: 0 - intersection not detected
//         1 - intersection horizontal wall
//         2 - intersection vertical wall
int raycast(Point src, int angle, Point *rayCell, Point *intersectionPoint, int maxRayDistance, int *intersDist) {
    int dy = sin(angle);
    int dx = cos(angle);

    if (dx >= -32 && dx <= 32)
    { 
        dx = 33;
    }
    if (dy >= -32 && dy <= 32)
    { 
        dy = 33;
    }

    int dxSign = (dx & 0x80000000) ? -1 : 1; 
    int dySign = (dy & 0x80000000) ? -1 : 1;

    rayCell->x = src.x >> 12; 
    rayCell->y = src.y >> 12; 

    int startDy = (dySign * 2048) + 2048 - (src.y & 0x00000fff);
    int startDx = (dxSign * 2048) + 2048 - (src.x & 0x00000fff);

    int distDx = 0x1000000 / dx;
    int distDy = 0x1000000 / dy;

    distDx = (distDx < 0) ? -distDx : distDx;
    distDy = (distDy < 0) ? -distDy : distDy;

    int totalDistDx = (distDx * dxSign * startDx) >> 12;
    int totalDistDy = (distDy * dySign * startDy) >> 12;
    int intersectionDistance = 0;
    int side = 0;

    while (intersectionDistance < maxRayDistance) {
       if (totalDistDx < totalDistDy) 
        {
            rayCell->x += dxSign;
            intersectionDistance = totalDistDx;
            totalDistDx += distDx;
            side = 2;
        }
        else 
        {
            rayCell->y += dySign;
            intersectionDistance = totalDistDy;
            totalDistDy += distDy;
            side = 1;
        }

        if (rayCell->x < 0 || rayCell->x >= mapCols
                || rayCell->y < 0 || rayCell->y >= mapRows) break;

        if (map[rayCell->y][rayCell->x] == 1) {
            int ipx = src.x + ((intersectionDistance * dx) >> 12);
            int ipy = src.y + ((intersectionDistance * dy) >> 12);
            intersectionPoint->x = ipx;
            intersectionPoint->y = ipy;
            *intersDist = intersectionDistance;
            return side;
        }
    }

    return 0;
}

Point playerSrc = { 22528, 22528 }; // = (5.5, 5.5)
Point playerRayCell = { 0, 0 }; 
Point playerIntersectionPoint = { 0, 0 };
int playerAngle = 0;

void movePlayer(int speed, int strafe)
{ 
    int dy = (speed * sin(playerAngle + strafe)) >> 12;
    int dx = (speed * cos(playerAngle + strafe)) >> 12;
    playerSrc.x += dx;
    playerSrc.y += dy;
}

void updatePlayerControl()
{
    u_long padInfo = PadRead(0);
    
    // strafe
    if (padInfo == _PAD(0,PADL1)) {
        movePlayer(512, -1024);
    }
    if (padInfo == _PAD(0,PADR1)) {
        movePlayer(512, 1024);
    }

    // rotate
    if (padInfo == _PAD(0,PADLleft)) {
        playerAngle -= 64;
    }
    if (padInfo == _PAD(0,PADLright)) {
        playerAngle += 64;
    }

    if (padInfo == _PAD(0,PADLup)) {
        movePlayer(512, 0);
    }
    if (padInfo == _PAD(0,PADLdown)) {
        movePlayer(-512, 0);
    }    
}

int projPlaneDistance;
int projectionAngles[320] = {
    -511,-509,-507,-505,-503,-501,-499,-497,-495,-493,-490,-488,-486,-484,-482,-479,
    -477,-475,-473,-470,-468,-466,-463,-461,-459,-456,-454,-451,-449,-447,-444,-442,
    -439,-437,-434,-432,-429,-427,-424,-421,-419,-416,-414,-411,-408,-406,-403,-400,
    -398,-395,-392,-389,-386,-384,-381,-378,-375,-372,-369,-366,-364,-361,-358,-355,
    -352,-349,-346,-343,-340,-337,-333,-330,-327,-324,-321,-318,-315,-311,-308,-305,
    -302,-298,-295,-292,-289,-285,-282,-278,-275,-272,-268,-265,-261,-258,-254,-251,
    -247,-244,-240,-237,-233,-230,-226,-223,-219,-215,-212,-208,-204,-201,-197,-193,
    -189,-186,-182,-178,-174,-171,-167,-163,-159,-155,-151,-148,-144,-140,-136,-132,
    -128,-124,-120,-116,-112,-108,-104,-101,-97,-93,-89,-85,-81,-77,-73,-68,-64,-60,
    -56,-52,-48,-44,-40,-36,-32,-28,-24,-20,-16,-12,-8,-4,0,4,8,12,16,20,24,28,32,36,
    40,44,48,52,56,60,64,68,73,77,81,85,89,93,97,101,104,108,112,116,120,124,128,132,
    136,140,144,148,151,155,159,163,167,171,174,178,182,186,189,193,197,201,204,208,
    212,215,219,223,226,230,233,237,240,244,247,251,254,258,261,265,268,272,275,278,
    282,285,289,292,295,298,302,305,308,311,315,318,321,324,327,330,333,337,340,343,
    346,349,352,355,358,361,364,366,369,372,375,378,381,384,386,389,392,395,398,400,
    403,406,408,411,414,416,419,421,424,427,429,432,434,437,439,442,444,447,449,451,
    454,456,459,461,463,466,468,470,473,475,477,479,482,484,486,488,490,493,495,497,
    499,501,503,505,507,509
};

#define OTLEN 1
u_long ot[OTLEN];
LINE_F2 lines[320];

void drawWall(int index, int side, int wallX, int interDist, int angle)
{
    int interDistFixed = (interDist * cos(angle)) >> 12;
    int persp = 0x800000 / interDistFixed;
    int wallY = persp * 160;
    int wallYInt = wallY >> 12;

    int distRange = interDistFixed > 0x14000 ? 0x14000 : interDist;
    int distPerc = (distRange << 12) / 0x14000;
    int colorIntensity = 255 - ((255 * distPerc) >> 12); 
    int colorHalfIntensity = colorIntensity >> 1;
    
    colorIntensity = colorIntensity < 32 ? 32 : colorIntensity;
    colorHalfIntensity = colorHalfIntensity < 32 ? 32 : colorHalfIntensity;

    setLineF2(&lines[index]);
    setRGB0(&lines[index], colorHalfIntensity, colorHalfIntensity, colorHalfIntensity);
    if (side == 1)
    {
        setRGB0(&lines[index], colorIntensity, colorIntensity, colorIntensity);
    }
    setXY2(&lines[index], wallX, 120 - wallYInt, wallX, 120 + wallYInt);

    AddPrim(&ot[0], &lines[index]);
}

void render()
{
    ClearOTagR(ot, OTLEN);

    // render walls
    for (int sx = 0; sx < 320; sx++)
    {
        int interDist;
        int angle = playerAngle + projectionAngles[sx];
        int side = raycast(playerSrc, angle, &playerRayCell, &playerIntersectionPoint, 0x800000, &interDist);
        if (side > 0)
        {
            drawWall(sx, side, sx, interDist, projectionAngles[sx]);
        }
    } 

    DrawOTag2(&ot[OTLEN - 1]);
}

DISPENV disp[2];
DRAWENV draw[2];
int db = 0;

int main()
{
    ResetGraph(0);
    InitGeom();
    PadInit(0);

    SetDefDispEnv(&disp[0], 0, 0, 320, 240);
    SetDefDispEnv(&disp[1], 0, 240, 320, 240);
    SetDefDrawEnv(&draw[0], 0, 240, 320, 240);
    SetDefDrawEnv(&draw[1], 0, 0, 320, 240);

    draw[0].isbg = 1;
    draw[1].isbg = 1;
    setRGB0(&draw[0], 32, 32, 32);
    setRGB0(&draw[1], 32, 32, 32);

    SetDispMask(1);

    FntLoad(320, 0);
    FntOpen(32, 32, 256, 64, 0, 1024);

    while (1)
    {
        updatePlayerControl();
        render();

        FntFlush(-1);
        DrawSync(0);
        VSync(0);
        db = !db;
        PutDispEnv(&disp[db]);
        PutDrawEnv(&draw[db]);
    }

    return 0;
}