#include <libetc.h>
#include <libgpu.h>
#include <libgte.h>
#include <stdlib.h>

#include "tiles.h"

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
    {1,0,1,1,0,1,0,1,1,1,0,0,1,1,0,1,1,1,0,1},
    {1,0,1,0,0,0,0,0,0,1,0,0,0,0,0,0,1,0,0,1},
    {1,0,0,0,1,1,1,0,0,0,0,0,1,0,0,0,1,0,0,1},
    {1,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,1},
    {1,0,1,0,0,0,0,0,0,1,0,0,1,0,0,0,1,0,0,1},
    {1,0,1,1,1,0,0,0,0,1,0,0,1,1,1,0,1,0,0,1},
    {1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,0,0,0,1,1,1,1,1,1,0,0,1,1,1,1,1},
    {1,1,1,1,0,0,0,1,1,1,1,1,1,0,0,1,1,1,1,1},
    {1,0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,1},
    {1,0,1,0,0,1,0,1,0,0,0,0,1,1,1,1,0,1,1,1},
    {1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,1,0,1},
    {1,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,1,0,1},
    {1,0,0,0,0,0,0,0,0,1,0,1,0,1,1,1,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1,0,1,0,1},
    {1,0,0,1,1,0,0,0,0,1,0,1,0,1,0,1,0,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1},
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

    int distDx = ABS(0x1000000 / dx);
    int distDy = ABS(0x1000000 / dy);

    int totalDistDx = (dxSign * distDx * startDx) >> 12;
    int totalDistDy = (dySign * distDy * startDy) >> 12;
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

        //if (rayCell->x < 0 || rayCell->x >= mapCols
        //        || rayCell->y < 0 || rayCell->y >= mapRows) break;

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
int playerMoving = 0;

#define PLAYER_RADIUS 1792

void movePlayer(int speed, int strafe)
{ 
    int prevX = playerSrc.x;
    int prevY = playerSrc.y;

    int hitWall = 0;
    int vy = (speed * sin(playerAngle + strafe)) >> 12;
    int vx = (speed * cos(playerAngle + strafe)) >> 12;

    // check collision x
    int signX = vx >= 0 ? 1 : -1;
    int playerTmpX = playerSrc.x + vx;
    int collX = (playerTmpX + signX * PLAYER_RADIUS) >> 12;
    int collY0 = (playerSrc.y - PLAYER_RADIUS) >> 12;
    int collY1 = (playerSrc.y + PLAYER_RADIUS) >> 12;
    int t1 = map[collY0][collX];
    int t2 = map[collY1][collX];
    if (t1 == 1 || t2 == 1) {
        hitWall = 1;
        playerSrc.x = (collX << 12) + (signX < 0 ? 4096 : 0) - signX * (PLAYER_RADIUS + 4);
    }
    else {
        playerSrc.x = playerTmpX;
    }

    // check collision y
    int signY = vy >= 0 ? 1 : -1;
    int playerTmpY = playerSrc.y + vy;
    int collY = (playerTmpY + signY * PLAYER_RADIUS) >> 12;
    int collX0 = (playerSrc.x - PLAYER_RADIUS) >> 12;
    int collX1 = (playerSrc.x + PLAYER_RADIUS) >> 12;
    int t3 = map[collY][collX0];
    int t4 = map[collY][collX1];
    if (t3 == 1 || t4 == 1) {
        hitWall = 1;
        playerSrc.y = (collY << 12) + (signY < 0 ? 4096 : 0) - signY * (PLAYER_RADIUS + 4);
    }
    else {
        playerSrc.y = playerTmpY;
    }

    if ((prevX & 0xfffffff0) != (playerSrc.x & 0xfffffff0) || (prevY & 0xfffffff0) != (playerSrc.y & 0xfffffff0))
    {
        playerMoving = 1;
    }
}

void updatePlayerControl()
{
    playerMoving = 0;

    u_long padInfo = PadRead(0);
    
    int speed = 256 + 64;

    // strafe
    if (padInfo & _PAD(0,PADL1)) {
        movePlayer(speed, -1024);
    }
    if (padInfo & _PAD(0,PADR1)) {
        movePlayer(speed, 1024);
    }

    // rotate
    if (padInfo & _PAD(0,PADLleft)) {
        playerMoving = 1;
        playerAngle -= 64;
    }
    if (padInfo & _PAD(0,PADLright)) {
        playerMoving = 1;
        playerAngle += 64;
    }

    if (padInfo & _PAD(0,PADLup)) {
        movePlayer(speed, 0);
    }
    if (padInfo & _PAD(0,PADLdown)) {
        movePlayer(-speed, 0);
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

TIM_IMAGE image;
u_short tpage;
u_short tpageFloor;

void loadTexture()
{
    OpenTIM((u_long*) tiles);
    ReadTIM(&image);
    tpage = getTPage(image.mode & 3, 3, image.prect->x, image.prect->y);
    LoadImage(image.prect, (u_long*) image.paddr);
    DrawSync(0);
}


#define OTLEN 320
u_long ot[OTLEN];
LINE_F2 lines[320];
POLY_FT4 polys[320];

int resolution = 1;

void drawWall(int index, int side, int wallX, int interDist, int angle)
{
    int interDistFixed = (interDist * cos(angle)) >> 12;
    int persp = 0x800000 / interDistFixed;
    int wallY = persp * 160;
    int wallYInt = wallY >> 12;

    int y0 = 120 - wallYInt;
    int y1 = 120 + wallYInt;

    setPolyFT4(&polys[index]);
    setRGB0(&polys[index], 128, 128, 128);
    setXY4(&polys[index], wallX, y0, wallX + resolution, y0, wallX + resolution, y1, wallX, y1);
    int tx = wallX % 64;

    if (side == 1)
    {
        tx = 128 + ((playerIntersectionPoint.x & 0xfff) >> 6);
    }
    else
    {
        tx = (playerIntersectionPoint.y & 0xfff) >> 6;
    }
    
    setUV4(&polys[index], tx, 0, tx + 1, 0, tx + 1, 64, tx, 64);
    polys[index].tpage = tpage;
    AddPrim(&ot[index], &polys[index]);

}

int w = 0;

void renderWalls()
{
    w = (w + 1) % 3;

    ClearOTagR(ot, OTLEN);

    // for each 3 frames, when player is moving the resolution is decreased by half (160 pixels)
    // for the first 2 frames, the 3rd frame is rendered using full 320 resolution 
    FntPrint("moving: %d", playerMoving);
    resolution = 1;
    if (playerMoving & w < 2)
    {
        resolution = 2;
    }

    // render walls
    for (int sx = 0; sx < 320; sx += resolution)
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

int floorAngles[240];

void initFloorAngles()
{
    floorAngles[ 239 ] =  43 ;
    floorAngles[ 238 ] =  43 ;
    floorAngles[ 237 ] =  43 ;
    floorAngles[ 236 ] =  44 ;
    floorAngles[ 235 ] =  44 ;
    floorAngles[ 234 ] =  44 ;
    floorAngles[ 233 ] =  45 ;
    floorAngles[ 232 ] =  45 ;
    floorAngles[ 231 ] =  46 ;
    floorAngles[ 230 ] =  46 ;
    floorAngles[ 229 ] =  46 ;
    floorAngles[ 228 ] =  47 ;
    floorAngles[ 227 ] =  47 ;
    floorAngles[ 226 ] =  48 ;
    floorAngles[ 225 ] =  48 ;
    floorAngles[ 224 ] =  49 ;
    floorAngles[ 223 ] =  49 ;
    floorAngles[ 222 ] =  50 ;
    floorAngles[ 221 ] =  50 ;
    floorAngles[ 220 ] =  51 ;
    floorAngles[ 219 ] =  51 ;
    floorAngles[ 218 ] =  52 ;
    floorAngles[ 217 ] =  52 ;
    floorAngles[ 216 ] =  53 ;
    floorAngles[ 215 ] =  53 ;
    floorAngles[ 214 ] =  54 ;
    floorAngles[ 213 ] =  55 ;
    floorAngles[ 212 ] =  55 ;
    floorAngles[ 211 ] =  56 ;
    floorAngles[ 210 ] =  56 ;
    floorAngles[ 209 ] =  57 ;
    floorAngles[ 208 ] =  58 ;
    floorAngles[ 207 ] =  58 ;
    floorAngles[ 206 ] =  59 ;
    floorAngles[ 205 ] =  60 ;
    floorAngles[ 204 ] =  60 ;
    floorAngles[ 203 ] =  61 ;
    floorAngles[ 202 ] =  62 ;
    floorAngles[ 201 ] =  63 ;
    floorAngles[ 200 ] =  64 ;
    floorAngles[ 199 ] =  64 ;
    floorAngles[ 198 ] =  65 ;
    floorAngles[ 197 ] =  66 ;
    floorAngles[ 196 ] =  67 ;
    floorAngles[ 195 ] =  68 ;
    floorAngles[ 194 ] =  69 ;
    floorAngles[ 193 ] =  70 ;
    floorAngles[ 192 ] =  71 ;
    floorAngles[ 191 ] =  72 ;
    floorAngles[ 190 ] =  73 ;
    floorAngles[ 189 ] =  74 ;
    floorAngles[ 188 ] =  75 ;
    floorAngles[ 187 ] =  76 ;
    floorAngles[ 186 ] =  77 ;
    floorAngles[ 185 ] =  78 ;
    floorAngles[ 184 ] =  80 ;
    floorAngles[ 183 ] =  81 ;
    floorAngles[ 182 ] =  82 ;
    floorAngles[ 181 ] =  83 ;
    floorAngles[ 180 ] =  85 ;
    floorAngles[ 179 ] =  86 ;
    floorAngles[ 178 ] =  88 ;
    floorAngles[ 177 ] =  89 ;
    floorAngles[ 176 ] =  91 ;
    floorAngles[ 175 ] =  93 ;
    floorAngles[ 174 ] =  94 ;
    floorAngles[ 173 ] =  96 ;
    floorAngles[ 172 ] =  98 ;
    floorAngles[ 171 ] =  100 ;
    floorAngles[ 170 ] =  102 ;
    floorAngles[ 169 ] =  104 ;
    floorAngles[ 168 ] =  106 ;
    floorAngles[ 167 ] =  108 ;
    floorAngles[ 166 ] =  111 ;
    floorAngles[ 165 ] =  113 ;
    floorAngles[ 164 ] =  116 ;
    floorAngles[ 163 ] =  119 ;
    floorAngles[ 162 ] =  121 ;
    floorAngles[ 161 ] =  124 ;

}

DISPENV disp[2];
DRAWENV draw[4];
int db = 0;

POLY_FT4 ceil[240];
POLY_FT4 floor[240];
POLY_FT4 poly[200];

VECTOR vertices[10][10];
MATRIX matrix;
SVECTOR rotation = { 0, 0, 0 };
VECTOR translation = { 192, 0, 192 };

void renderFloorAndCeiling() {
    rotation.vy = playerAngle + 1024;

    RotMatrix(&rotation, &matrix);
    TransMatrix(&matrix, &translation);

    SetRotMatrix(&matrix);
    SetTransMatrix(&matrix);
    
    long flag;
    SVECTOR modelPoint = { 0, 0, 0 };
    VECTOR worldPoint = { 0, 0, 0 };
    
    int pdx = ((playerSrc.x * 64) >> 12) % 64;
    int pdz = ((playerSrc.y * 64) >> 12) % 64;

    int polySize = 10;
    int polySizeMinusOne = polySize - 1;
    int polyHalfSize = polySize / 2;
    for (int row = 0; row < polySize; row++)
    {
        for (int col = 0; col < polySize; col++)
        {
            modelPoint.vx = (col - polyHalfSize) * 64 - pdx;
            modelPoint.vz = (row - polyHalfSize) * 64 - pdz;
            RotTrans(&modelPoint, &vertices[row][col], &flag);
        }
    }

    ClearOTagR(ot, OTLEN);
    for (int row = 0; row < polySizeMinusOne; row++)
    {
        for (int col = 0; col < polySizeMinusOne; col++)
        {
            VECTOR p0 = vertices[row][col];
            VECTOR p1 = vertices[row][col + 1];
            VECTOR p2 = vertices[row + 1][col + 1];
            VECTOR p3 = vertices[row + 1][col];

            int polyIndex = row * polySize + col;
            setPolyFT4(&poly[polyIndex]);
            setRGB0(&poly[polyIndex], 128, 128, 128);
            setXY4(&poly[polyIndex], p0.vx, p0.vz, p1.vx, p1.vz, p2.vx, p2.vz, p3.vx, p3.vz);
            setUV4(&poly[polyIndex], 0, 65, 63, 65, 63, 127, 0, 127);
            poly[polyIndex].tpage = tpage;
            AddPrim(&ot[1], &poly[polyIndex]);
        }
    }
    DrawOTag2(&ot[OTLEN - 1]);
    DrawSync(0);
    PutDrawEnv(&draw[db]);
    
    ClearOTagR(ot, OTLEN);

    for (int sy = 239; sy >= 161; sy--)
    {
        
        if (sy <= 170 && (sy % 2) == 0) continue;
        if (sy <= 190 && (sy % 3) == 0) continue;

        int z = floorAngles[sy];

        // floor
        setPolyFT4(&floor[sy]);
        setRGB0(&floor[sy], 128, 128, 128);
        setXY4(&floor[sy], 0, sy, 320, sy, 320, sy + 1, 0, sy + 1);
        setUV4(&floor[sy], 128 - z, 191 - z, 128 + z, 191 - z, 128 + z, 192 - z, 128 - z, 192 - z);
        floor[sy].tpage = tpageFloor;        
        AddPrim(&ot[sy], &floor[sy]);

        //ceil
        setPolyFT4(&ceil[sy]);
        setRGB0(&ceil[sy], 128, 128, 128);
        setXY4(&ceil[sy], 0, 239 - sy, 320, 239 - sy, 320, 239 - sy + 1, 0, 239 - sy + 1);
        setUV4(&ceil[sy], 128 - z, 191 - z, 128 + z, 191 - z, 128 + z, 192 - z, 128 - z, 192 - z);
        ceil[sy].tpage = tpageFloor;        
        AddPrim(&ot[sy], &ceil[sy]);
    }

    int pacing = 4;
    for (int sy = 160; sy > 120; sy -= pacing)
    {
        pacing += 2;
        int k = 160 - sy;
        int z = 97 + k;

        // floor
        setPolyFT4(&floor[sy]);
        setRGB0(&floor[sy], 128, 128, 128);
        setXY4(&floor[sy], -pacing, sy, 320 + pacing, sy, 320 + pacing, sy + 1, -pacing, sy + 1);
        setUV4(&floor[sy], 128 - z, 191 - z, 128 + z, 191 - z, 128 + z, 192 - z, 128 - z, 192 - z);
        floor[sy].tpage = tpageFloor;        
        AddPrim(&ot[sy], &floor[sy]);

        int k2 = 40 - (160 - sy);
        int z2 = 97 + k2;

        //ceil
        setPolyFT4(&ceil[sy]);
        setRGB0(&ceil[sy], 128, 128, 128);
        setXY4(&ceil[sy], -pacing, 239 - sy, 320 + pacing, 239 - sy, 320 + pacing, 239 - sy + 1, -pacing, 239 - sy + 1);
        setUV4(&ceil[sy], 128 - z2, 191 - z2, 128 + z2, 191 - z2, 128 + z2, 192 - z2, 128 - z2, 192 - z2);
        ceil[sy].tpage = tpageFloor;        
        AddPrim(&ot[sy], &ceil[sy]);
    }

    DrawOTag2(&ot[OTLEN - 1]);
    DrawSync(0);    
}

int main()
{
    initFloorAngles();
    ResetGraph(0);
    InitGeom();
    PadInit(0);
    SetDispMask(1);
    loadTexture();
    
    SetDefDispEnv(&disp[0], 0, 0, 320, 240);
    SetDefDispEnv(&disp[1], 0, 240, 320, 240);

    SetDefDrawEnv(&draw[0], 0, 240, 320, 240);
    SetDefDrawEnv(&draw[1], 0, 0, 320, 240);
    SetDefDrawEnv(&draw[2], 512, 0, 384, 192);
    SetDefDrawEnv(&draw[3], 576, 0, 256, 192);

    tpageFloor = getTPage(2, 1, 576, 0);
    draw[0].tpage = tpage;
    draw[1].tpage = tpage;
    draw[2].tpage = tpage;
    draw[3].tpage = tpage;

    draw[0].isbg = 1;
    draw[1].isbg = 1;
    draw[2].isbg = 0;
    draw[3].isbg = 0;

    setRGB0(&draw[0], 16, 16, 16);
    setRGB0(&draw[1], 16, 16, 16);
    setRGB0(&draw[2], 255, 32, 32);
    setRGB0(&draw[3], 255, 32, 32);
    
    FntLoad(320, 256);
    FntOpen(32, 32, 256, 64, 0, 1024);

    while (1)
    {
        updatePlayerControl();

        PutDrawEnv(&draw[2]);
        renderFloorAndCeiling();
        renderWalls();

        SPRT sprt;
        setSprt(&sprt);
        setRGB0(&sprt, 128, 128, 128);
        setXY0(&sprt, 32, 64 + 16);
        setWH(&sprt, 255, 64);
        setUV0(&sprt, 0, 192);
        setSemiTrans(&sprt, 1);
        DrawPrim(&sprt);

        FntFlush(-1);
        DrawSync(0);
        VSync(0);

        db = !db;
        PutDispEnv(&disp[db]);
    }

    return 0;
}