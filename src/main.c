/*
 * CITY LIFE 3D — NOVA BAY V5 ARABIC UPGRADE
 * Expanded Android open-world driving / city-life prototype.
 * C99 + raylib 5.5 + KarvinJ/raylib-mobile Android template.
 * This project is not affiliated with Rockstar Games or GTA.
 *
 * V5 upgrade: enlarged procedural city, Arabic sprite UI, score/levels, and target-training weapons.
 * V4 foundations: an extended desert drift zone, sand surface FX, expanded NPC/traffic
 * populations, a procedural tree/park distribution, 12 vehicle silhouettes,
 * six bundled original music tracks and a playlist for user-added audio assets.
 * Artwork remains procedural low-poly geometry; external premium models/PBR
 * materials and recorded artist songs are not included.
 */

#include "raymob.h"
#include "rlgl.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define PI_F                 3.14159265358979323846f
#define RAD_TO_DEG           57.29577951308232f
#define CITY_HALF            432.0f
#define WORLD_HALF           930.0f
#define CITY_BLOCK           48.0f
#define CITY_ROAD_COUNT      19
#define MAX_BUILDINGS        1400
#define WALKER_COUNT         128
#define TRAFFIC_COUNT        72
#define TREE_CAPACITY        320
#define MUSIC_CAPACITY       32
#define MISSION_COUNT        12
#define DRAW_DISTANCE        214.0f
#define SAND_X_MIN           440.0f
#define SAND_X_MAX           820.0f
#define SAND_Z_MIN          -110.0f
#define SAND_Z_MAX           390.0f
#define SAND_CENTER_X        630.0f
#define SAND_CENTER_Z        140.0f
#define SMOKE_COUNT          64
#define SKID_COUNT           220
#define PLAYER_RADIUS        0.62f
#define CAR_RADIUS           1.55f
#define TARGET_COUNT         8

/* ---- Palette ----------------------------------------------------------- */
static const Color C_SKY          = { 111, 183, 221, 255 };
static const Color C_GRASS        = { 79, 143, 91, 255 };
static const Color C_ROAD         = { 43, 49, 58, 255 };
static const Color C_SIDEWALK     = { 157, 157, 148, 255 };
static const Color C_CURB         = { 205, 202, 188, 255 };
static const Color C_LANE         = { 246, 194, 69, 255 };
static const Color C_WINDOW       = { 103, 174, 202, 255 };
static const Color C_WINDOW_LIT   = { 246, 205, 131, 255 };
static const Color C_GLASS_DARK   = { 50, 95, 120, 255 };
static const Color C_HUD          = { 12, 20, 31, 232 };
static const Color C_GOLD         = { 255, 194, 66, 255 };
static const Color C_MINT         = { 91, 220, 174, 255 };
static const Color C_WHITE        = { 245, 247, 249, 255 };
static const Color C_RED          = { 225, 84, 74, 255 };
static const Color C_BLUE         = { 64, 143, 222, 255 };
static const Color C_DARK         = { 22, 28, 39, 255 };

/* ---- Data -------------------------------------------------------------- */
typedef struct {
    Vector3 position;
    float width, depth, height;
    int style;
    Color wall;
    Color trim;
    Color sign;
    unsigned int seed;
} Building;

typedef struct {
    Vector3 position;
    float yaw;
    float speed;
    float steering;
    Color paint;
    int model;
} Vehicle;

typedef struct {
    Vector3 position;
    float yaw;
    float phase;
    float speed;
    float routeMin;
    float routeMax;
    bool alongX;
    bool isStatic;
    int direction;
    Color shirt;
    Color trousers;
    Color skin;
} Pedestrian;

typedef struct {
    float x, z, scale;
    int species;
} WorldTree;

typedef struct {
    float x;
    float y;
} MoveInput;

typedef struct {
    Vector3 position;
    Vector3 velocity;
    float life;
    float maxLife;
    float size;
} SmokeParticle;

typedef struct {
    Vector3 a;
    Vector3 b;
    float life;
} SkidMark;

typedef enum {
    GAME_TITLE = 0,
    GAME_PLAYING,
    GAME_PAUSED
} GameState;

/* ---- Global city data -------------------------------------------------- */
static Building gBuildings[MAX_BUILDINGS];
static int gBuildingCount = 0;
static Pedestrian gPedestrians[WALKER_COUNT];
static Vehicle gTraffic[TRAFFIC_COUNT];
static WorldTree gTrees[TREE_CAPACITY];
static int gTreeCount = 0;
static Music gMusicTracks[MUSIC_CAPACITY];
static char gMusicTitles[MUSIC_CAPACITY][64];
static int gMusicCount = 0;
static SmokeParticle gSmoke[SMOKE_COUNT];
static SkidMark gSkids[SKID_COUNT];
static int gSmokeCursor = 0;
static int gSkidCursor = 0;
static float gSmokeSpawnTimer = 0.0f;
static float gNightAmount = 0.0f;
static float gDriftScore = 0.0f;
static int gBestDrift = 0;
static bool gRadioEnabled = true;
static bool gAudioReady = false;
static bool gAudioDeviceReady = false;
static Sound gRadioSounds[3] = {{0}}; /* fallback procedural radio */
static int gRadioIndex = 0;
static Sound gEngineSound = {0};
static Texture2D gArabicAtlas = {0};
static int gWeaponIndex = 0;
static int gWeaponAmmo[3] = {45, 120, 12};
static int gWeaponReserve[3] = {135, 360, 60};
static float gWeaponCooldown = 0.0f;

typedef struct {
    Vector3 position;
    bool active;
    float respawnTimer;
} TrainingTarget;
static TrainingTarget gTargets[TARGET_COUNT];

/* Arabic UI labels are pre-shaped into an atlas to guarantee correct RTL text
   on raylib's small Android default font. The generated PNG contains no font. */
enum {
    AR_CITY=0, AR_CASH, AR_MISSIONS, AR_SPEED, AR_DRIFT, AR_BEST, AR_RADIO, AR_GARAGE,
    AR_PICKUP, AR_DELIVER, AR_ALL_DONE, AR_ENTER_CAR, AR_EXIT, AR_INTERACT, AR_JUMP, AR_GAS,
    AR_BRAKE, AR_NITRO, AR_HANDBRAKE, AR_FIRE, AR_SWITCH_WEAPON, AR_AMMO, AR_POINTS, AR_LEVEL,
    AR_PULSE, AR_RIFLE, AR_SHOCK, AR_RANGE, AR_DOWNTOWN, AR_OLDTOWN, AR_AIRPORT, AR_HARBOR,
    AR_DESERT, AR_COAST, AR_INDUSTRIAL, AR_WEATHER, AR_RAIN, AR_NIGHT, AR_CLEAR, AR_START,
    AR_RESUME, AR_RESTART, AR_PAUSED, AR_MISSION_OK, AR_PICKED, AR_DELIVERED, AR_HIT, AR_NO_AMMO,
    AR_MISS, AR_MOVE, AR_CAMERA, AR_RADIO_NAME, AR_SAVE, AR_SCORE, AR_EASY, AR_JOB,
    AR_SAND, AR_COAST_ROAD, AR_DRIFT_POINTS, AR_MUSIC, AR_SWITCHED, AR_READY, AR_CLOSE,
    AR_FOOT, AR_VEHICLE, AR_KMH
};

static int GetRadioTrackCount(void);
static bool IsInSandArena(Vector3 p);

/* ---- Small helpers ----------------------------------------------------- */
static float ClampF(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static float LerpF(float a, float b, float t) {
    return a + (b - a) * t;
}

static float Distance2D(float x1, float z1, float x2, float z2) {
    float dx = x1 - x2;
    float dz = z1 - z2;
    return sqrtf(dx*dx + dz*dz);
}

static float NormalizeAngle(float a) {
    while (a > PI_F) a -= 2.0f*PI_F;
    while (a < -PI_F) a += 2.0f*PI_F;
    return a;
}

static Color ColorScale(Color c, float factor) {
    Color out = c;
    out.r = (unsigned char)ClampF((float)c.r*factor, 0.0f, 255.0f);
    out.g = (unsigned char)ClampF((float)c.g*factor, 0.0f, 255.0f);
    out.b = (unsigned char)ClampF((float)c.b*factor, 0.0f, 255.0f);
    return out;
}

static Color LerpColor(Color a, Color b, float t) {
    t = ClampF(t, 0.0f, 1.0f);
    return (Color){
        (unsigned char)LerpF((float)a.r, (float)b.r, t),
        (unsigned char)LerpF((float)a.g, (float)b.g, t),
        (unsigned char)LerpF((float)a.b, (float)b.b, t), 255
    };
}

static unsigned int Hash2(int a, int b, int c) {
    unsigned int h = (unsigned int)(a*73856093u) ^
                     (unsigned int)(b*19349663u) ^
                     (unsigned int)(c*83492791u);
    h ^= h >> 13;
    h *= 1274126177u;
    h ^= h >> 16;
    return h;
}

static bool PointInRect(Vector2 p, Rectangle r) {
    return p.x >= r.x && p.x <= r.x + r.width &&
           p.y >= r.y && p.y <= r.y + r.height;
}

static bool TouchHeld(Rectangle r) {
    int count = GetTouchPointCount();
    for (int i = 0; i < count; i++) {
        if (PointInRect(GetTouchPosition(i), r)) return true;
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && PointInRect(GetMousePosition(), r)) return true;
    return false;
}

static bool TouchPressed(Rectangle r, bool *wasDown) {
    bool down = TouchHeld(r);
    bool pressed = down && !(*wasDown);
    *wasDown = down;
    return pressed;
}

static void DrawPanel(Rectangle r, Color fill, Color outline) {
    DrawRectangleRounded(r, 0.12f, 8, fill);
    DrawRectangleRoundedLines(r, 0.12f, 8, outline);
}

static void DrawArabicLabel(int label, Rectangle dest, Color tint) {
    if (gArabicAtlas.id == 0 || label < 0 || label >= 68) return;
    Rectangle source = {(float)(label%4)*256.0f, (float)(label/4)*64.0f, 256.0f, 64.0f};
    DrawTexturePro(gArabicAtlas, source, dest, (Vector2){0.0f, 0.0f}, 0.0f, tint);
}

static void DrawArabicButton(Rectangle r, int label, Color fill, bool active) {
    Color face = active ? ColorScale(fill, 1.23f) : fill;
    DrawRectangleRounded(r, 0.20f, 9, Fade(face, 0.93f));
    DrawRectangleRoundedLines(r, 0.20f, 9, Fade(C_WHITE, 0.82f));
    DrawArabicLabel(label, (Rectangle){r.x + 5.0f, r.y + 5.0f, r.width - 10.0f, r.height - 10.0f}, C_WHITE);
}

static int GetArabicDistrictLabel(Vector3 p) {
    if (IsInSandArena(p)) return AR_DESERT;
    if (p.x < -270.0f && p.z > -170.0f && p.z < 170.0f) return AR_AIRPORT;
    if (p.x > 170.0f && p.z < -275.0f) return AR_HARBOR;
    if (p.z < -250.0f) return AR_COAST;
    if (fabsf(p.x) < 120.0f && fabsf(p.z) < 120.0f) return AR_DOWNTOWN;
    if (p.x > 180.0f) return AR_INDUSTRIAL;
    if (p.z > 190.0f) return AR_OLDTOWN;
    if (p.x < -180.0f) return AR_OLDTOWN;
    return AR_CITY;
}

static int GetArabicMessageLabel(const char *message) {
    if (strstr(message, "TARGET HIT") != NULL) return AR_HIT;
    if (strstr(message, "NO AMMO") != NULL) return AR_NO_AMMO;
    if (strstr(message, "PRACTICE RANGE") != NULL) return AR_MISS;
    if (strstr(message, "DELIVERY COMPLETE") != NULL) return AR_DELIVERED;
    if (strstr(message, "PACKAGE COLLECTED") != NULL) return AR_PICKED;
    if (strstr(message, "ALL JOBS") != NULL) return AR_ALL_DONE;
    if (strstr(message, "VEHICLE READY") != NULL) return AR_READY;
    if (strstr(message, "RADIO:") != NULL) return AR_RADIO_NAME;
    if (strstr(message, "RELOADED") != NULL) return AR_AMMO;
    if (strstr(message, "WEAPON") != NULL) return AR_SWITCH_WEAPON;
    if (strstr(message, "YOU ARE ON FOOT") != NULL) return AR_MOVE;
    if (strstr(message, "CAMERA MODE") != NULL) return AR_CAMERA;
    if (strstr(message, "COLLISION") != NULL) return AR_MISS;
    return -1;
}

static void DrawButton(Rectangle r, const char *label, Color fill, bool active) {
    Color face = active ? ColorScale(fill, 1.23f) : fill;
    DrawRectangleRounded(r, 0.20f, 9, Fade(face, 0.91f));
    DrawRectangleRoundedLines(r, 0.20f, 9, Fade(C_WHITE, 0.82f));
    int size = (r.width < 82.0f) ? 15 : 17;
    int tw = MeasureText(label, size);
    DrawText(label, (int)(r.x + (r.width - (float)tw)*0.5f),
             (int)(r.y + (r.height - (float)size)*0.5f), size, C_WHITE);
}

/* ---- City generation --------------------------------------------------- */
static void AddBuilding(float x, float z, float w, float d, float h,
                        int style, unsigned int seed) {
    if (gBuildingCount >= MAX_BUILDINGS) return;

    static const Color walls[] = {
        { 206, 188, 165, 255 }, { 182, 194, 200, 255 },
        { 210, 171, 151, 255 }, { 170, 186, 176, 255 },
        { 190, 177, 209, 255 }, { 220, 205, 168, 255 },
        { 146, 171, 190, 255 }, { 204, 179, 185, 255 }
    };
    static const Color trims[] = {
        { 87, 100, 112, 255 }, { 73, 90, 103, 255 },
        { 116, 91, 75, 255 }, { 83, 105, 94, 255 }
    };
    static const Color signs[] = {
        { 40, 105, 151, 255 }, { 181, 76, 65, 255 },
        { 44, 133, 105, 255 }, { 126, 82, 156, 255 }
    };

    Building *b = &gBuildings[gBuildingCount++];
    b->position = (Vector3){ x, 0.0f, z };
    b->width = w;
    b->depth = d;
    b->height = h;
    b->style = style;
    b->seed = seed;
    b->wall = walls[seed % (sizeof(walls)/sizeof(walls[0]))];
    b->trim = trims[(seed >> 4) % (sizeof(trims)/sizeof(trims[0]))];
    b->sign = signs[(seed >> 7) % (sizeof(signs)/sizeof(signs[0]))];
}

static void BuildCity(void) {
    gBuildingCount = 0;

    /* Eighteen by eighteen blocks, split into distinct urban districts.
       Airport and harbour corridors remain reserved for their landmarks. */
    for (int bx = -9; bx < 9; bx++) {
        for (int bz = -9; bz < 9; bz++) {
            if (bx == 0 && bz == 0) continue;
            if (bx <= -6 && bz >= -3 && bz <= 2) continue;
            if (bx >= 4 && bz <= -6) continue; /* container harbour district */

            float cx = (float)bx*CITY_BLOCK + CITY_BLOCK*0.5f;
            float cz = (float)bz*CITY_BLOCK + CITY_BLOCK*0.5f;

            for (int sx = -1; sx <= 1; sx += 2) {
                for (int sz = -1; sz <= 1; sz += 2) {
                    unsigned int seed = Hash2(bx, bz, (sx+2)*5 + (sz+2));
                    int style = (int)(seed % 4u);
                    float h = 13.0f + (float)((seed >> 5) % 8u)*3.1f;
                    /* Downtown towers; industrial and old-town buildings stay lower. */
                    if (abs(bx) <= 2 && abs(bz) <= 2) h += 24.0f;
                    else if (bx >= 4) h = 9.0f + (float)((seed >> 8) % 5u)*1.8f;
                    else if (bz >= 4) h = 10.0f + (float)((seed >> 9) % 6u)*2.0f;
                    if (style == 2) h += 12.0f;
                    if (style == 3) h = 10.0f + (float)((seed >> 9) % 4u)*2.0f;
                    if (h > 82.0f) h = 82.0f;
                    float width = 11.5f + (float)((seed >> 12) % 4u)*0.75f;
                    float depth = 11.5f + (float)((seed >> 15) % 4u)*0.75f;
                    AddBuilding(cx + (float)sx*9.0f,
                                cz + (float)sz*9.0f,
                                width, depth, h, style, seed);
                }
            }
        }
    }
}

static bool HitsBuilding(float x, float z, float radius) {
    for (int i = 0; i < gBuildingCount; i++) {
        const Building *b = &gBuildings[i];
        float minX = b->position.x - b->width*0.5f;
        float maxX = b->position.x + b->width*0.5f;
        float minZ = b->position.z - b->depth*0.5f;
        float maxZ = b->position.z + b->depth*0.5f;
        float nearX = ClampF(x, minX, maxX);
        float nearZ = ClampF(z, minZ, maxZ);
        if (Distance2D(x, z, nearX, nearZ) < radius) return true;
    }
    return false;
}

static bool IsInSandArena(Vector3 p) {
    return p.x >= SAND_X_MIN && p.x <= SAND_X_MAX &&
           p.z >= SAND_Z_MIN && p.z <= SAND_Z_MAX;
}

static void MoveWithCollision(Vector3 *p, float dx, float dz, float radius) {
    float nx = ClampF(p->x + dx, -WORLD_HALF + 4.0f, WORLD_HALF - 4.0f);
    if (!HitsBuilding(nx, p->z, radius)) p->x = nx;

    float nz = ClampF(p->z + dz, -WORLD_HALF + 4.0f, WORLD_HALF - 4.0f);
    if (!HitsBuilding(p->x, nz, radius)) p->z = nz;
}

/* ---- World rendering --------------------------------------------------- */
static void DrawRoadNetwork(Vector3 focus) {
    /* City ground. */
    /* A wider ground plane connects the old city to the desert drift peninsula. */
    DrawPlane((Vector3){150.0f, -0.12f, 0.0f},
              (Vector2){1800.0f, 2000.0f}, C_GRASS);

    /* Ocean, beaches and a long coastal promenade north of the city. */
    DrawCube((Vector3){0.0f, -0.08f, -438.0f},
             860.0f, 0.12f, 122.0f, (Color){ 41, 126, 169, 255 });
    DrawCube((Vector3){0.0f, -0.01f, -377.0f},
             860.0f, 0.14f, 8.0f, (Color){ 224, 198, 153, 255 });
    DrawCube((Vector3){0.0f, 0.025f, -366.0f},
             860.0f, 0.11f, 3.0f, C_SIDEWALK);

    for (int i = 0; i < CITY_ROAD_COUNT; i++) {
        float c = -CITY_HALF + (float)i*CITY_BLOCK;
        float width = (i == 4) ? 17.0f : 14.0f;
        float sidewalk = 3.0f;

        /* Road surfaces. */
        DrawCube((Vector3){c, 0.0f, 0.0f}, width, 0.12f, 920.0f, C_ROAD);
        DrawCube((Vector3){0.0f, 0.0f, c}, 920.0f, 0.12f, width, C_ROAD);

        /* Road shoulders and sidewalks on both sides. */
        for (int side = -1; side <= 1; side += 2) {
            float offset = (width*0.5f) + sidewalk*0.5f;
            DrawCube((Vector3){c + (float)side*offset, 0.015f, 0.0f},
                     sidewalk, 0.15f, 920.0f, C_SIDEWALK);
            DrawCube((Vector3){0.0f, 0.015f, c + (float)side*offset},
                     920.0f, 0.15f, sidewalk, C_SIDEWALK);
            DrawCube((Vector3){c + (float)side*(width*0.5f + sidewalk), 0.10f, 0.0f},
                     0.22f, 0.18f, 920.0f, C_CURB);
            DrawCube((Vector3){0.0f, 0.10f, c + (float)side*(width*0.5f + sidewalk)},
                     920.0f, 0.18f, 0.22f, C_CURB);
        }

        /* Broken centre lines. */
        for (int m = -440; m <= 440; m += 18) {
            if (fabsf((float)m - focus.z) < DRAW_DISTANCE) {
                DrawCube((Vector3){c, 0.075f, (float)m}, 0.20f, 0.025f, 7.0f, C_LANE);
            }
            if (fabsf((float)m - focus.x) < DRAW_DISTANCE) {
                DrawCube((Vector3){(float)m, 0.075f, c}, 7.0f, 0.025f, 0.20f, C_LANE);
            }
        }
    }

    /* Two-lane access road from the city ring road to the desert drift zone. */
    DrawCube((Vector3){475.0f, 0.0f, 96.0f}, 190.0f, 0.13f, 17.0f, C_ROAD);
    DrawCube((Vector3){475.0f, 0.09f, 96.0f}, 190.0f, 0.025f, 0.22f, C_LANE);
    DrawCube((Vector3){475.0f, 0.10f, 87.0f}, 190.0f, 0.16f, 0.35f, C_CURB);
    DrawCube((Vector3){475.0f, 0.10f, 105.0f}, 190.0f, 0.16f, 0.35f, C_CURB);

    /* Crosswalk bars at the central civic plaza. */
    for (int i = -4; i <= 4; i++) {
        float p = (float)i*1.55f;
        DrawCube((Vector3){p, 0.09f, 8.8f}, 0.72f, 0.025f, 3.4f, C_WHITE);
        DrawCube((Vector3){8.8f, 0.09f, p}, 3.4f, 0.025f, 0.72f, C_WHITE);
        DrawCube((Vector3){p, 0.09f, -8.8f}, 0.72f, 0.025f, 3.4f, C_WHITE);
        DrawCube((Vector3){-8.8f, 0.09f, p}, 3.4f, 0.025f, 0.72f, C_WHITE);
    }
}

static void DrawBuildingWindows(const Building *b, float distance) {
    /* Distance-based detail keeps the expanded world practical on phones. */
    if (distance > 145.0f) return;

    int floors = (int)(b->height/3.25f);
    if (floors < 2) floors = 2;
    int floorLimit = (distance > 92.0f) ? 5 : 10;
    if (floors > floorLimit) floors = floorLimit;

    float windowY;
    float windowW = (b->style == 2) ? 1.55f : 1.35f;
    float windowH = (b->style == 2) ? 1.55f : 1.25f;
    float sideOffset = b->width*0.5f + 0.055f;
    float frontOffset = b->depth*0.5f + 0.055f;
    int columns = (distance > 92.0f) ? 2 : ((b->width > 13.0f) ? 3 : 2);

    for (int f = 0; f < floors; f++) {
        windowY = 2.1f + (float)f*3.05f;
        if (windowY + 0.8f > b->height - 0.8f) break;

        for (int col = 0; col < columns; col++) {
            float x = b->position.x + ((float)col - (float)(columns-1)*0.5f)*3.0f;
            float z1 = b->position.z + frontOffset;
            float z2 = b->position.z - frontOffset;
            unsigned int lit = Hash2((int)b->position.x + col, (int)b->position.z + f, b->style);
            Color glass = (gNightAmount > 0.34f || (lit % 7u) == 0u) ? C_WINDOW_LIT : C_WINDOW;
            DrawCube((Vector3){x, windowY, z1}, windowW, windowH, 0.09f, glass);
            DrawCube((Vector3){x, windowY, z2}, windowW, windowH, 0.09f, glass);
        }

        /* Side windows on glass and office buildings. */
        if (distance < 92.0f && (b->style == 1 || b->style == 2)) {
            for (int col = -1; col <= 1; col += 2) {
                float z = b->position.z + (float)col*2.5f;
                DrawCube((Vector3){b->position.x + sideOffset, windowY, z},
                         0.09f, windowH, windowW, C_GLASS_DARK);
                DrawCube((Vector3){b->position.x - sideOffset, windowY, z},
                         0.09f, windowH, windowW, C_GLASS_DARK);
            }
        }
    }
}

static void DrawBuilding(const Building *b, Vector3 focus) {
    float distance = Distance2D(focus.x, focus.z, b->position.x, b->position.z);
    if (distance > DRAW_DISTANCE) return;

    Vector3 center = {b->position.x, b->height*0.5f, b->position.z};
    DrawCube(center, b->width, b->height, b->depth, b->wall);
    DrawCubeWires(center, b->width, b->height, b->depth,
                  (Color){ 65, 70, 77, 255 });

    /* Contrasting roof cap and vertical facade strips. */
    DrawCube((Vector3){b->position.x, b->height + 0.20f, b->position.z},
             b->width + 0.45f, 0.40f, b->depth + 0.45f, b->trim);
    DrawCube((Vector3){b->position.x - b->width*0.5f + 0.30f,
                       b->height*0.5f, b->position.z},
             0.32f, b->height, b->depth, ColorScale(b->wall, 0.82f));

    DrawBuildingWindows(b, distance);

    /* Shopfront / awning gives street level a recognizable silhouette. */
    if (b->style == 3) {
        DrawCube((Vector3){b->position.x, 1.55f, b->position.z + b->depth*0.5f + 0.15f},
                 b->width*0.78f, 1.65f, 0.14f, C_GLASS_DARK);
        DrawCube((Vector3){b->position.x, 3.05f, b->position.z + b->depth*0.5f + 0.48f},
                 b->width*0.90f, 0.48f, 0.72f, b->sign);
        DrawCube((Vector3){b->position.x, 0.34f, b->position.z + b->depth*0.5f + 0.38f},
                 b->width*0.94f, 0.18f, 0.72f, b->trim);
    } else {
        DrawCube((Vector3){b->position.x, 1.45f, b->position.z + b->depth*0.5f + 0.06f},
                 b->width*0.72f, 1.35f, 0.10f, C_WINDOW);
    }

    /* Small balconies on selected residential towers. */
    if (distance < 115.0f && (b->style == 0 || b->style == 1)) {
        for (int f = 1; f < 4; f++) {
            float y = 2.7f + (float)f*6.0f;
            if (y < b->height - 2.0f && ((b->seed >> (unsigned int)f) & 1u)) {
                DrawCube((Vector3){b->position.x, y, b->position.z + b->depth*0.5f + 0.55f},
                         b->width*0.55f, 0.13f, 1.0f, b->trim);
                DrawCube((Vector3){b->position.x, y + 0.45f, b->position.z + b->depth*0.5f + 1.0f},
                         b->width*0.55f, 0.12f, 0.10f, C_WHITE);
            }
        }
    }

    /* Rooftop water tank / antenna. */
    if ((b->seed % 3u) == 0u) {
        DrawCylinder((Vector3){b->position.x + b->width*0.22f,
                               b->height + 0.85f,
                               b->position.z - b->depth*0.18f},
                     0.72f, 0.72f, 1.30f, 10, (Color){ 85, 111, 128, 255 });
    } else if ((b->seed % 4u) == 0u) {
        DrawCube((Vector3){b->position.x, b->height + 1.6f, b->position.z},
                 0.18f, 2.8f, 0.18f, b->trim);
        DrawSphere((Vector3){b->position.x, b->height + 3.05f, b->position.z},
                   0.23f, C_RED);
    }
}

static void DrawTree(float x, float z, float scale, int species) {
    DrawCylinder((Vector3){x, 0.95f*scale, z}, 0.22f*scale, 0.28f*scale,
                 1.9f*scale, 8, (Color){119, 82, 53, 255});
    /* Species 0: broadleaf, 1: pine, 2: palm-like crown. */
    if (species == 2) {
        DrawCylinder((Vector3){x, 2.95f*scale, z}, 0.05f*scale, 0.05f*scale,
                     4.0f*scale, 7, (Color){121, 89, 57, 255});
        for (int k = 0; k < 4; k++) {
            float y = (1.55f + (float)k*0.64f)*scale;
            DrawCylinder((Vector3){x, y, z}, (1.08f - (float)k*0.17f)*scale,
                         0.04f*scale, 1.05f*scale, 7,
                         (Color){42, (unsigned char)(105 + k*8), 64, 255});
        }
    } else if (species == 1) {
        DrawSphere((Vector3){x, 2.20f*scale, z}, 1.05f*scale,
                   (Color){58, 130, 73, 255});
        for (int k = 0; k < 6; k++) {
            float a = (float)k*PI_F/3.0f;
            DrawLine3D((Vector3){x, 2.95f*scale, z},
                       (Vector3){x + cosf(a)*1.8f*scale, 2.75f*scale,
                                 z + sinf(a)*1.8f*scale},
                       (Color){62, 137, 76, 255});
        }
    } else {
        DrawSphere((Vector3){x, 2.15f*scale, z}, 1.18f*scale,
                   (Color){51, 133, 76, 255});
        DrawSphere((Vector3){x - 0.58f*scale, 2.0f*scale, z + 0.12f*scale},
                   0.70f*scale, (Color){66, 151, 84, 255});
        DrawSphere((Vector3){x + 0.55f*scale, 2.08f*scale, z - 0.18f*scale},
                   0.72f*scale, (Color){43, 119, 69, 255});
    }
}

static void AddWorldTree(float x, float z, float scale, int species) {
    if (gTreeCount >= TREE_CAPACITY) return;
    gTrees[gTreeCount++] = (WorldTree){x, z, scale, species};
}

static void InitWorldTrees(void) {
    gTreeCount = 0;
    for (int bx = -9; bx < 9; bx++) {
        for (int bz = -9; bz < 9; bz++) {
            unsigned int seed = Hash2(bx, bz, 991);
            float cx = (float)bx*CITY_BLOCK + CITY_BLOCK*0.5f;
            float cz = (float)bz*CITY_BLOCK + CITY_BLOCK*0.5f;
            if ((seed % 2u) != 0u) continue;
            AddWorldTree(cx - 3.8f, cz - 2.2f, 0.72f + (float)(seed%40u)*0.01f,
                         (int)(seed%3u));
            if ((seed % 4u) == 0u)
                AddWorldTree(cx + 4.0f, cz + 3.1f, 0.65f + (float)((seed>>4)%35u)*0.01f,
                             (int)((seed>>3)%3u));
        }
    }
    /* Coastal promenade parks and neighbourhood green belts. */
    for (int i = 0; i < 26; i++) {
        float x = -330.0f + (float)(i%13)*55.0f;
        float z = (i < 13) ? -356.0f : 342.0f;
        AddWorldTree(x, z, 0.80f + (float)(i%4)*0.10f, i%3);
    }
}

static void DrawStreetLamp(float x, float z) {
    DrawCylinder((Vector3){x, 3.2f, z}, 0.10f, 0.10f, 6.4f, 8,
                 (Color){ 63, 72, 82, 255 });
    DrawCube((Vector3){x + 0.68f, 6.25f, z}, 1.45f, 0.12f, 0.12f,
             (Color){ 63, 72, 82, 255 });
    DrawCube((Vector3){x + 1.23f, 6.05f, z}, 0.52f, 0.18f, 0.52f,
             (Color){ 244, 218, 151, 255 });
    if (gNightAmount > 0.25f) {
        DrawSphere((Vector3){x + 1.23f, 5.95f, z}, 0.48f,
                   Fade((Color){255, 220, 145, 255}, gNightAmount*0.30f));
    }
}

static void DrawCentralPlaza(float time) {
    DrawCube((Vector3){24.0f, 0.04f, 24.0f}, 31.0f, 0.18f, 31.0f,
             (Color){ 176, 176, 162, 255 });
    DrawCube((Vector3){24.0f, 0.15f, 24.0f}, 19.0f, 0.12f, 19.0f,
             (Color){ 202, 193, 170, 255 });

    /* Fountain with a softly moving water column. */
    DrawCylinder((Vector3){24.0f, 0.60f, 24.0f}, 4.8f, 4.8f, 0.65f,
                 32, (Color){ 91, 114, 126, 255 });
    DrawCylinder((Vector3){24.0f, 1.02f, 24.0f}, 3.9f, 3.9f, 0.18f,
                 32, (Color){ 92, 173, 196, 255 });
    float waterHeight = 1.5f + 0.18f*sinf(time*2.4f);
    DrawCylinder((Vector3){24.0f, 1.65f + waterHeight*0.5f, 24.0f},
                 0.52f, 0.08f, waterHeight, 16,
                 (Color){ 126, 202, 221, 255 });
    DrawSphere((Vector3){24.0f, 1.55f + waterHeight, 24.0f}, 0.28f,
               (Color){ 176, 230, 237, 255 });

    for (int i = 0; i < 4; i++) {
        float a = (float)i*PI_F*0.5f;
        float x = 24.0f + cosf(a)*11.2f;
        float z = 24.0f + sinf(a)*11.2f;
        DrawCube((Vector3){x, 0.65f, z}, 4.2f, 0.55f, 0.70f,
                 (Color){ 110, 79, 57, 255 });
        DrawCube((Vector3){x, 0.98f, z}, 4.2f, 0.12f, 0.76f,
                 (Color){ 183, 143, 101, 255 });
    }

    DrawTree(10.0f, 10.0f, 0.92f, 0);
    DrawTree(38.0f, 10.0f, 0.92f, 2);
    DrawTree(10.0f, 38.0f, 0.92f, 1);
    DrawTree(38.0f, 38.0f, 0.92f, 0);
}

/* ---- Large-world landmarks -------------------------------------------- */
static void DrawAirport(Vector3 focus) {
    if (Distance2D(focus.x, focus.z, -330.0f, 0.0f) > DRAW_DISTANCE + 80.0f) return;
    DrawCube((Vector3){-330.0f, 0.025f, 0.0f}, 25.0f, 0.18f, 224.0f,
             (Color){ 48, 53, 62, 255 });
    DrawCube((Vector3){-330.0f, 0.13f, 0.0f}, 0.55f, 0.035f, 198.0f, C_WHITE);
    for (int z = -92; z <= 92; z += 24) {
        DrawCube((Vector3){-330.0f, 0.15f, (float)z}, 10.0f, 0.035f, 1.1f, C_WHITE);
    }
    /* Terminal and control tower. */
    DrawCube((Vector3){-292.0f, 5.0f, -82.0f}, 44.0f, 10.0f, 25.0f,
             (Color){ 174, 190, 197, 255 });
    DrawCube((Vector3){-292.0f, 9.0f, -68.8f}, 38.0f, 2.0f, 1.0f, C_WINDOW);
    DrawCube((Vector3){-270.0f, 17.0f, -76.0f}, 7.0f, 24.0f, 7.0f,
             (Color){ 125, 145, 155, 255 });
    DrawCube((Vector3){-270.0f, 30.0f, -76.0f}, 9.0f, 4.0f, 9.0f, C_GLASS_DARK);
    DrawCube((Vector3){-270.0f, 33.0f, -76.0f}, 4.0f, 0.35f, 4.0f, C_RED);
    /* Parked passenger aircraft, stylised from primitives. */
    DrawCylinderEx((Vector3){-330.0f, 3.1f, 0.0f},
                   (Vector3){-330.0f, 3.1f, -20.0f}, 1.7f, 1.7f, 12, C_WHITE);
    DrawCube((Vector3){-330.0f, 3.3f, -6.0f}, 30.0f, 0.28f, 7.5f, C_WHITE);
    DrawCube((Vector3){-330.0f, 3.3f, 1.0f}, 9.0f, 0.18f, 5.0f, C_WHITE);
    DrawCube((Vector3){-330.0f, 3.4f, -12.0f}, 4.8f, 0.18f, 8.5f, C_WHITE);
    DrawCube((Vector3){-330.0f, 4.3f, -9.0f}, 4.2f, 1.0f, 4.6f, C_GLASS_DARK);
}

static void DrawHarbor(Vector3 focus, float time) {
    if (Distance2D(focus.x, focus.z, 270.0f, -332.0f) > DRAW_DISTANCE + 100.0f) return;
    DrawCube((Vector3){270.0f, 0.02f, -352.0f}, 190.0f, 0.28f, 38.0f,
             (Color){ 112, 117, 118, 255 });
    for (int i = 0; i < 12; i++) {
        float x = 190.0f + (float)(i%4)*23.0f;
        float z = -338.0f - (float)(i/4)*12.0f;
        Color c = (i%3 == 0) ? (Color){190, 91, 61, 255} :
                  ((i%3 == 1) ? (Color){58, 112, 145, 255} : (Color){190, 157, 73, 255});
        DrawCube((Vector3){x, 3.0f, z}, 19.0f, 6.0f, 10.0f, c);
        DrawCube((Vector3){x, 6.2f, z}, 19.2f, 0.16f, 10.2f, ColorScale(c, 0.72f));
    }
    DrawCube((Vector3){270.0f, 0.25f, -392.0f}, 180.0f, 0.55f, 50.0f,
             (Color){ 100, 103, 101, 255 });
    for (int i = 0; i < 5; i++) {
        float x = 205.0f + (float)i*32.0f;
        DrawCylinder((Vector3){x, 6.0f, -370.0f}, 0.45f, 0.45f, 12.0f, 8,
                     (Color){ 53, 64, 73, 255 });
        DrawLine3D((Vector3){x, 12.0f, -370.0f},
                   (Vector3){x + 5.0f*sinf(time*0.2f), 20.0f, -365.0f}, C_GOLD);
    }
}

static void DrawDriftDesert(Vector3 focus, float time) {
    if (Distance2D(focus.x, focus.z, SAND_CENTER_X, SAND_CENTER_Z) > DRAW_DISTANCE + 240.0f) return;
    Color sand = (Color){210, 177, 116, 255};
    Color darkSand = (Color){177, 137, 83, 255};
    DrawCube((Vector3){SAND_CENTER_X, -0.045f, SAND_CENTER_Z}, 420.0f, 0.20f, 500.0f, sand);
    /* Rolling dune ridges around the playable drift basin. */
    for (int i = 0; i < 10; i++) {
        float x = 455.0f + (float)(i%5)*88.0f;
        float z = (i < 5) ? -72.0f : 345.0f;
        rlPushMatrix();
        rlTranslatef(x, 1.0f, z);
        rlScalef(1.75f, 0.48f, 1.30f);
        DrawSphere((Vector3){0.0f, 7.0f + (float)(i%3)*1.1f, 0.0f},
                   21.0f, (i%2) ? darkSand : (Color){220, 190, 135, 255});
        rlPopMatrix();
    }
    /* A large circular drift bowl and a second linked donut track. */
    for (int ring = 0; ring < 2; ring++) {
        float radius = 46.0f + (float)ring*1.35f;
        for (int i = 0; i < 72; i++) {
            float a0 = (float)i/72.0f*2.0f*PI_F;
            float a1 = (float)(i+1)/72.0f*2.0f*PI_F;
            DrawLine3D((Vector3){SAND_CENTER_X + cosf(a0)*radius, 0.08f,
                                  SAND_CENTER_Z + sinf(a0)*radius},
                       (Vector3){SAND_CENTER_X + cosf(a1)*radius, 0.08f,
                                  SAND_CENTER_Z + sinf(a1)*radius},
                       Fade(darkSand, 0.72f));
        }
    }
    for (int loop = 0; loop < 2; loop++) {
        float cx = SAND_CENTER_X + (loop == 0 ? -94.0f : 94.0f);
        for (int i = 0; i < 64; i++) {
            float a0 = (float)i/64.0f*2.0f*PI_F;
            float a1 = (float)(i+1)/64.0f*2.0f*PI_F;
            DrawLine3D((Vector3){cx + cosf(a0)*31.0f, 0.09f,
                                  SAND_CENTER_Z + sinf(a0)*42.0f},
                       (Vector3){cx + cosf(a1)*31.0f, 0.09f,
                                  SAND_CENTER_Z + sinf(a1)*42.0f},
                       Fade((Color){142, 105, 64, 255}, 0.82f));
        }
    }
    /* Start/finish gate, flag poles, floodlights and viewing platforms. */
    DrawCube((Vector3){SAND_CENTER_X, 6.0f, SAND_CENTER_Z - 72.0f}, 25.0f, 0.5f, 0.7f,
             (Color){34, 42, 49, 255});
    DrawCube((Vector3){SAND_CENTER_X - 12.0f, 3.2f, SAND_CENTER_Z - 72.0f}, 0.45f, 6.4f, 0.45f, C_DARK);
    DrawCube((Vector3){SAND_CENTER_X + 12.0f, 3.2f, SAND_CENTER_Z - 72.0f}, 0.45f, 6.4f, 0.45f, C_DARK);
    DrawCube((Vector3){SAND_CENTER_X, 6.45f, SAND_CENTER_Z - 72.0f}, 24.0f, 0.65f, 0.35f,
             (Color){197, 66, 54, 255});
    DrawCube((Vector3){SAND_CENTER_X, 4.2f, SAND_CENTER_Z - 94.0f}, 32.0f, 1.0f, 0.7f,
             (Color){30, 39, 49, 255});
    DrawCube((Vector3){SAND_CENTER_X, 4.25f, SAND_CENTER_Z - 93.55f}, 23.0f, 0.22f, 0.06f,
             C_GOLD);
    for (int i = 0; i < 8; i++) {
        float x = 470.0f + (float)i*43.0f;
        DrawCylinder((Vector3){x, 5.0f, -28.0f}, 0.20f, 0.20f, 10.0f, 8,
                     (Color){57, 64, 69, 255});
        if (gNightAmount > 0.25f)
            DrawSphere((Vector3){x, 9.8f, -28.0f}, 1.15f,
                       Fade((Color){255, 223, 162, 255}, 0.55f));
    }
    /* Static spectator stands on the southern edge. */
    DrawCube((Vector3){SAND_CENTER_X, 1.2f, 315.0f}, 150.0f, 2.4f, 14.0f,
             (Color){86, 91, 89, 255});
    for (int row = 0; row < 3; row++)
        DrawCube((Vector3){SAND_CENTER_X, 2.2f + (float)row*0.55f,
                            307.0f + (float)row*2.0f},
                 138.0f, 0.20f, 1.1f, (Color){145, 149, 143, 255});
    if (gNightAmount > 0.35f) {
        float pulse = 0.8f + 0.2f*sinf(time*2.0f);
        DrawSphere((Vector3){SAND_CENTER_X - 82.0f, 10.0f, SAND_CENTER_Z - 42.0f},
                   1.4f*pulse, Fade(C_GOLD, 0.55f));
        DrawSphere((Vector3){SAND_CENTER_X + 82.0f, 10.0f, SAND_CENTER_Z - 42.0f},
                   1.4f*pulse, Fade(C_GOLD, 0.55f));
    }
}

static void DrawWorldLandmarks(Vector3 focus, float time) {
    DrawAirport(focus);
    DrawHarbor(focus, time);
    DrawDriftDesert(focus, time);
    /* Distant low-poly hills give the outer districts a defined skyline. */
    if (focus.z > 170.0f) {
        for (int i = 0; i < 9; i++) {
            float x = -370.0f + (float)i*92.0f;
            float h = 38.0f + (float)((i*17)%43);
            DrawCylinder((Vector3){x, h*0.40f, 440.0f}, 38.0f, 10.0f, h, 7,
                         (Color){ 92, 112, 91, 255 });
        }
    }
}

/* ---- Vehicles ---------------------------------------------------------- */
static void DrawVehicleModel(const Vehicle *v, bool playerVehicle) {
    rlPushMatrix();
    rlTranslatef(v->position.x, v->position.y, v->position.z);
    rlRotatef(v->yaw*RAD_TO_DEG, 0.0f, 1.0f, 0.0f);

    float bodyY = (v->model == 1 || v->model == 6) ? 0.72f :
                  ((v->model == 2 || v->model == 4 || v->model == 5) ? 0.96f : 0.80f);
    float bodyW = (v->model == 1 || v->model == 3 || v->model == 4 || v->model == 5) ? 2.18f : 1.94f;
    float bodyL = (v->model == 1 || v->model == 3 || v->model == 4 || v->model == 5) ? 4.95f : 4.22f;
    Color paint = v->paint;
    Color glass = (Color){ 65, 121, 151, 255 };

    /* Main body, hood and trunk. */
    DrawCube((Vector3){0.0f, bodyY, 0.0f}, bodyW, 0.72f, bodyL, paint);
    DrawCubeWires((Vector3){0.0f, bodyY, 0.0f}, bodyW, 0.72f, bodyL,
                  ColorScale(paint, 0.62f));
    DrawCube((Vector3){0.0f, 1.18f, -1.12f}, bodyW*0.83f, 0.28f, 1.1f,
             ColorScale(paint, 1.12f));
    DrawCube((Vector3){0.0f, 1.17f, 1.38f}, bodyW*0.82f, 0.25f, 0.76f,
             ColorScale(paint, 0.88f));

    /* Cabin and windows. */
    float cabinHeight = (v->model == 1 || v->model == 6) ? 0.56f :
                        ((v->model == 2 || v->model == 4 || v->model == 5) ? 1.02f : 0.82f);
    float cabinY = (v->model == 1 || v->model == 6) ? 1.34f :
                   ((v->model == 2 || v->model == 4 || v->model == 5) ? 1.55f : 1.47f);
    DrawCube((Vector3){0.0f, cabinY, 0.24f}, bodyW*0.78f, cabinHeight,
             (v->model == 3 || v->model == 5) ? 1.42f : 2.0f, ColorScale(paint, 0.94f));
    DrawCube((Vector3){0.0f, 1.52f, -0.79f}, bodyW*0.72f, 0.58f, 0.08f, glass);
    DrawCube((Vector3){0.0f, 1.52f, 1.27f}, bodyW*0.70f, 0.55f, 0.08f, glass);
    DrawCube((Vector3){-bodyW*0.405f, 1.51f, 0.20f}, 0.08f, 0.58f, 1.45f, glass);
    DrawCube((Vector3){ bodyW*0.405f, 1.51f, 0.20f}, 0.08f, 0.58f, 1.45f, glass);

    /* Headlights, tail lamps and bumpers. */
    DrawCube((Vector3){-0.66f, 0.91f, -bodyL*0.5f - 0.035f},
             0.42f, 0.22f, 0.09f, (Color){ 255, 240, 190, 255 });
    DrawCube((Vector3){ 0.66f, 0.91f, -bodyL*0.5f - 0.035f},
             0.42f, 0.22f, 0.09f, (Color){ 255, 240, 190, 255 });
    DrawCube((Vector3){-0.67f, 0.91f, bodyL*0.5f + 0.035f},
             0.38f, 0.24f, 0.09f, C_RED);
    DrawCube((Vector3){ 0.67f, 0.91f, bodyL*0.5f + 0.035f},
             0.38f, 0.24f, 0.09f, C_RED);
    DrawCube((Vector3){0.0f, 0.47f, -bodyL*0.5f - 0.08f}, bodyW*0.94f, 0.18f, 0.18f,
             (Color){ 64, 68, 75, 255 });
    DrawCube((Vector3){0.0f, 0.47f, bodyL*0.5f + 0.08f}, bodyW*0.94f, 0.18f, 0.18f,
             (Color){ 64, 68, 75, 255 });

    /* Four low-poly tyres and silver hubs. */
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2) {
            float x = (float)sx*(bodyW*0.51f);
            float z = (float)sz*(bodyL*0.31f);
            DrawSphere((Vector3){x, 0.50f, z}, 0.43f,
                       (Color){ 24, 28, 34, 255 });
            DrawSphere((Vector3){x*1.015f, 0.50f, z}, 0.20f,
                       (Color){ 157, 166, 174, 255 });
        }
    }

    /* Model-specific silhouettes: sport rear wing, SUV roof rack, pickup bed. */
    if (v->model == 1) {
        DrawCube((Vector3){0.0f, 1.48f, 1.72f}, 1.75f, 0.10f, 0.22f, C_DARK);
        DrawCube((Vector3){0.0f, 1.30f, 1.70f}, 0.10f, 0.36f, 0.14f, C_DARK);
    } else if (v->model == 2) {
        DrawCube((Vector3){0.0f, 2.12f, 0.20f}, 1.55f, 0.08f, 1.60f, C_DARK);
    } else if (v->model == 3) {
        DrawCube((Vector3){0.0f, 1.18f, 1.16f}, bodyW*0.78f, 0.48f, 1.02f,
                 ColorScale(paint, 0.76f));
    } else if (v->model == 4) {
        DrawCube((Vector3){0.0f, 2.12f, 0.10f}, 0.70f, 0.12f, 0.42f, C_WHITE);
        DrawCube((Vector3){-0.34f, 2.24f, 0.10f}, 0.22f, 0.12f, 0.32f, C_BLUE);
        DrawCube((Vector3){0.34f, 2.24f, 0.10f}, 0.22f, 0.12f, 0.32f, C_RED);
    } else if (v->model == 5) {
        DrawCube((Vector3){0.0f, 1.28f, 1.23f}, 1.72f, 0.82f, 1.28f, ColorScale(paint, 0.88f));
        DrawCube((Vector3){0.0f, 1.88f, 0.22f}, 1.50f, 0.20f, 1.55f, glass);
    } else if (v->model == 6) {
        DrawCube((Vector3){0.0f, 1.10f, 0.22f}, bodyW*0.72f, 0.12f, 2.00f, C_GOLD);
        DrawCube((Vector3){0.0f, 1.72f, 0.10f}, bodyW*0.74f, 0.12f, 1.65f, C_DARK);
    } else if (v->model == 7) {
        DrawCube((Vector3){0.0f, 2.05f, 0.12f}, 0.44f, 0.14f, 0.34f, C_GOLD);
        DrawCube((Vector3){0.0f, 0.91f, 0.0f}, bodyW*0.97f, 0.07f, bodyL*0.82f, C_WHITE);
    } else if (v->model == 8) {
        DrawCube((Vector3){0.0f, 1.44f, 1.70f}, 1.55f, 0.10f, 0.22f, C_DARK);
        DrawCube((Vector3){0.0f, 0.88f, -0.20f}, 0.24f, 0.08f, 3.60f, C_WHITE);
    } else if (v->model == 9) {
        DrawCube((Vector3){0.0f, 1.20f, -0.72f}, 0.62f, 0.17f, 0.80f, C_DARK);
        DrawCube((Vector3){0.0f, 1.50f, 1.60f}, 1.85f, 0.10f, 0.18f, C_DARK);
    } else if (v->model == 10) {
        DrawCube((Vector3){0.0f, 1.85f, -0.10f}, 1.45f, 0.10f, 1.90f, glass);
        DrawCube((Vector3){0.0f, 0.48f, -2.10f}, 1.70f, 0.12f, 0.10f, C_GOLD);
    } else if (v->model == 11) {
        DrawCube((Vector3){-0.78f, 1.90f, 0.10f}, 0.10f, 1.00f, 1.95f, C_DARK);
        DrawCube((Vector3){0.78f, 1.90f, 0.10f}, 0.10f, 1.00f, 1.95f, C_DARK);
        DrawCube((Vector3){0.0f, 2.40f, 0.10f}, 1.65f, 0.10f, 1.95f, C_DARK);
    }
    if (playerVehicle) {
        DrawCube((Vector3){0.0f, 1.91f, -0.15f}, 0.18f, 0.08f, 0.32f,
                 (Color){ 235, 235, 235, 255 });
    }
    rlPopMatrix();
}

static void DrawSandMeetCars(Vector3 focus) {
    if (Distance2D(focus.x, focus.z, SAND_CENTER_X, SAND_CENTER_Z) > 230.0f) return;
    static const Color paints[8] = {
        {215, 62, 55, 255}, {51, 112, 188, 255}, {236, 221, 181, 255},
        {42, 46, 55, 255}, {208, 150, 55, 255}, {83, 149, 107, 255},
        {139, 81, 161, 255}, {221, 222, 228, 255}
    };
    for (int i = 0; i < 8; i++) {
        Vehicle display = {0};
        display.position = (Vector3){SAND_CENTER_X - 84.0f + (float)(i%4)*56.0f,
                                    0.0f, SAND_CENTER_Z + 170.0f + (float)(i/4)*15.0f};
        display.yaw = PI_F;
        display.paint = paints[i];
        display.model = (i*3 + 1)%12;
        DrawVehicleModel(&display, false);
    }
}

static void InitTraffic(void) {
    static const Color paints[12] = {
        {219, 87, 70, 255}, {54, 120, 177, 255}, {224, 221, 208, 255},
        {51, 62, 73, 255}, {205, 165, 75, 255}, {93, 156, 119, 255},
        {123, 83, 160, 255}, {229, 229, 220, 255}, {231, 123, 52, 255},
        {39, 42, 51, 255}, {102, 187, 191, 255}, {170, 178, 187, 255}
    };

    for (int i = 0; i < TRAFFIC_COUNT; i++) {
        Vehicle *v = &gTraffic[i];
        int road = (i%17) - 8;
        float coord = (float)road*48.0f;
        v->speed = 6.0f + (float)(i%5)*1.55f;
        v->paint = paints[i%12];
        v->model = i%12;
        v->position.y = 0.0f;
        if ((i%2) == 0) {
            v->position.x = -CITY_HALF + 16.0f + (float)((i*73)%((int)(CITY_HALF*2.0f - 32.0f)));
            v->position.z = coord + ((i%4 < 2) ? -3.2f : 3.2f);
            v->yaw = (i%4 < 2) ? -PI_F*0.5f : PI_F*0.5f;
        } else {
            v->position.x = coord + ((i%4 < 2) ? -3.2f : 3.2f);
            v->position.z = -CITY_HALF + 16.0f + (float)((i*59)%((int)(CITY_HALF*2.0f - 32.0f)));
            v->yaw = (i%4 < 2) ? 0.0f : PI_F;
        }
    }
}

static void UpdateTraffic(float dt) {
    for (int i = 0; i < TRAFFIC_COUNT; i++) {
        Vehicle *v = &gTraffic[i];
        float dx = -sinf(v->yaw)*v->speed*dt;
        float dz = -cosf(v->yaw)*v->speed*dt;
        v->position.x += dx;
        v->position.z += dz;
        if (v->position.x > CITY_HALF + 8.0f) v->position.x = -CITY_HALF - 8.0f;
        if (v->position.x < -CITY_HALF - 8.0f) v->position.x = CITY_HALF + 8.0f;
        if (v->position.z > CITY_HALF + 8.0f) v->position.z = -CITY_HALF - 8.0f;
        if (v->position.z < -CITY_HALF - 8.0f) v->position.z = CITY_HALF + 8.0f;
    }
}

/* ---- People ------------------------------------------------------------ */
static void DrawHuman(Vector3 p, float yaw, float phase,
                      Color shirt, Color trousers, Color skin,
                      bool isPlayer, bool running) {
    rlPushMatrix();
    rlTranslatef(p.x, p.y, p.z);
    rlRotatef(yaw*RAD_TO_DEG, 0.0f, 1.0f, 0.0f);

    float swing = (running ? 32.0f : 20.0f)*sinf(phase);
    float bob = (running ? 0.075f : 0.035f)*fabsf(sinf(phase));
    Color shoe = (Color){ 34, 39, 46, 255 };
    Color hair = (Color){ 47, 39, 35, 255 };

    /* Torso, collar and belt. */
    DrawCube((Vector3){0.0f, 1.12f + bob, 0.0f}, 0.66f, 0.86f, 0.38f, shirt);
    DrawCube((Vector3){0.0f, 0.68f + bob, 0.0f}, 0.62f, 0.12f, 0.40f,
             (Color){ 54, 48, 47, 255 });
    DrawCube((Vector3){0.0f, 1.57f + bob, -0.01f}, 0.34f, 0.14f, 0.34f, skin);

    /* Head, hair and face. */
    DrawSphere((Vector3){0.0f, 1.86f + bob, 0.0f}, 0.31f, skin);
    DrawSphere((Vector3){0.0f, 2.04f + bob, 0.035f}, 0.285f, hair);
    DrawCube((Vector3){0.0f, 1.86f + bob, -0.274f}, 0.12f, 0.055f, 0.035f,
             (Color){ 44, 43, 43, 255 });

    /* Arms pivot from shoulders and swing while walking. */
    rlPushMatrix();
    rlTranslatef(-0.40f, 1.43f + bob, 0.0f);
    rlRotatef(swing, 1.0f, 0.0f, 0.0f);
    DrawCube((Vector3){0.0f, -0.34f, 0.0f}, 0.20f, 0.68f, 0.23f, shirt);
    DrawSphere((Vector3){0.0f, -0.72f, -0.01f}, 0.12f, skin);
    rlPopMatrix();

    rlPushMatrix();
    rlTranslatef(0.40f, 1.43f + bob, 0.0f);
    rlRotatef(-swing, 1.0f, 0.0f, 0.0f);
    DrawCube((Vector3){0.0f, -0.34f, 0.0f}, 0.20f, 0.68f, 0.23f, shirt);
    DrawSphere((Vector3){0.0f, -0.72f, -0.01f}, 0.12f, skin);
    rlPopMatrix();

    /* Legs and shoes. */
    rlPushMatrix();
    rlTranslatef(-0.18f, 0.67f + bob, 0.0f);
    rlRotatef(-swing*0.72f, 1.0f, 0.0f, 0.0f);
    DrawCube((Vector3){0.0f, -0.30f, 0.0f}, 0.25f, 0.60f, 0.29f, trousers);
    DrawCube((Vector3){0.0f, -0.59f, -0.10f}, 0.28f, 0.16f, 0.42f, shoe);
    rlPopMatrix();

    rlPushMatrix();
    rlTranslatef(0.18f, 0.67f + bob, 0.0f);
    rlRotatef(swing*0.72f, 1.0f, 0.0f, 0.0f);
    DrawCube((Vector3){0.0f, -0.30f, 0.0f}, 0.25f, 0.60f, 0.29f, trousers);
    DrawCube((Vector3){0.0f, -0.59f, -0.10f}, 0.28f, 0.16f, 0.42f, shoe);
    rlPopMatrix();

    if (isPlayer) {
        DrawCube((Vector3){0.0f, 1.12f + bob, 0.25f}, 0.42f, 0.48f, 0.20f,
                 (Color){ 42, 57, 80, 255 });
    }
    rlPopMatrix();
}

static void InitPedestrians(void) {
    static const Color shirts[] = {
        { 42, 116, 185, 255 }, { 195, 82, 73, 255 },
        { 68, 143, 104, 255 }, { 192, 146, 63, 255 },
        { 120, 91, 165, 255 }, { 220, 220, 213, 255 },
        { 56, 82, 105, 255 }
    };
    static const Color pants[] = {
        { 44, 55, 73, 255 }, { 84, 84, 80, 255 },
        { 90, 102, 116, 255 }, { 70, 64, 58, 255 }
    };

    for (int i = 0; i < WALKER_COUNT; i++) {
        Pedestrian *p = &gPedestrians[i];
        p->isStatic = (i >= 108);
        p->alongX = ((i%2) == 0);
        p->direction = ((i%3) == 0) ? -1 : 1;
        p->routeMin = -CITY_HALF + 24.0f;
        p->routeMax = CITY_HALF - 24.0f;
        p->speed = 1.0f + (float)(i%5)*0.27f;
        p->phase = (float)i*0.83f;
        p->shirt = shirts[i%7];
        p->trousers = pants[i%4];
        p->skin = (Color){
            (unsigned char)(183 + (i%4)*11),
            (unsigned char)(132 + (i%3)*12),
            (unsigned char)(101 + (i%4)*9), 255
        };

        if (p->isStatic) {
            int guest = i - 108;
            float a = (float)guest/20.0f*2.0f*PI_F;
            float radius = (guest%2 == 0) ? 91.0f : 99.0f;
            p->position = (Vector3){SAND_CENTER_X + cosf(a)*radius, 0.0f,
                                     SAND_CENTER_Z + sinf(a)*radius};
            p->yaw = -a + PI_F*0.5f;
            p->speed = 0.0f;
        } else if (p->alongX) {
            int road = (i/2)%17 - 8;
            float start = -CITY_HALF + 30.0f + (float)((i*41)%((int)(CITY_HALF*2.0f - 60.0f)));
            p->position = (Vector3){ start, 0.0f,
                                     (float)road*48.0f + ((i%4 < 2) ? -10.2f : 10.2f) };
            p->yaw = (p->direction > 0) ? -PI_F*0.5f : PI_F*0.5f;
        } else {
            int road = (i/2)%17 - 8;
            float start = -CITY_HALF + 30.0f + (float)((i*37)%((int)(CITY_HALF*2.0f - 60.0f)));
            p->position = (Vector3){ (float)road*48.0f + ((i%4 < 2) ? -10.2f : 10.2f),
                                     0.0f, start };
            p->yaw = (p->direction > 0) ? PI_F : 0.0f;
        }
    }
}

static void UpdatePedestrians(float dt) {
    for (int i = 0; i < WALKER_COUNT; i++) {
        Pedestrian *p = &gPedestrians[i];
        float movement = p->speed*(float)p->direction*dt;
        if (!p->isStatic) {
            if (p->alongX) p->position.x += movement;
            else p->position.z += movement;
        }
        p->phase += dt*7.0f;
        if (p->isStatic) continue;

        float value = p->alongX ? p->position.x : p->position.z;
        if (value > p->routeMax) {
            if (p->alongX) p->position.x = p->routeMin;
            else p->position.z = p->routeMin;
        } else if (value < p->routeMin) {
            if (p->alongX) p->position.x = p->routeMax;
            else p->position.z = p->routeMax;
        }
    }
}

static void DrawRainAround(Vector3 focus, float time, bool raining) {
    if (!raining) return;
    for (int i = 0; i < 72; i++) {
        float x = focus.x + (float)((i*37)%74) - 37.0f;
        float z = focus.z + (float)((i*53)%74) - 37.0f;
        float y = 5.0f + (float)((i*19)%17) + fmodf(time*29.0f + (float)i*1.7f, 8.0f);
        DrawLine3D((Vector3){x, y, z}, (Vector3){x - 0.30f, y - 1.25f, z + 0.10f},
                   Fade((Color){190, 212, 225, 255}, 0.48f));
    }
}

static const char *GetCarName(int model) {
    static const char *names[12] = {
        "CITY SEDAN", "SPORT COUPE", "OFFROAD SUV", "PICKUP",
        "POLICE UNIT", "CARGO VAN", "CLASSIC GT", "CITY TAXI",
        "RALLY HATCH", "MUSCLE GT", "ELECTRIC LUX", "DESERT BUGGY"
    };
    return names[model%12];
}

/* ---- Missions ---------------------------------------------------------- */
static const Vector3 gMissionPoints[MISSION_COUNT] = {
    {   45.0f, 0.0f,   11.0f }, {   98.0f, 0.0f,  -48.0f },
    {  -44.0f, 0.0f,   60.0f }, {  -96.0f, 0.0f,  -52.0f },
    {  145.0f, 0.0f, -105.0f }, { -145.0f, 0.0f,  144.0f },
    {  285.0f, 0.0f,  -90.0f }, { -286.0f, 0.0f, -150.0f },
    {  250.0f, 0.0f,  240.0f }, { -240.0f, 0.0f,  285.0f },
    {  565.0f, 0.0f,  110.0f }, {  690.0f, 0.0f,  230.0f }
};

static void DrawMissionMarker(Vector3 p, float time, bool pickup) {
    float pulse = 1.0f + 0.16f*sinf(time*3.0f);
    Color c = pickup ? C_MINT : C_GOLD;
    DrawCylinder((Vector3){p.x, 0.13f, p.z}, 1.35f*pulse, 1.35f*pulse,
                 0.18f, 24, Fade(c, 0.55f));
    DrawCylinderWires((Vector3){p.x, 0.15f, p.z}, 1.5f*pulse, 1.5f*pulse,
                      0.22f, 24, c);
    DrawCylinder((Vector3){p.x, 1.35f, p.z}, 0.12f, 0.12f, 2.2f,
                 12, Fade(c, 0.36f));
    DrawSphere((Vector3){p.x, 2.65f + 0.15f*sinf(time*2.0f), p.z},
               0.45f*pulse, c);
}

/* ---- Minimap and HUD --------------------------------------------------- */
static void DrawMinimap(Rectangle map, Vector3 actor, Vector3 target,
                        bool hasTarget, bool inCar) {
    DrawPanel(map, (Color){ 11, 19, 28, 226 }, (Color){ 153, 174, 188, 220 });
    Rectangle inner = {map.x + 9.0f, map.y + 9.0f, map.width - 18.0f, map.height - 18.0f};
    DrawRectangle((int)inner.x, (int)inner.y, (int)inner.width, (int)inner.height,
                  (Color){ 47, 78, 69, 255 });

    float sx = inner.width/(WORLD_HALF*2.0f);
    float sz = inner.height/(WORLD_HALF*2.0f);
    Rectangle desert = {
        inner.x + (SAND_X_MIN + WORLD_HALF)*sx,
        inner.y + (SAND_Z_MIN + WORLD_HALF)*sz,
        (SAND_X_MAX - SAND_X_MIN)*sx,
        (SAND_Z_MAX - SAND_Z_MIN)*sz
    };
    DrawRectangle((int)desert.x, (int)desert.y, (int)desert.width,
                  (int)desert.height, (Color){196, 157, 101, 255});

    for (int i = -4; i <= 4; i++) {
        float x = inner.x + (float)(i + 4)*(inner.width/8.0f);
        float y = inner.y + (float)(i + 4)*(inner.height/8.0f);
        DrawRectangle((int)(x - 1.0f), (int)inner.y, 2, (int)inner.height,
                      (Color){ 85, 91, 96, 255 });
        DrawRectangle((int)inner.x, (int)(y - 1.0f), (int)inner.width, 2,
                      (Color){ 85, 91, 96, 255 });
    }

    float px = inner.x + (actor.x + WORLD_HALF)*sx;
    float py = inner.y + (actor.z + WORLD_HALF)*sz;
    if (hasTarget) {
        float tx = inner.x + (target.x + WORLD_HALF)*sx;
        float ty = inner.y + (target.z + WORLD_HALF)*sz;
        DrawCircle((int)tx, (int)ty, 3.8f, C_GOLD);
    }
    DrawCircle((int)px, (int)py, inCar ? 4.6f : 3.5f, C_MINT);
    DrawCircleLines((int)px, (int)py, 6.0f, C_WHITE);
}

static void DrawGameHUD(int screenW, int screenH, int cash,
                        int delivered, int missionStep, float speed,
                        bool inCar, bool nearCar, bool nearMission,
                        Vector3 actor, Vector3 target, bool hasTarget,
                        MoveInput stick, float stickX, float stickY,
                        float time, bool radioEnabled, int radioTrack, int carModel,
                        float driftScore, int bestDrift, float nightAmount, bool raining,
                        int weaponIndex, int ammo, int reserveAmmo, int points) {
    DrawPanel((Rectangle){14.0f, 14.0f, 312.0f, 178.0f}, C_HUD, (Color){111, 142, 160, 220});
    DrawArabicLabel(AR_CITY, (Rectangle){22.0f, 18.0f, 150.0f, 25.0f}, C_MINT);
    DrawArabicLabel(GetArabicDistrictLabel(actor), (Rectangle){22.0f, 48.0f, 128.0f, 22.0f}, Fade(C_WHITE, 0.92f));
    DrawArabicLabel(AR_CASH, (Rectangle){22.0f, 77.0f, 54.0f, 20.0f}, C_GOLD);
    DrawText(TextFormat("$%d", cash), 84, 79, 17, C_GOLD);
    DrawArabicLabel(AR_MISSIONS, (Rectangle){22.0f, 105.0f, 54.0f, 20.0f}, C_WHITE);
    DrawText(TextFormat("%d / 6", delivered), 84, 107, 16, C_WHITE);
    DrawArabicLabel(AR_POINTS, (Rectangle){174.0f, 77.0f, 55.0f, 20.0f}, C_MINT);
    DrawText(TextFormat("%d", points), 232, 79, 16, C_MINT);
    DrawArabicLabel(AR_LEVEL, (Rectangle){174.0f, 105.0f, 55.0f, 20.0f}, C_WHITE);
    DrawText(TextFormat("%d", points/500 + 1), 232, 107, 16, C_WHITE);
    if (!inCar) {
        DrawArabicLabel((weaponIndex == 0) ? AR_PULSE : ((weaponIndex == 1) ? AR_RIFLE : AR_SHOCK),
                        (Rectangle){22.0f, 135.0f, 103.0f, 24.0f}, C_GOLD);
        DrawArabicLabel(AR_AMMO, (Rectangle){132.0f, 135.0f, 40.0f, 24.0f}, C_WHITE);
        DrawText(TextFormat("%d/%d", ammo, reserveAmmo), 176, 140, 13, C_WHITE);
    }

    Rectangle map = {(float)screenW - 160.0f, 14.0f, 146.0f, 146.0f};
    DrawMinimap(map, actor, target, hasTarget, inCar);
    DrawPanel((Rectangle){(float)screenW - 160.0f, 169.0f, 146.0f, 42.0f}, C_HUD,
              Fade(C_WHITE, 0.30f));
    DrawArabicLabel(AR_SPEED, (Rectangle){(float)screenW - 151.0f, 171.0f, 45.0f, 18.0f}, C_WHITE);
    if (inCar) {
        DrawText(TextFormat("%03d", (int)(fabsf(speed)*4.0f)), screenW - 105, 174, 13, C_WHITE);
        DrawArabicLabel(AR_KMH, (Rectangle){(float)screenW - 61.0f, 174.0f, 44.0f, 15.0f}, C_WHITE);
    } else {
        DrawArabicLabel(AR_FOOT, (Rectangle){(float)screenW - 106.0f, 173.0f, 90.0f, 18.0f}, C_WHITE);
    }
    DrawArabicLabel(AR_DRIFT, (Rectangle){(float)screenW - 151.0f, 191.0f, 42.0f, 16.0f}, C_GOLD);
    DrawText(TextFormat("%05d / %05d", (int)driftScore, bestDrift), screenW - 106, 194, 9, C_GOLD);
    DrawPanel((Rectangle){(float)screenW - 160.0f, 216.0f, 146.0f, 38.0f},
              (Color){12, 20, 31, 220}, Fade(C_MINT, 0.35f));
    DrawArabicLabel(AR_RADIO, (Rectangle){(float)screenW - 151.0f, 221.0f, 45.0f, 21.0f},
                    radioEnabled ? C_MINT : C_RED);
    DrawText(radioEnabled ? TextFormat("%02d / %02d", radioTrack + 1, GetRadioTrackCount()) : "OFF",
             screenW - 105, 228, 11, radioEnabled ? C_MINT : C_RED);
    DrawPanel((Rectangle){(float)screenW - 160.0f, 260.0f, 146.0f, 38.0f},
              (Color){12, 20, 31, 220}, Fade(C_GOLD, 0.35f));
    DrawArabicLabel(AR_GARAGE, (Rectangle){(float)screenW - 151.0f, 265.0f, 45.0f, 21.0f}, C_GOLD);
    DrawArabicLabel(AR_VEHICLE, (Rectangle){(float)screenW - 105.0f, 267.0f, 43.0f, 17.0f}, C_GOLD);
    DrawText(TextFormat("%02d", carModel + 1), screenW - 58, 271, 11, C_GOLD);
    DrawArabicLabel(AR_WEATHER, (Rectangle){(float)screenW - 151.0f, 288.0f, 42.0f, 17.0f}, C_WHITE);
    DrawArabicLabel(raining ? AR_RAIN : (nightAmount > 0.55f ? AR_NIGHT : AR_CLEAR),
                    (Rectangle){(float)screenW - 105.0f, 288.0f, 88.0f, 17.0f},
                    raining ? C_BLUE : (nightAmount > 0.55f ? C_MINT : C_WHITE));

    if (missionStep < MISSION_COUNT) {
        DrawPanel((Rectangle){(float)screenW*0.5f - 128.0f, 16.0f, 256.0f, 44.0f},
                  (Color){14, 23, 35, 210}, Fade(C_GOLD, 0.7f));
        DrawArabicLabel(((missionStep%2) == 0) ? AR_PICKUP : AR_DELIVER,
                        (Rectangle){(float)screenW*0.5f - 118.0f, 23.0f, 236.0f, 30.0f}, C_GOLD);
    } else {
        DrawPanel((Rectangle){(float)screenW*0.5f - 128.0f, 16.0f, 256.0f, 44.0f},
                  (Color){14, 23, 35, 210}, Fade(C_MINT, 0.7f));
        DrawArabicLabel(AR_ALL_DONE, (Rectangle){(float)screenW*0.5f - 118.0f, 23.0f, 236.0f, 30.0f}, C_MINT);
    }

    /* Virtual analogue stick and Arabic control labels. */
    DrawCircle((int)stickX, (int)stickY, 62.0f, Fade(C_DARK, 0.48f));
    DrawCircleLines((int)stickX, (int)stickY, 62.0f, Fade(C_WHITE, 0.85f));
    DrawCircle((int)(stickX + stick.x*34.0f), (int)(stickY - stick.y*34.0f),
               24.0f, Fade(C_WHITE, 0.88f));

    Rectangle actionRect = {(float)screenW - 118.0f, (float)screenH - 254.0f, 100.0f, 58.0f};
    Rectangle jumpRect = {(float)screenW - 118.0f, (float)screenH - 326.0f, 100.0f, 58.0f};
    Rectangle gasRect = {(float)screenW - 112.0f, (float)screenH - 144.0f, 94.0f, 82.0f};
    Rectangle brakeRect = {(float)screenW - 218.0f, (float)screenH - 132.0f, 94.0f, 72.0f};
    Rectangle sprintRect = {18.0f, (float)screenH - 118.0f, 90.0f, 48.0f};
    Rectangle driftRect = {(float)screenW - 218.0f, (float)screenH - 216.0f, 94.0f, 56.0f};
    Rectangle fireRect = {(float)screenW*0.59f, (float)screenH - 398.0f, 100.0f, 58.0f};
    Rectangle weaponRect = {(float)screenW*0.46f, (float)screenH - 398.0f, 94.0f, 58.0f};
    int actionLabel = inCar ? AR_EXIT : (nearCar ? AR_ENTER_CAR : AR_INTERACT);
    DrawArabicButton(actionRect, actionLabel, C_BLUE, nearCar || nearMission);
    DrawArabicButton(jumpRect, inCar ? AR_CAMERA : AR_JUMP,
                     (Color){67, 113, 168, 255}, false);
    DrawArabicButton(gasRect, AR_GAS, (Color){34, 139, 91, 255}, false);
    DrawArabicButton(brakeRect, AR_BRAKE, (Color){163, 68, 64, 255}, false);
    DrawArabicButton(sprintRect, inCar ? AR_NITRO : AR_MOVE,
                     (Color){121, 94, 164, 255}, false);
    if (inCar) DrawArabicButton(driftRect, AR_HANDBRAKE, (Color){169, 104, 49, 255}, false);
    else {
        DrawArabicButton(fireRect, AR_FIRE, (Color){172, 63, 54, 255}, true);
        DrawArabicButton(weaponRect, AR_SWITCH_WEAPON, (Color){71, 91, 126, 255}, false);
    }
    DrawArabicLabel(AR_MOVE, (Rectangle){16.0f, (float)screenH - 26.0f, 130.0f, 20.0f}, Fade(C_WHITE, 0.86f));
    (void)time;
}
/* ---- Drift, tyre smoke and original radio ------------------------------ */
static void SpawnDriftEffects(const Vehicle *car) {
    for (int side = -1; side <= 1; side += 2) {
        float localX = (float)side*0.88f;
        float localZ = 1.48f;
        float wx = car->position.x + cosf(car->yaw)*localX + sinf(car->yaw)*localZ;
        float wz = car->position.z - sinf(car->yaw)*localX + cosf(car->yaw)*localZ;
        SmokeParticle *p = &gSmoke[gSmokeCursor++ % SMOKE_COUNT];
        bool sand = IsInSandArena(car->position);
        p->position = (Vector3){wx, sand ? 0.18f : 0.38f, wz};
        p->velocity = (Vector3){(float)side*(sand ? 0.52f : 0.35f),
                                sand ? 0.92f : 0.72f, 0.0f};
        p->life = sand ? 1.35f : 0.95f;
        p->maxLife = p->life;
        p->size = sand ? 0.46f : 0.32f;
        SkidMark *m = &gSkids[gSkidCursor++ % SKID_COUNT];
        m->a = (Vector3){wx - 0.18f, 0.12f, wz};
        m->b = (Vector3){wx + 0.18f, 0.12f, wz};
        m->life = 13.0f;
    }
}

static void UpdateDriftEffects(float dt) {
    for (int i = 0; i < SMOKE_COUNT; i++) {
        SmokeParticle *p = &gSmoke[i];
        if (p->life <= 0.0f) continue;
        p->life -= dt;
        p->position.x += p->velocity.x*dt;
        p->position.y += p->velocity.y*dt;
        p->position.z += p->velocity.z*dt;
        p->size += dt*0.28f;
    }
    for (int i = 0; i < SKID_COUNT; i++) {
        if (gSkids[i].life > 0.0f) gSkids[i].life -= dt;
    }
}

static void DrawDriftEffects(void) {
    for (int i = 0; i < SKID_COUNT; i++) {
        if (gSkids[i].life > 0.0f) {
            float alpha = ClampF(gSkids[i].life/13.0f, 0.0f, 0.45f);
            Color mark = IsInSandArena(gSkids[i].a) ? (Color){126, 91, 55, 255} :
                         (Color){22, 25, 29, 255};
            DrawLine3D(gSkids[i].a, gSkids[i].b, Fade(mark, alpha));
        }
    }
    for (int i = 0; i < SMOKE_COUNT; i++) {
        SmokeParticle *p = &gSmoke[i];
        if (p->life > 0.0f) {
            float alpha = ClampF(p->life/p->maxLife, 0.0f, 1.0f)*0.38f;
            Color puff = IsInSandArena(p->position) ? (Color){207, 171, 119, 255} :
                         (Color){191, 199, 205, 255};
            DrawSphere(p->position, p->size, Fade(puff, alpha));
        }
    }
}

static Sound CreateOriginalRadioLoop(int style) {
    const unsigned int rate = 22050;
    const float bpm = (style == 0) ? 112.0f : ((style == 1) ? 96.0f : 124.0f);
    const float beat = 60.0f/bpm;
    const unsigned int frames = (unsigned int)(rate*beat*16.0f);
    short *pcm = (short *)MemAlloc(frames*sizeof(short));
    if (pcm == NULL) return (Sound){0};
    uint32_t noise = 0x51A7C3u;
    const float bassNotes[3][4] = {
        {55.0f, 65.41f, 73.42f, 82.41f},
        {49.0f, 58.27f, 65.41f, 73.42f},
        {61.74f, 73.42f, 82.41f, 92.50f}
    };
    const float leadNotes[3][8] = {
        {440.0f, 523.25f, 493.88f, 587.33f, 392.0f, 440.0f, 523.25f, 659.25f},
        {329.63f, 392.0f, 440.0f, 493.88f, 392.0f, 329.63f, 293.66f, 392.0f},
        {493.88f, 587.33f, 659.25f, 783.99f, 659.25f, 587.33f, 523.25f, 659.25f}
    };
    if (style < 0 || style > 2) style = 0;
    for (unsigned int i = 0; i < frames; i++) {
        float t = (float)i/(float)rate;
        float beatPos = fmodf(t, beat);
        unsigned int beatIndex = (unsigned int)(t/beat);
        unsigned int halfStep = (unsigned int)(t/(beat*0.5f));
        float kickEnv = expf(-beatPos*18.0f);
        float kick = ((beatIndex%4u)==0u || (beatIndex%4u)==2u) ?
                     sinf(2.0f*PI_F*(47.0f - 13.0f*beatPos)*beatPos)*kickEnv*0.62f : 0.0f;
        float snareEnv = expf(-beatPos*12.0f);
        noise = noise*1664525u + 1013904223u;
        float n = ((float)((noise>>8)&65535u)/32767.5f)-1.0f;
        float snare = ((beatIndex%4u)==1u || (beatIndex%4u)==3u) ? n*snareEnv*0.11f : 0.0f;
        float hat = ((halfStep%2u)==1u) ? n*0.035f*expf(-fmodf(t,beat*0.5f)*38.0f) : 0.0f;
        unsigned int bar = (beatIndex/4u)%4u;
        float bass = sinf(2.0f*PI_F*bassNotes[style][bar]*t)*0.18f;
        unsigned int note = (unsigned int)(t/(beat*0.5f))%8u;
        float leadEnv = 0.35f + 0.65f*expf(-fmodf(t,beat*0.5f)*2.4f);
        float lead = (sinf(2.0f*PI_F*leadNotes[style][note]*t) +
                      0.35f*sinf(4.0f*PI_F*leadNotes[style][note]*t))*0.055f*leadEnv;
        float pad = (sinf(2.0f*PI_F*(bassNotes[style][0]*2.0f + (float)bar*13.0f)*t) +
                     sinf(2.0f*PI_F*(bassNotes[style][2]*3.0f + (float)bar*9.0f)*t))*0.035f;
        float sample = ClampF(kick + snare + hat + bass + lead + pad, -0.92f, 0.92f);
        pcm[i] = (short)(sample*32767.0f);
    }
    Wave wave = {frames, rate, 16, 1, pcm};
    Sound result = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return result;
}

static Sound CreateEngineLoop(void) {
    const unsigned int rate = 22050;
    const unsigned int frames = rate*2u;
    short *pcm = (short *)MemAlloc(frames*sizeof(short));
    if (pcm == NULL) return (Sound){0};
    for (unsigned int i = 0; i < frames; i++) {
        float t = (float)i/(float)rate;
        float envelope = 0.65f + 0.35f*sinf(2.0f*PI_F*t*3.0f);
        float engine = sinf(2.0f*PI_F*58.0f*t) + 0.35f*sinf(2.0f*PI_F*116.0f*t);
        float pulse = 0.16f*sinf(2.0f*PI_F*19.0f*t);
        pcm[i] = (short)(ClampF((engine*0.12f + pulse)*envelope, -0.7f, 0.7f)*32767.0f);
    }
    Wave wave = {frames, rate, 16, 1, pcm};
    Sound result = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return result;
}

static void LoadMusicLibrary(void) {
    gMusicCount = 0;
    char *playlist = LoadFileText("music/playlist.txt");
    if (playlist == NULL) return;
    char *line = strtok(playlist, "\r\n");
    while (line != NULL && gMusicCount < MUSIC_CAPACITY) {
        if (line[0] != '#' && line[0] != '\0') {
            char *separator = strchr(line, '|');
            if (separator != NULL) {
                *separator = '\0';
                const char *fileName = line;
                const char *title = separator + 1;
                char path[192];
                snprintf(path, sizeof(path), "music/%s", fileName);
                Music track = LoadMusicStream(path);
                if (IsMusicValid(track)) {
                    gMusicTracks[gMusicCount] = track;
                    snprintf(gMusicTitles[gMusicCount], sizeof(gMusicTitles[gMusicCount]),
                             "%s", (title[0] != '\0') ? title : fileName);
                    SetMusicVolume(gMusicTracks[gMusicCount], 0.32f);
                    gMusicCount++;
                }
            }
        }
        line = strtok(NULL, "\r\n");
    }
    UnloadFileText(playlist);
}

static int GetRadioTrackCount(void) {
    return (gMusicCount > 0) ? gMusicCount : 3;
}

static void StopRadioPlayback(void) {
    if (gMusicCount > 0) {
        for (int i = 0; i < gMusicCount; i++)
            if (IsMusicStreamPlaying(gMusicTracks[i])) StopMusicStream(gMusicTracks[i]);
    } else {
        for (int i = 0; i < 3; i++)
            if (IsSoundPlaying(gRadioSounds[i])) StopSound(gRadioSounds[i]);
    }
}

static void UpdateAudio(bool inCar, float speed, bool radioEnabled) {
    if (!gAudioReady) return;
    if (gMusicCount > 0) {
        if (radioEnabled) {
            Music *track = &gMusicTracks[gRadioIndex % gMusicCount];
            UpdateMusicStream(*track);
            if (!IsMusicStreamPlaying(*track)) {
                SeekMusicStream(*track, 0.0f);
                PlayMusicStream(*track);
            }
        } else {
            Music *track = &gMusicTracks[gRadioIndex % gMusicCount];
            if (IsMusicStreamPlaying(*track)) StopMusicStream(*track);
        }
    } else if (radioEnabled) {
        if (!IsSoundPlaying(gRadioSounds[gRadioIndex % 3])) PlaySound(gRadioSounds[gRadioIndex % 3]);
    } else if (IsSoundPlaying(gRadioSounds[gRadioIndex % 3])) StopSound(gRadioSounds[gRadioIndex % 3]);
    if (IsSoundValid(gEngineSound) && inCar && fabsf(speed) > 0.8f) {
        if (!IsSoundPlaying(gEngineSound)) PlaySound(gEngineSound);
        SetSoundPitch(gEngineSound, 0.78f + ClampF(fabsf(speed)/38.0f, 0.0f, 1.0f)*0.85f);
        SetSoundVolume(gEngineSound, 0.12f);
    } else if (IsSoundValid(gEngineSound) && IsSoundPlaying(gEngineSound)) {
        StopSound(gEngineSound);
    }
}

/* ---- Arcade training range --------------------------------------------- */
static void InitTrainingRange(void) {
    for (int i = 0; i < TARGET_COUNT; i++) {
        gTargets[i].position = (Vector3){
            SAND_CENTER_X - 84.0f + (float)i*24.0f,
            1.35f,
            -78.0f
        };
        gTargets[i].active = true;
        gTargets[i].respawnTimer = 0.0f;
    }
}

static void UpdateTrainingTargets(float dt) {
    for (int i = 0; i < TARGET_COUNT; i++) {
        TrainingTarget *target = &gTargets[i];
        if (!target->active) {
            target->respawnTimer -= dt;
            if (target->respawnTimer <= 0.0f) {
                target->active = true;
                target->respawnTimer = 0.0f;
            }
        }
    }
    if (gWeaponCooldown > 0.0f) gWeaponCooldown = fmaxf(0.0f, gWeaponCooldown - dt);
}

static void DrawTrainingRange(Vector3 focus, float time) {
    if (Distance2D(focus.x, focus.z, SAND_CENTER_X, -42.0f) > 210.0f) return;
    DrawCube((Vector3){SAND_CENTER_X, -0.015f, -42.0f}, 220.0f, 0.12f, 94.0f,
             (Color){174, 145, 102, 255});
    DrawCube((Vector3){SAND_CENTER_X, 0.045f, -88.0f}, 220.0f, 0.10f, 2.0f,
             (Color){93, 82, 68, 255});
    for (int i = 0; i < TARGET_COUNT; i++) {
        const TrainingTarget *target = &gTargets[i];
        float x = target->position.x;
        float z = target->position.z;
        DrawCylinder((Vector3){x, 0.62f, z}, 0.18f, 0.18f, 1.24f, 8,
                     (Color){83, 87, 91, 255});
        if (target->active) {
            float pulse = 0.92f + 0.08f*sinf(time*3.0f + (float)i);
            DrawCube((Vector3){x, 1.65f*pulse, z}, 1.55f, 1.75f*pulse, 0.28f,
                     (Color){194, 72, 58, 255});
            DrawCube((Vector3){x, 1.65f*pulse, z - 0.16f}, 0.86f, 1.02f*pulse, 0.04f,
                     (Color){228, 218, 190, 255});
            DrawSphere((Vector3){x, 1.65f*pulse, z - 0.22f}, 0.22f,
                       (Color){54, 121, 163, 255});
        } else {
            DrawCube((Vector3){x, 0.16f, z}, 1.2f, 0.25f, 0.28f,
                     (Color){88, 93, 97, 255});
        }
    }
}

static void FireTrainingWeapon(Vector3 player, float yaw, int *cash, int *points,
                               char *message, size_t messageSize) {
    static const float rangeByWeapon[3] = {42.0f, 64.0f, 34.0f};
    static const float coneByWeapon[3] = {0.86f, 0.90f, 0.68f};
    static const int rewardPoints[3] = {80, 100, 150};
    static const int rewardCash[3] = {20, 30, 45};
    if (gWeaponAmmo[gWeaponIndex] <= 0) {
        if (gWeaponReserve[gWeaponIndex] > 0) {
            snprintf(message, messageSize, "NO AMMO - PRESS R TO RELOAD");
        } else {
            snprintf(message, messageSize, "NO AMMO LEFT");
        }
        return;
    }
    gWeaponAmmo[gWeaponIndex]--;

    float dirX = -sinf(yaw);
    float dirZ = -cosf(yaw);
    int bestIndex = -1;
    float bestDistance = rangeByWeapon[gWeaponIndex] + 1.0f;
    for (int i = 0; i < TARGET_COUNT; i++) {
        const TrainingTarget *target = &gTargets[i];
        if (!target->active) continue;
        float dx = target->position.x - player.x;
        float dz = target->position.z - player.z;
        float distance = sqrtf(dx*dx + dz*dz);
        if (distance < 2.0f || distance > rangeByWeapon[gWeaponIndex]) continue;
        float dot = (dx*dirX + dz*dirZ)/distance;
        if (dot >= coneByWeapon[gWeaponIndex] && distance < bestDistance) {
            bestDistance = distance;
            bestIndex = i;
        }
    }
    if (bestIndex >= 0) {
        gTargets[bestIndex].active = false;
        gTargets[bestIndex].respawnTimer = 4.0f;
        *points += rewardPoints[gWeaponIndex];
        *cash += rewardCash[gWeaponIndex];
        snprintf(message, messageSize, "TARGET HIT  +%d PTS", rewardPoints[gWeaponIndex]);
    } else {
        snprintf(message, messageSize, "PRACTICE RANGE - TARGET MISSED");
    }
}

/* ---- Main game --------------------------------------------------------- */
int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(1280, 720, "CITY LIFE 3D - NOVA BAY V5");
#if !defined(PLATFORM_ANDROID)
    /* Desktop builds run from the project root; Android uses APK assets as cwd. */
    ChangeDirectory("assets");
#endif
    SetTargetFPS(60);

    InitAudioDevice();
    gArabicAtlas = LoadTexture("ui/arabic-ui.png");
    gAudioDeviceReady = IsAudioDeviceReady();
    gAudioReady = gAudioDeviceReady;
    if (gAudioDeviceReady) {
        for (int track = 0; track < 3; track++) {
            gRadioSounds[track] = CreateOriginalRadioLoop(track);
            if (IsSoundValid(gRadioSounds[track])) SetSoundVolume(gRadioSounds[track], 0.22f);
        }
        gEngineSound = CreateEngineLoop();
        LoadMusicLibrary();
        bool fallbackReady = IsSoundValid(gRadioSounds[0]) || IsSoundValid(gRadioSounds[1]) ||
                             IsSoundValid(gRadioSounds[2]);
        gAudioReady = (gMusicCount > 0) || fallbackReady;
        if (IsSoundValid(gEngineSound)) SetSoundVolume(gEngineSound, 0.12f);
    }
    gRadioEnabled = gAudioReady;

    BuildCity();
    InitTrainingRange();
    InitPedestrians();
    InitTraffic();
    InitWorldTrees();

    GameState state = GAME_TITLE;
    float worldTime = 0.0f;
    float walkPhase = 0.0f;
    float jumpVelocity = 0.0f;
    bool grounded = true;
    bool inCar = false;
    int cameraMode = 0;
    int cash = 0;
    int missionStep = 0;
    int delivered = 0;
    float messageTimer = 0.0f;
    float driftComboTimer = 0.0f;
    bool raining = false;
    float weatherTimer = 0.0f;
    char message[96] = "WELCOME TO NOVA BAY MEGA WORLD";

    Vector3 player = { 0.0f, 0.0f, 12.0f };
    float playerYaw = 0.0f;
    Vehicle playerCar = {
        { 0.0f, 0.0f, 4.0f }, 0.0f, 0.0f, 0.0f,
        { 206, 58, 67, 255 }, 0
    };

    Camera3D camera = {0};
    camera.position = (Vector3){ -8.0f, 7.0f, 20.0f };
    camera.target = (Vector3){ 0.0f, 1.2f, 12.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 58.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    bool actionWasDown = false;
    bool jumpWasDown = false;
    bool pauseWasDown = false;
    bool startWasDown = false;
    bool resumeWasDown = false;
    bool restartWasDown = false;
    bool radioWasDown = false;
    bool garageWasDown = false;
    bool muteWasDown = false;
    bool fireWasDown = false;
    bool weaponWasDown = false;
    int points = 0;
    static const Color playerPaints[12] = {
        {206, 58, 67, 255}, {46, 106, 186, 255}, {71, 134, 102, 255},
        {198, 151, 65, 255}, {220, 224, 229, 255}, {80, 94, 115, 255},
        {65, 58, 52, 255}, {224, 192, 54, 255}, {231, 123, 52, 255},
        {39, 42, 51, 255}, {102, 187, 191, 255}, {170, 178, 187, 255}
    };

    while (!WindowShouldClose()) {
        float dt = ClampF(GetFrameTime(), 0.0f, 0.045f);
        worldTime += dt;
        float dayPhase = fmodf(worldTime, 240.0f)/240.0f;
        float daylight = 0.5f + 0.5f*cosf(dayPhase*2.0f*PI_F);
        gNightAmount = 1.0f - daylight;
        weatherTimer += dt;
        if (weatherTimer > 95.0f) { raining = !raining; weatherTimer = 0.0f; }
        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();

        Rectangle actionRect = {(float)screenW - 118.0f, (float)screenH - 254.0f, 100.0f, 58.0f};
        Rectangle jumpRect = {(float)screenW - 118.0f, (float)screenH - 326.0f, 100.0f, 58.0f};
        Rectangle gasRect = {(float)screenW - 112.0f, (float)screenH - 144.0f, 94.0f, 82.0f};
        Rectangle brakeRect = {(float)screenW - 218.0f, (float)screenH - 132.0f, 94.0f, 72.0f};
        Rectangle sprintRect = {18.0f, (float)screenH - 118.0f, 90.0f, 48.0f};
        Rectangle pauseRect = {(float)screenW - 54.0f, 304.0f, 40.0f, 38.0f};
        Rectangle radioRect = {(float)screenW - 160.0f, 216.0f, 146.0f, 38.0f};
        Rectangle garageRect = {(float)screenW - 160.0f, 260.0f, 146.0f, 38.0f};
        Rectangle muteRect = {(float)screenW - 108.0f, 304.0f, 48.0f, 38.0f};
        Rectangle driftRect = {(float)screenW - 218.0f, (float)screenH - 216.0f, 94.0f, 56.0f};
        Rectangle fireRect = {(float)screenW*0.59f, (float)screenH - 398.0f, 100.0f, 58.0f};
        Rectangle weaponRect = {(float)screenW*0.46f, (float)screenH - 398.0f, 94.0f, 58.0f};
        Rectangle playRect = {(float)screenW*0.5f - 128.0f, (float)screenH*0.62f, 256.0f, 66.0f};
        Rectangle resumeRect = {(float)screenW*0.5f - 115.0f, (float)screenH*0.53f, 230.0f, 58.0f};
        Rectangle restartRect = {(float)screenW*0.5f - 115.0f, (float)screenH*0.63f, 230.0f, 58.0f};

        if (state == GAME_TITLE) {
            bool start = TouchPressed(playRect, &startWasDown) || IsKeyPressed(KEY_ENTER);
            if (start) {
                state = GAME_PLAYING;
                messageTimer = 2.6f;
                snprintf(message, sizeof(message), "YOUR FIRST DELIVERY IS READY");
            }
        } else if (state == GAME_PAUSED) {
            bool resume = TouchPressed(resumeRect, &resumeWasDown) || IsKeyPressed(KEY_ENTER);
            bool restart = TouchPressed(restartRect, &restartWasDown);
            if (resume) state = GAME_PLAYING;
            if (restart) {
                player = (Vector3){0.0f, 0.0f, 12.0f};
                playerYaw = 0.0f;
                playerCar.position = (Vector3){0.0f, 0.0f, 4.0f};
                playerCar.speed = 0.0f;
                playerCar.yaw = 0.0f;
                cash = 0;
                points = 0;
                gWeaponAmmo[0] = 45; gWeaponAmmo[1] = 120; gWeaponAmmo[2] = 12;
                gWeaponReserve[0] = 135; gWeaponReserve[1] = 360; gWeaponReserve[2] = 60;
                gWeaponIndex = 0;
                gWeaponCooldown = 0.0f;
                InitTrainingRange();
                missionStep = 0;
                delivered = 0;
                inCar = false;
                jumpVelocity = 0.0f;
                grounded = true;
                state = GAME_PLAYING;
            }
        } else {
            bool pauseDown = TouchHeld(pauseRect) || IsKeyDown(KEY_ESCAPE);
            bool pausePressed = pauseDown && !pauseWasDown;
            pauseWasDown = pauseDown;
            if (pausePressed) state = GAME_PAUSED;
            if (TouchPressed(radioRect, &radioWasDown)) {
                StopRadioPlayback();
                gRadioIndex = (gRadioIndex + 1)%GetRadioTrackCount();
                gRadioEnabled = true;
                snprintf(message, sizeof(message), "RADIO: TRACK CHANGED");
                messageTimer = 2.0f;
            }
            if (TouchPressed(muteRect, &muteWasDown) || IsKeyPressed(KEY_M)) gRadioEnabled = !gRadioEnabled;
            if (TouchPressed(garageRect, &garageWasDown) || IsKeyPressed(KEY_C)) {
                playerCar.model = (playerCar.model + 1)%12;
                playerCar.paint = playerPaints[playerCar.model];
                snprintf(message, sizeof(message), "GARAGE: %s", GetCarName(playerCar.model));
                messageTimer = 2.0f;
            }
        }

        Vector3 actor = inCar ? playerCar.position : player;
        Vector3 missionTarget = (missionStep < MISSION_COUNT) ? gMissionPoints[missionStep] : actor;
        bool hasTarget = missionStep < MISSION_COUNT;
        bool nearCar = !inCar && Distance2D(player.x, player.z,
                                             playerCar.position.x, playerCar.position.z) < 5.0f;
        bool nearMission = hasTarget &&
            Distance2D(actor.x, actor.z, missionTarget.x, missionTarget.z) < 5.2f;

        MoveInput stick = {0.0f, 0.0f};
        float stickX = (float)screenW*0.165f;
        float stickY = (float)screenH*0.79f;
        bool gasDown = false;
        bool brakeDown = false;
        bool sprintDown = false;
        bool nitroDown = false;
        bool handbrakeDown = false;
        bool jumpPressed = false;
        bool actionPressed = false;

        if (state == GAME_PLAYING) {
            /* Keyboard support for desktop testing. */
            if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) stick.x -= 1.0f;
            if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) stick.x += 1.0f;
            if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) stick.y += 1.0f;
            if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) stick.y -= 1.0f;

            /* Touch joystick is on the lower left; other buttons are right side. */
            for (int t = 0; t < GetTouchPointCount(); t++) {
                Vector2 p = GetTouchPosition(t);
                if (p.x < (float)screenW*0.46f && p.y > (float)screenH*0.40f) {
                    float dx = p.x - stickX;
                    float dy = p.y - stickY;
                    float len = sqrtf(dx*dx + dy*dy);
                    if (len > 8.0f) {
                        float scale = (len > 62.0f) ? 62.0f/len : 1.0f;
                        stick.x = ClampF(dx*scale/62.0f, -1.0f, 1.0f);
                        stick.y = ClampF(-dy*scale/62.0f, -1.0f, 1.0f);
                    }
                }
            }
            gasDown = TouchHeld(gasRect) || IsKeyDown(KEY_LEFT_SHIFT);
            brakeDown = TouchHeld(brakeRect) || IsKeyDown(KEY_SPACE);
            sprintDown = !inCar && (TouchHeld(sprintRect) || IsKeyDown(KEY_LEFT_CONTROL));
            nitroDown = inCar && (TouchHeld(sprintRect) || IsKeyDown(KEY_N));
            handbrakeDown = inCar && (TouchHeld(driftRect) || IsKeyDown(KEY_LEFT_ALT));
            jumpPressed = TouchPressed(jumpRect, &jumpWasDown) || IsKeyPressed(KEY_SPACE);
            actionPressed = TouchPressed(actionRect, &actionWasDown) || IsKeyPressed(KEY_E);
            bool firePressed = !inCar && (TouchPressed(fireRect, &fireWasDown) || IsKeyPressed(KEY_F));
            bool weaponPressed = !inCar && (TouchPressed(weaponRect, &weaponWasDown) || IsKeyPressed(KEY_Q));
            if (weaponPressed) {
                gWeaponIndex = (gWeaponIndex + 1)%3;
                snprintf(message, sizeof(message), "WEAPON: %s", (gWeaponIndex == 0) ? "PULSE" : ((gWeaponIndex == 1) ? "ARC RIFLE" : "SHOCKWAVE"));
                messageTimer = 1.5f;
            }
            if (!inCar && (firePressed || IsKeyPressed(KEY_R)) && gWeaponCooldown <= 0.0f) {
                int clip = (gWeaponIndex == 0) ? 45 : ((gWeaponIndex == 1) ? 120 : 12);
                if (IsKeyPressed(KEY_R) && gWeaponAmmo[gWeaponIndex] < clip) {
                    int take = gWeaponReserve[gWeaponIndex] < clip ? gWeaponReserve[gWeaponIndex] : clip;
                    gWeaponAmmo[gWeaponIndex] += take;
                    gWeaponReserve[gWeaponIndex] -= take;
                    snprintf(message, sizeof(message), "WEAPON RELOADED");
                    messageTimer = 1.2f;
                } else if (firePressed) {
                    FireTrainingWeapon(player, playerYaw, &cash, &points, message, sizeof(message));
                    messageTimer = 1.5f;
                    gWeaponCooldown = 0.30f;
                }
            }

            stick.x = ClampF(stick.x, -1.0f, 1.0f);
            stick.y = ClampF(stick.y, -1.0f, 1.0f);

            if (!inCar) {
                if (actionPressed && nearCar) {
                    inCar = true;
                    cameraMode = 0;
                    playerCar.position = player;
                    playerCar.position.y = 0.0f;
                    playerCar.yaw = playerYaw;
                    snprintf(message, sizeof(message), "VEHICLE READY - DRIVE SAFE");
                    messageTimer = 2.0f;
                } else if (actionPressed && nearMission && hasTarget) {
                    if ((missionStep%2) == 0) {
                        missionStep++;
                        snprintf(message, sizeof(message), "PACKAGE COLLECTED - DELIVER IT");
                    } else {
                        missionStep++;
                        delivered++;
                        cash += 650;
                        points += 250;
                        snprintf(message, sizeof(message), "DELIVERY COMPLETE  +$650  +250 PTS");
                    }
                    messageTimer = 2.6f;
                    if (missionStep >= MISSION_COUNT) {
                        snprintf(message, sizeof(message), "NOVA BAY THANKS YOU - ALL JOBS DONE");
                        messageTimer = 4.0f;
                    }
                }

                playerYaw = NormalizeAngle(playerYaw - stick.x*2.3f*dt);
                float speed = sprintDown ? 7.3f : 4.6f;
                float move = stick.y*speed;
                float dx = -sinf(playerYaw)*move*dt;
                float dz = -cosf(playerYaw)*move*dt;
                MoveWithCollision(&player, dx, dz, PLAYER_RADIUS);
                if (fabsf(stick.y) > 0.06f) walkPhase += dt*(sprintDown ? 13.0f : 9.0f);

                if (jumpPressed && grounded) {
                    jumpVelocity = 6.7f;
                    grounded = false;
                }
                jumpVelocity -= 18.0f*dt;
                player.y += jumpVelocity*dt;
                if (player.y <= 0.0f) {
                    player.y = 0.0f;
                    jumpVelocity = 0.0f;
                    grounded = true;
                }
            } else {
                if (jumpPressed) {
                    cameraMode = (cameraMode + 1)%3;
                    snprintf(message, sizeof(message), "CAMERA MODE %d / 3", cameraMode + 1);
                    messageTimer = 1.2f;
                }
                if (actionPressed) {
                    inCar = false;
                    playerCar.speed = 0.0f;
                    player = playerCar.position;
                    player.x += cosf(playerCar.yaw)*3.2f;
                    player.z -= sinf(playerCar.yaw)*3.2f;
                    player.y = 0.0f;
                    playerYaw = playerCar.yaw;
                    if (HitsBuilding(player.x, player.z, PLAYER_RADIUS)) {
                        player.x = playerCar.position.x - cosf(playerCar.yaw)*3.2f;
                        player.z = playerCar.position.z + sinf(playerCar.yaw)*3.2f;
                    }
                    snprintf(message, sizeof(message), "YOU ARE ON FOOT");
                    messageTimer = 1.5f;
                } else {
                    float throttle = stick.y;
                    if (gasDown) throttle = 1.0f;
                    if (brakeDown) throttle = -0.50f;

                    playerCar.speed += throttle*(nitroDown ? 23.0f : 14.5f)*dt;
                    if (!gasDown && !brakeDown && fabsf(stick.y) < 0.05f) {
                        playerCar.speed *= (1.0f - ClampF((handbrakeDown ? 0.48f : 0.86f)*dt, 0.0f, 0.9f));
                    }
                    if (brakeDown) {
                        if (playerCar.speed > 0.0f) playerCar.speed = fmaxf(0.0f, playerCar.speed - 26.0f*dt);
                        else playerCar.speed = fminf(0.0f, playerCar.speed + 26.0f*dt);
                    }
                    bool onSand = IsInSandArena(playerCar.position);
                    playerCar.speed = ClampF(playerCar.speed, -9.0f,
                                             nitroDown ? 54.0f : (onSand ? 34.0f : 39.0f));
                    float steerRate = handbrakeDown ? (onSand ? 2.85f : 2.45f) :
                                      (onSand ? 1.38f : 1.68f);
                    float turn = stick.x*steerRate*ClampF(fabsf(playerCar.speed)/4.0f, 0.18f, 1.15f)*dt;
                    playerCar.yaw = NormalizeAngle(playerCar.yaw - turn*(playerCar.speed < 0.0f ? -1.0f : 1.0f));
                    bool driftingNow = handbrakeDown && fabsf(playerCar.speed) > 7.0f && fabsf(stick.x) > 0.12f;
                    if (driftingNow) {
                        gDriftScore += fabsf(playerCar.speed)*fabsf(stick.x)*dt*(onSand ? 7.2f : 4.3f);
                        driftComboTimer = 1.25f;
                        gSmokeSpawnTimer += dt;
                        if (gSmokeSpawnTimer > 0.065f) { SpawnDriftEffects(&playerCar); gSmokeSpawnTimer = 0.0f; }
                    } else {
                        if (driftComboTimer > 0.0f) driftComboTimer -= dt;
                        else if (gDriftScore > 0.0f) {
                            if ((int)gDriftScore > gBestDrift) gBestDrift = (int)gDriftScore;
                            cash += (int)(gDriftScore/35.0f);
                            points += (int)(gDriftScore/8.0f);
                            gDriftScore = 0.0f;
                        }
                    }

                    float dx = -sinf(playerCar.yaw)*playerCar.speed*dt;
                    float dz = -cosf(playerCar.yaw)*playerCar.speed*dt;
                    float oldX = playerCar.position.x;
                    float oldZ = playerCar.position.z;
                    MoveWithCollision(&playerCar.position, dx, dz, CAR_RADIUS);
                    if (fabsf(playerCar.position.x - (oldX + dx)) > 0.01f ||
                        fabsf(playerCar.position.z - (oldZ + dz)) > 0.01f) {
                        playerCar.speed *= -0.18f;
                        snprintf(message, sizeof(message), "BE CAREFUL - COLLISION");
                        messageTimer = 1.0f;
                    }
                }
            }

            UpdateTraffic(dt);
            UpdatePedestrians(dt);
            UpdateTrainingTargets(dt);
            UpdateDriftEffects(dt);
            if (messageTimer > 0.0f) messageTimer -= dt;
        }
        UpdateAudio(inCar, playerCar.speed, gRadioEnabled);

        /* Camera follows smoothly behind the player or vehicle. */
        actor = inCar ? playerCar.position : player;
        float actorYaw = inCar ? playerCar.yaw : playerYaw;
        Vector3 desiredTarget = { actor.x, actor.y + (inCar ? 1.35f : 1.35f), actor.z };
        float cameraBack = inCar ? ((cameraMode == 1) ? 17.0f :
                                    ((cameraMode == 2) ? 6.2f : 10.8f)) : 10.6f;
        if (inCar && cameraMode == 0) cameraBack += ClampF(fabsf(playerCar.speed)*0.075f, 0.0f, 3.4f);
        Vector3 desiredCamera = {
            actor.x + sinf(actorYaw)*cameraBack,
            actor.y + (inCar ? ((cameraMode == 1) ? 8.2f : ((cameraMode == 2) ? 3.4f : 6.1f)) : 6.7f),
            actor.z + cosf(actorYaw)*cameraBack
        };
        if (state == GAME_TITLE) {
            desiredTarget = (Vector3){ 0.0f, 2.0f, 8.0f };
            desiredCamera = (Vector3){ 3.5f, 12.0f, 26.0f };
        }
        camera.target.x = LerpF(camera.target.x, desiredTarget.x, 1.0f - expf(-5.2f*dt));
        camera.target.y = LerpF(camera.target.y, desiredTarget.y, 1.0f - expf(-5.2f*dt));
        camera.target.z = LerpF(camera.target.z, desiredTarget.z, 1.0f - expf(-5.2f*dt));
        camera.position.x = LerpF(camera.position.x, desiredCamera.x, 1.0f - expf(-3.5f*dt));
        camera.position.y = LerpF(camera.position.y, desiredCamera.y, 1.0f - expf(-3.5f*dt));
        camera.position.z = LerpF(camera.position.z, desiredCamera.z, 1.0f - expf(-3.5f*dt));

        /* ---- Render 3D world ------------------------------------------- */
        BeginDrawing();
        Color sky = LerpColor((Color){9, 16, 34, 255}, C_SKY, daylight);
        ClearBackground(sky);
        BeginMode3D(camera);

        DrawRoadNetwork(actor);
        DrawCentralPlaza(worldTime);
        DrawWorldLandmarks(actor, worldTime);
        DrawTrainingRange(actor, worldTime);

        /* Buildings are distance-culled around the current actor. */
        for (int i = 0; i < gBuildingCount; i++) {
            DrawBuilding(&gBuildings[i], actor);
        }

        /* Procedural urban trees, park greenery and coastal tree belts. */
        for (int i = 0; i < gTreeCount; i++) {
            WorldTree *tree = &gTrees[i];
            if (Distance2D(actor.x, actor.z, tree->x, tree->z) < 178.0f)
                DrawTree(tree->x, tree->z, tree->scale, tree->species);
        }

        /* Street lamps along the centre boulevards. */
        for (int i = -3; i <= 3; i++) {
            float c = (float)i*48.0f;
            float lx1 = c + 11.0f;
            float lx2 = c - 11.0f;
            float lz = (float)((i%2 == 0) ? 48 : -48);
            if (Distance2D(actor.x, actor.z, lx1, lz) < DRAW_DISTANCE) DrawStreetLamp(lx1, lz);
            if (Distance2D(actor.x, actor.z, lx2, -lz) < DRAW_DISTANCE) DrawStreetLamp(lx2, -lz);
        }

        /* Traffic and walkers are updated continuously. */
        for (int i = 0; i < TRAFFIC_COUNT; i++) {
            if (Distance2D(actor.x, actor.z, gTraffic[i].position.x,
                           gTraffic[i].position.z) < DRAW_DISTANCE) {
                DrawVehicleModel(&gTraffic[i], false);
            }
        }
        DrawSandMeetCars(actor);
        for (int i = 0; i < WALKER_COUNT; i++) {
            Pedestrian *p = &gPedestrians[i];
            float drawRange = p->isStatic ? 132.0f : 112.0f;
            if (Distance2D(actor.x, actor.z, p->position.x, p->position.z) < drawRange) {
                DrawHuman(p->position, p->yaw, p->isStatic ? 0.0f : p->phase, p->shirt,
                          p->trousers, p->skin, false, false);
            }
        }

        /* Player vehicle and character. */
        DrawVehicleModel(&playerCar, true);
        if (!inCar) {
            DrawHuman(player, playerYaw, walkPhase,
                      (Color){ 32, 111, 190, 255 },
                      (Color){ 42, 53, 74, 255 },
                      (Color){ 196, 151, 117, 255 }, true,
                      false);
        }

        if (missionStep < MISSION_COUNT) {
            DrawMissionMarker(gMissionPoints[missionStep], worldTime,
                              (missionStep%2) == 0);
        }

        /* Sun/moon, tyre effects and local weather. */
        if (daylight > 0.22f) {
            DrawSphere((Vector3){actor.x - 100.0f, 72.0f, actor.z - 145.0f}, 7.0f,
                       (Color){255, 226, 166, 255});
        } else {
            DrawSphere((Vector3){actor.x + 110.0f, 78.0f, actor.z - 135.0f}, 5.3f,
                       (Color){218, 229, 244, 255});
        }
        DrawDriftEffects();
        DrawRainAround(actor, worldTime, raining);
        EndMode3D();

        /* ---- Render 2D interface -------------------------------------- */
        if (state == GAME_TITLE) {
            DrawRectangle(0, 0, screenW, screenH, Fade((Color){ 5, 12, 22, 255 }, 0.54f));
            DrawPanel((Rectangle){(float)screenW*0.5f - 225.0f, 62.0f, 450.0f, 130.0f},
                      (Color){ 10, 19, 31, 228 }, Fade(C_MINT, 0.70f));
            DrawArabicLabel(AR_CITY, (Rectangle){(float)screenW*0.5f - 170.0f, 79.0f, 340.0f, 42.0f}, C_MINT);
            DrawText("CITY LIFE 3D", screenW/2 - MeasureText("CITY LIFE 3D", 36)/2,
                     120, 36, C_WHITE);
            DrawArabicLabel(AR_JOB, (Rectangle){(float)screenW*0.5f - 130.0f, 165.0f, 260.0f, 28.0f}, C_GOLD);
            DrawArabicButton(playRect, AR_START, (Color){35, 137, 93, 255}, true);
            DrawArabicLabel(AR_READY, (Rectangle){(float)screenW*0.5f - 160.0f,
                            playRect.y + 76.0f, 320.0f, 26.0f}, Fade(C_WHITE, 0.82f));
            DrawArabicLabel(AR_MOVE, (Rectangle){(float)screenW*0.5f - 120.0f,
                            (float)screenH - 54.0f, 240.0f, 24.0f}, Fade(C_WHITE, 0.72f));
            DrawArabicLabel(AR_MUSIC, (Rectangle){(float)screenW*0.5f - 52.0f,
                            (float)screenH - 33.0f, 44.0f, 20.0f}, Fade(C_MINT, 0.85f));
            DrawText(TextFormat("%d", gAudioReady ? ((gMusicCount > 0) ? gMusicCount : 3) : 0),
                     screenW/2 + 8, screenH - 31, 14, Fade(C_MINT, 0.85f));
        } else {
            DrawGameHUD(screenW, screenH, cash, delivered, missionStep,
                        playerCar.speed, inCar, nearCar, nearMission,
                        actor, missionTarget, hasTarget,
                        stick, stickX, stickY, worldTime, gRadioEnabled, gRadioIndex,
                        playerCar.model, gDriftScore, gBestDrift, gNightAmount, raining,
                        gWeaponIndex, gWeaponAmmo[gWeaponIndex], gWeaponReserve[gWeaponIndex], points);

            DrawButton(pauseRect, "II", (Color){ 58, 73, 91, 255 }, false);
            DrawArabicButton(muteRect, gRadioEnabled ? AR_RADIO : AR_CLOSE, (Color){62, 90, 112, 255}, false);

            if (messageTimer > 0.0f) {
                Rectangle mr = {(float)screenW*0.5f - 190.0f,
                                (float)screenH - 60.0f, 380.0f, 36.0f};
                DrawPanel(mr, (Color){ 12, 22, 32, 218 }, Fade(C_MINT, 0.45f));
                int arabicMessage = GetArabicMessageLabel(message);
                if (arabicMessage >= 0) {
                    DrawArabicLabel(arabicMessage, (Rectangle){(float)screenW*0.5f - 150.0f,
                                    (float)screenH - 55.0f, 300.0f, 28.0f}, C_WHITE);
                } else {
                    DrawText(message, screenW/2 - MeasureText(message, 15)/2,
                             screenH - 50, 15, C_WHITE);
                }
            }

            if (state == GAME_PAUSED) {
                DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.62f));
                DrawPanel((Rectangle){(float)screenW*0.5f - 165.0f,
                                      (float)screenH*0.5f - 140.0f,
                                      330.0f, 290.0f},
                          (Color){ 12, 21, 34, 245 }, Fade(C_MINT, 0.75f));
                DrawArabicLabel(AR_PAUSED, (Rectangle){(float)screenW*0.5f - 100.0f,
                                (float)screenH*0.5f - 118.0f, 200.0f, 34.0f}, C_WHITE);
                DrawArabicButton(resumeRect, AR_RESUME, (Color){36, 139, 94, 255}, true);
                DrawArabicButton(restartRect, AR_RESTART, (Color){55, 99, 153, 255}, false);
                DrawArabicLabel(AR_READY, (Rectangle){(float)screenW*0.5f - 150.0f,
                                (float)screenH*0.5f + 103.0f, 300.0f, 24.0f}, Fade(C_WHITE, 0.72f));
            }
        }

        EndDrawing();
    }

    if (gAudioDeviceReady) {
        StopRadioPlayback();
        for (int track = 0; track < gMusicCount; track++) {
            if (IsMusicValid(gMusicTracks[track])) UnloadMusicStream(gMusicTracks[track]);
        }
        for (int track = 0; track < 3; track++) {
            if (IsSoundValid(gRadioSounds[track])) {
                StopSound(gRadioSounds[track]);
                UnloadSound(gRadioSounds[track]);
            }
        }
        if (IsSoundValid(gEngineSound)) {
            StopSound(gEngineSound);
            UnloadSound(gEngineSound);
        }
        CloseAudioDevice();
    }
    if (gArabicAtlas.id != 0) UnloadTexture(gArabicAtlas);
    CloseWindow();
    return 0;
}
