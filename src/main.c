#ifdef __APPLE__
#include <GLUT/glut.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>
#else
#include <GL/glut.h>
#endif

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VIEW_W 320
#define VIEW_H 180
#define SCALE 3
#define TILE 16
#define MAP_W 96
#define MAP_H 12
#define FIXED_DT (1.0f / 60.0f)
#define WALK_FRAME_SECONDS 0.145f
#define WALK_FRAME_COUNT 4
#define WALK_ANIM_BASE_RATE 0.70f
#define WALK_ANIM_SPEED_SCALE 210.0f
#define WALK_ANIM_MAX_RATE 1.16f

typedef struct {
    float x, y;
    float vx, vy;
    float anim_time;
    float air_time;
    float attack_age;
    float landing_timer;
    int facing;
    bool grounded;
    float coyote;
    float jump_buffer;
    float attack_timer;
    float backdash_timer;
    int relics;
} Player;

typedef struct {
    float x, y;
    bool taken;
} Relic;

typedef struct {
    unsigned char r, g, b;
} Color;

typedef struct {
    float x, y;
    float vx, vy;
    float life;
    float max_life;
    float size;
    Color color;
} Particle;

typedef struct {
    GLuint id;
    int w, h;
    bool ready;
} Texture;

typedef struct {
    int x, y, w, h;
} SrcRect;

typedef enum {
    TUNE_IDLE,
    TUNE_WALK,
    TUNE_JUMP,
    TUNE_SLASH,
    TUNE_DASH,
    TUNE_COUNT
} TuneAnim;

static const char *map_rows[MAP_H] = {
    "................................................................................................",
    "................................................................................................",
    "................................................................................................",
    "................................................................................................",
    "................................................................................................",
    "................................................................................................",
    "................................................................................................",
    "................................................................................................",
    "................................................................................................",
    "................................................................................................",
    "................................................................................................",
    "################################################################################################",
};

static Player player = { 42.0f, 148.0f, 0, 0, 0, 0, 0, 0, 1, true, 0, 0, 0, 0, 0 };
static Relic relics[] = {
    { 300, 112, false },
    { 650, 80, false },
    { 1030, 128, false },
    { 1390, 64, false },
};
static const int relic_count = (int)(sizeof(relics) / sizeof(relics[0]));
static Particle particles[48];

static const SrcRect FG_FLOOR = { 33, 63, 514, 130 };
static const SrcRect FG_PLATFORM_L = { 601, 59, 223, 160 };
static const SrcRect FG_PLATFORM_M = { 848, 60, 342, 154 };
static const SrcRect FG_PLATFORM_R = { 1217, 61, 292, 161 };
static const SrcRect FG_COLUMN = { 46, 249, 151, 709 };
static const SrcRect FG_ARCH = { 238, 269, 265, 355 };
static const SrcRect FG_CANDELABRA = { 576, 318, 118, 276 };
static const SrcRect FG_CANDLES = { 731, 388, 128, 210 };
static const SrcRect FG_LANTERN = { 924, 288, 105, 305 };
static const SrcRect FG_BANNER = { 1073, 287, 218, 551 };
static const SrcRect FG_VINES = { 1323, 340, 153, 552 };
static const SrcRect FG_ALTAR = { 360, 682, 221, 275 };
static const SrcRect FG_WINDOW = { 771, 642, 227, 339 };

static const SrcRect MG_BELL_TOWER = { 70, 45, 210, 505 };
static const SrcRect MG_TALL_RUIN = { 320, 90, 360, 450 };
static const SrcRect MG_CLOISTER = { 720, 135, 410, 405 };
static const SrcRect MG_CHAPEL = { 1160, 65, 310, 475 };
static const SrcRect MG_BRIDGE = { 100, 700, 425, 245 };
static const SrcRect MG_CROSS = { 660, 585, 210, 375 };
static const SrcRect MG_WALL = { 940, 665, 345, 285 };

static const SrcRect HERO_IDLE_FRAMES[] = {
    { 72, 30, 220, 260 },
    { 402, 30, 220, 260 },
    { 732, 30, 220, 260 },
    { 1060, 30, 220, 260 },
};
static const SrcRect HERO_IDLE_BREATH_FRAMES[] = {
    { 0, 110, 362, 470 },
    { 362, 110, 362, 470 },
    { 724, 110, 362, 470 },
    { 1086, 110, 362, 470 },
    { 1448, 110, 362, 470 },
    { 1810, 110, 362, 470 },
};
static const SrcRect HERO_WALK_FRAMES[] = {
    { 44, 305, 215, 260 },
    { 272, 305, 215, 260 },
    { 500, 305, 215, 260 },
    { 728, 305, 220, 260 },
    { 942, 305, 225, 260 },
    { 1165, 305, 215, 260 },
};
static const SrcRect HERO_WALK_SMOOTH_FRAMES[] = {
    { 45, 110, 420, 530 },
    { 535, 110, 430, 530 },
    { 1050, 110, 498, 530 },
    { 1536, 110, 420, 530 },
};
static const SrcRect HERO_JUMP_FRAMES[] = {
    { 50, 585, 250, 225 },
    { 395, 560, 260, 250 },
    { 632, 555, 270, 255 },
    { 1030, 585, 275, 225 },
};
static const SrcRect HERO_SLASH_FRAMES[] = {
    { 45, 842, 245, 235 },
    { 350, 828, 320, 250 },
    { 660, 825, 420, 250 },
    { 1065, 842, 270, 235 },
};

static const float DEFAULT_IDLE_DRAW_W[] = { 48.2f, 48.2f, 48.2f, 48.2f, 41.9f, 48.2f };
static const float DEFAULT_IDLE_DRAW_H[] = { 62.0f, 62.0f, 62.0f, 62.0f, 62.0f, 62.0f };
static const float DEFAULT_IDLE_OX[] = { 24.0f, 21.3f, 20.1f, 18.6f, 18.8f, 17.4f };
static const float DEFAULT_IDLE_OY[] = { 62.0f, 62.0f, 62.0f, 62.0f, 62.0f, 62.0f };
static const int DEFAULT_IDLE_SRC_X[] = { 0, 0, 0, 0, 0, 0 };
static const int DEFAULT_IDLE_SRC_Y[] = { 0, 0, 0, 0, 0, 0 };
static const int DEFAULT_IDLE_SRC_W[] = { 362, 362, 362, 362, 315, 362 };
static const int DEFAULT_IDLE_SRC_H[] = { 470, 470, 470, 470, 470, 470 };

static const int DEFAULT_WALK_SRC_X[] = { 0, 0, 0, 0 };
static const int DEFAULT_WALK_SRC_Y[] = { 0, 0, 0, 0 };
static const int DEFAULT_WALK_SRC_W[] = { 420, 430, 498, 420 };
static const int DEFAULT_WALK_SRC_H[] = { 530, 530, 530, 530 };
static const float DEFAULT_WALK_DRAW_W[] = { 50.7f, 51.9f, 60.1f, 50.7f };
static const float DEFAULT_WALK_DRAW_H[] = { 64.0f, 64.0f, 64.0f, 64.0f };
static const float DEFAULT_WALK_OX[] = { 23.9f, 24.4f, 30.3f, 22.4f };
static const float DEFAULT_WALK_OY[] = { 62.2f, 62.2f, 62.2f, 62.2f };

static const float DEFAULT_JUMP_DRAW_W[] = { 62.6f, 70.8f, 70.8f, 70.8f };
static const float DEFAULT_JUMP_DRAW_H[] = { 59.2f, 67.3f, 67.3f, 63.8f };
static const float DEFAULT_JUMP_OX[] = { 30.2f, 25.9f, 36.2f, 18.2f };
static const float DEFAULT_JUMP_OY[] = { 56.8f, 63.8f, 63.8f, 61.5f };
static const int DEFAULT_JUMP_SRC_X[] = { 0, 0, 0, 0 };
static const int DEFAULT_JUMP_SRC_Y[] = { 0, 0, 0, 0 };
static const int DEFAULT_JUMP_SRC_W[] = { 250, 260, 270, 275 };
static const int DEFAULT_JUMP_SRC_H[] = { 225, 250, 255, 225 };

static const float DEFAULT_SLASH_DRAW_W[] = { 58.0f, 74.0f, 80.2f, 58.0f };
static const float DEFAULT_SLASH_DRAW_H[] = { 57.0f, 57.0f, 60.0f, 57.0f };
static const float DEFAULT_SLASH_OX[] = { 23.0f, 23.0f, 23.0f, 23.0f };
static const float DEFAULT_SLASH_OY[] = { 55.0f, 55.0f, 55.0f, 55.0f };
static const int DEFAULT_SLASH_SRC_X[] = { 0, 0, 0, 0 };
static const int DEFAULT_SLASH_SRC_Y[] = { 0, 0, 0, 0 };
static const int DEFAULT_SLASH_SRC_W[] = { 245, 320, 370, 270 };
static const int DEFAULT_SLASH_SRC_H[] = { 235, 250, 250, 235 };

static const float DEFAULT_DASH_DRAW_W[] = { 70.8f };
static const float DEFAULT_DASH_DRAW_H[] = { 67.3f };
static const float DEFAULT_DASH_OX[] = { 25.9f };
static const float DEFAULT_DASH_OY[] = { 63.8f };
static const int DEFAULT_DASH_SRC_X[] = { 0 };
static const int DEFAULT_DASH_SRC_Y[] = { 0 };
static const int DEFAULT_DASH_SRC_W[] = { 260 };
static const int DEFAULT_DASH_SRC_H[] = { 250 };

static float idle_draw_w[] = { 48.2f, 48.2f, 48.2f, 48.2f, 41.9f, 48.2f };
static float idle_draw_h[] = { 62.0f, 62.0f, 62.0f, 62.0f, 62.0f, 62.0f };
static float idle_ox[] = { 24.0f, 21.3f, 20.1f, 18.6f, 18.8f, 17.4f };
static float idle_oy[] = { 62.0f, 62.0f, 62.0f, 62.0f, 62.0f, 62.0f };
static int idle_src_x[] = { 0, 0, 0, 0, 0, 0 };
static int idle_src_y[] = { 0, 0, 0, 0, 0, 0 };
static int idle_src_w[] = { 362, 362, 362, 362, 315, 362 };
static int idle_src_h[] = { 470, 470, 470, 470, 470, 470 };

static int walk_src_x[] = { 0, 0, 0, 0 };
static int walk_src_y[] = { 0, 0, 0, 0 };
static int walk_src_w[] = { 420, 430, 498, 420 };
static int walk_src_h[] = { 530, 530, 530, 530 };
static float walk_draw_w[] = { 50.7f, 51.9f, 60.1f, 50.7f };
static float walk_draw_h[] = { 64.0f, 64.0f, 64.0f, 64.0f };
static float walk_ox[] = { 23.9f, 24.4f, 30.3f, 22.4f };
static float walk_oy[] = { 62.2f, 62.2f, 62.2f, 62.2f };

static float jump_draw_w[] = { 62.6f, 70.8f, 70.8f, 70.8f };
static float jump_draw_h[] = { 59.2f, 67.3f, 67.3f, 63.8f };
static float jump_ox[] = { 30.2f, 25.9f, 36.2f, 18.2f };
static float jump_oy[] = { 56.8f, 63.8f, 63.8f, 61.5f };
static int jump_src_x[] = { 0, 0, 0, 0 };
static int jump_src_y[] = { 0, 0, 0, 0 };
static int jump_src_w[] = { 250, 260, 270, 275 };
static int jump_src_h[] = { 225, 250, 255, 225 };

static float slash_draw_w[] = { 58.0f, 74.0f, 80.2f, 58.0f };
static float slash_draw_h[] = { 57.0f, 57.0f, 60.0f, 57.0f };
static float slash_ox[] = { 23.0f, 23.0f, 23.0f, 23.0f };
static float slash_oy[] = { 55.0f, 55.0f, 55.0f, 55.0f };
static int slash_src_x[] = { 0, 0, 0, 0 };
static int slash_src_y[] = { 0, 0, 0, 0 };
static int slash_src_w[] = { 245, 320, 370, 270 };
static int slash_src_h[] = { 235, 250, 250, 235 };

static float dash_draw_w[] = { 70.8f };
static float dash_draw_h[] = { 67.3f };
static float dash_ox[] = { 25.9f };
static float dash_oy[] = { 63.8f };
static int dash_src_x[] = { 0 };
static int dash_src_y[] = { 0 };
static int dash_src_w[] = { 260 };
static int dash_src_h[] = { 250 };

static bool keys[256];
static bool special_keys[256];
static bool prev_jump_down;
static bool prev_attack_down;
static bool prev_dash_down;
static float camera_x;
static float target_camera_x;
static float camera_lookahead;
static int last_time_ms;
static int sim_frame;
static int capture_frame;
static int forced_walk_frame = -1;
static int forced_idle_frame = -1;
static int forced_jump_frame = -1;
static int forced_facing;
static const char *capture_script;
static const char *telemetry_path;
static FILE *telemetry_file;
static float accumulator;
static Texture background_texture;
static Texture foreground_texture;
static Texture player_texture;
static Texture idle_texture;
static Texture walk_texture;
static Texture midground_texture;
static bool captured_frame;
static bool player_interacted;
static bool isolate_player;
static bool tune_mode;
static bool tune_trim_mode;
static TuneAnim tune_anim = TUNE_IDLE;
static int tune_frame;
static const char *tune_file_path = "assets/pilgrim_tuning.txt";
static const char *tune_export_path = "/tmp/pilgrim_tuning_tables.c";

static void color(Color c)
{
    glColor3ub(c.r, c.g, c.b);
}

static void rect(float x, float y, float w, float h, Color c)
{
    color(c);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

static void rect_alpha(float x, float y, float w, float h, Color c, unsigned char a)
{
    glColor4ub(c.r, c.g, c.b, a);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

static void tri(float x1, float y1, float x2, float y2, float x3, float y3, Color c)
{
    color(c);
    glBegin(GL_TRIANGLES);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glVertex2f(x3, y3);
    glEnd();
}

static void line(float x1, float y1, float x2, float y2, Color c)
{
    color(c);
    glBegin(GL_LINES);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glEnd();
}

static void texture_src(Texture *texture, SrcRect src, float x, float y, float w, float h, bool flip_x)
{
    if (!texture->ready) {
        return;
    }

    float u0 = (float)src.x / texture->w;
    float u1 = (float)(src.x + src.w) / texture->w;
    float v_top = 1.0f - (float)src.y / texture->h;
    float v_bottom = 1.0f - (float)(src.y + src.h) / texture->h;

    if (flip_x) {
        float tmp = u0;
        u0 = u1;
        u1 = tmp;
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture->id);
    glColor4ub(255, 255, 255, 255);
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v_top);
    glVertex2f(x, y);
    glTexCoord2f(u1, v_top);
    glVertex2f(x + w, y);
    glTexCoord2f(u1, v_bottom);
    glVertex2f(x + w, y + h);
    glTexCoord2f(u0, v_bottom);
    glVertex2f(x, y + h);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

static void texture_full_crop(Texture *texture, float x, float y, float w, float h, float u0, float u1)
{
    if (!texture->ready) {
        return;
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture->id);
    glColor4ub(255, 255, 255, 255);
    glBegin(GL_QUADS);
    glTexCoord2f(u0, 1.0f);
    glVertex2f(x, y);
    glTexCoord2f(u1, 1.0f);
    glVertex2f(x + w, y);
    glTexCoord2f(u1, 0.0f);
    glVertex2f(x + w, y + h);
    glTexCoord2f(u0, 0.0f);
    glVertex2f(x, y + h);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

static float draw_ox_for_facing(float draw_w, float ox, int facing)
{
    return facing < 0 ? draw_w - ox : ox;
}

#ifdef __APPLE__
static bool load_png_texture_keyed(const char *path, Texture *texture, bool green_key)
{
    CFURLRef url = CFURLCreateFromFileSystemRepresentation(NULL, (const UInt8 *)path, strlen(path), false);
    if (!url) {
        return false;
    }

    CGImageSourceRef source = CGImageSourceCreateWithURL(url, NULL);
    CFRelease(url);
    if (!source) {
        return false;
    }

    CGImageRef image = CGImageSourceCreateImageAtIndex(source, 0, NULL);
    CFRelease(source);
    if (!image) {
        return false;
    }

    size_t width = CGImageGetWidth(image);
    size_t height = CGImageGetHeight(image);
    size_t stride = width * 4;
    unsigned char *pixels = calloc(height, stride);
    if (!pixels) {
        CGImageRelease(image);
        return false;
    }

    CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(pixels, width, height, 8, stride, space,
        kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
    CGColorSpaceRelease(space);
    if (!ctx) {
        free(pixels);
        CGImageRelease(image);
        return false;
    }

    CGContextTranslateCTM(ctx, 0, height);
    CGContextScaleCTM(ctx, 1, -1);
    CGContextDrawImage(ctx, CGRectMake(0, 0, width, height), image);
    CGContextRelease(ctx);
    CGImageRelease(image);

    if (green_key) {
        for (size_t i = 0; i < width * height; ++i) {
            unsigned char *p = pixels + i * 4;
            int dr = abs((int)p[0] - 0);
            int dg = abs((int)p[1] - 255);
            int db = abs((int)p[2] - 0);
            if (dr < 48 && dg < 80 && db < 48 && p[1] > 150) {
                p[0] = 0;
                p[1] = 0;
                p[2] = 0;
                p[3] = 0;
            } else if (p[1] > p[0] + 60 && p[1] > p[2] + 60) {
                p[1] = (unsigned char)((p[0] + p[2]) / 2);
            }
        }
    }

    glGenTextures(1, &texture->id);
    glBindTexture(GL_TEXTURE_2D, texture->id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    free(pixels);
    texture->w = (int)width;
    texture->h = (int)height;
    texture->ready = true;
    return true;
}
#else
static bool load_png_texture_keyed(const char *path, Texture *texture, bool green_key)
{
    (void)path;
    (void)texture;
    (void)green_key;
    return false;
}
#endif

static bool solid_at(int tx, int ty)
{
    if (tx < 0 || ty < 0 || tx >= MAP_W || ty >= MAP_H) {
        return false;
    }
    return map_rows[ty][tx] == '#';
}

static bool hitbox_solid(float x, float y, float w, float h)
{
    int x0 = (int)floorf(x / TILE);
    int y0 = (int)floorf(y / TILE);
    int x1 = (int)floorf((x + w - 0.01f) / TILE);
    int y1 = (int)floorf((y + h - 0.01f) / TILE);

    for (int ty = y0; ty <= y1; ++ty) {
        for (int tx = x0; tx <= x1; ++tx) {
            if (solid_at(tx, ty)) {
                return true;
            }
        }
    }
    return false;
}

static float approach(float v, float target, float delta)
{
    if (v < target) {
        v += delta;
        return v > target ? target : v;
    }
    if (v > target) {
        v -= delta;
        return v < target ? target : v;
    }
    return v;
}

static void spawn_particle(float x, float y, float vx, float vy, float life, float size, Color color)
{
    for (int i = 0; i < (int)(sizeof(particles) / sizeof(particles[0])); ++i) {
        if (particles[i].life <= 0.0f) {
            particles[i].x = x;
            particles[i].y = y;
            particles[i].vx = vx;
            particles[i].vy = vy;
            particles[i].life = life;
            particles[i].max_life = life;
            particles[i].size = size;
            particles[i].color = color;
            return;
        }
    }
}

static void spawn_jump_dust(float x, float y)
{
    for (int i = 0; i < 6; ++i) {
        float dir = (float)i - 2.5f;
        spawn_particle(x + dir * 2.2f, y, dir * 8.0f, -12.0f - fabsf(dir) * 1.5f, 0.24f, 2.4f, (Color){ 171, 151, 111 });
    }
}

static void spawn_land_dust(float x, float y, float speed)
{
    int count = speed > 210.0f ? 9 : 5;
    for (int i = 0; i < count; ++i) {
        float dir = (float)i - (count - 1) * 0.5f;
        spawn_particle(x + dir * 1.7f, y, dir * 10.0f, -16.0f - fabsf(dir), 0.30f, 2.8f, (Color){ 129, 118, 94 });
    }
}

static void spawn_slash_sparks(float x, float y, int facing)
{
    for (int i = 0; i < 5; ++i) {
        spawn_particle(x + facing * (12.0f + i * 3.0f), y - 8.0f + i, facing * (32.0f + i * 5.0f), -22.0f + i * 6.0f, 0.18f, 1.8f, (Color){ 238, 209, 127 });
    }
}

static void spawn_backdash_dust(float x, float y, int facing)
{
    for (int i = 0; i < 7; ++i) {
        float spread = (float)i - 3.0f;
        spawn_particle(x - facing * (3.0f + i), y + spread * 0.4f, -facing * (22.0f + i * 4.0f), -10.0f - fabsf(spread), 0.22f, 2.4f, (Color){ 117, 111, 101 });
    }
}

static void update_particles(float dt)
{
    for (int i = 0; i < (int)(sizeof(particles) / sizeof(particles[0])); ++i) {
        if (particles[i].life <= 0.0f) {
            continue;
        }
        particles[i].life -= dt;
        particles[i].x += particles[i].vx * dt;
        particles[i].y += particles[i].vy * dt;
        particles[i].vy += 80.0f * dt;
    }
}

static void draw_particles(void)
{
    for (int i = 0; i < (int)(sizeof(particles) / sizeof(particles[0])); ++i) {
        if (particles[i].life <= 0.0f) {
            continue;
        }
        float t = particles[i].life / particles[i].max_life;
        unsigned char a = (unsigned char)(190.0f * t);
        rect_alpha(particles[i].x, particles[i].y, particles[i].size * (1.0f + (1.0f - t)), particles[i].size, particles[i].color, a);
    }
}

static void write_telemetry(void)
{
    if (!telemetry_file) {
        return;
    }
    fprintf(telemetry_file, "%d %.3f %.3f %.3f %.3f %d %.3f %.3f\n",
        sim_frame, player.x, player.y, player.vx, player.vy, player.grounded ? 1 : 0, player.air_time, camera_x);
    fflush(telemetry_file);
}

static void close_telemetry(void)
{
    if (telemetry_file) {
        fclose(telemetry_file);
        telemetry_file = NULL;
    }
}

static bool scripted_key(unsigned char key)
{
    if (!capture_script) {
        return false;
    }
    if (strcmp(capture_script, "walk") == 0) {
        return key == 'd' && sim_frame < 140;
    }
    if (strcmp(capture_script, "jump") == 0) {
        return key == 'd' || (key == ' ' && sim_frame >= 10 && sim_frame < 23);
    }
    if (strcmp(capture_script, "attack") == 0) {
        return key == 'j' && sim_frame == 18;
    }
    if (strcmp(capture_script, "dash") == 0) {
        return (key == 'd' && sim_frame < 18) || (key == 'k' && sim_frame == 24);
    }
    return false;
}

static bool jump_down(void)
{
    return keys[' '] || keys['w'] || keys['W'] || special_keys[GLUT_KEY_UP] || scripted_key(' ');
}

static bool attack_down(void)
{
    return keys['j'] || keys['J'] || scripted_key('j');
}

static bool dash_down(void)
{
    return keys['k'] || keys['K'] || scripted_key('k');
}

static int input_axis(void)
{
    int left = keys['a'] || keys['A'] || special_keys[GLUT_KEY_LEFT] || scripted_key('a');
    int right = keys['d'] || keys['D'] || special_keys[GLUT_KEY_RIGHT] || scripted_key('d');
    return right - left;
}

static void move_x(float dx)
{
    player.x += dx;
    if (!hitbox_solid(player.x, player.y, 10, 28)) {
        return;
    }

    if (dx > 0) {
        player.x = floorf((player.x + 10) / TILE) * TILE - 10.01f;
    } else if (dx < 0) {
        player.x = floorf(player.x / TILE + 1) * TILE + 0.01f;
    }
    player.vx = 0;
}

static void move_y(float dy)
{
    player.y += dy;
    player.grounded = false;

    if (!hitbox_solid(player.x, player.y, 10, 28)) {
        return;
    }

    if (dy > 0) {
        player.y = floorf((player.y + 28) / TILE) * TILE - 28.01f;
        player.grounded = true;
        player.coyote = 0.08f;
    } else if (dy < 0) {
        player.y = floorf(player.y / TILE + 1) * TILE + 0.01f;
    }
    player.vy = 0;
}

static void reset_player(void)
{
    player.x = 42;
    player.y = 148;
    player.vx = 0;
    player.vy = 0;
    player.anim_time = 0;
    player.air_time = 0;
    player.attack_age = 0;
    player.landing_timer = 0;
    player.facing = 1;
    player.grounded = true;
    player.coyote = 0;
    player.jump_buffer = 0;
    player.attack_timer = 0;
    player.backdash_timer = 0;
    player.relics = 0;
    memset(particles, 0, sizeof(particles));
    target_camera_x = 0;
    camera_x = 0;
    camera_lookahead = 0;
    player_interacted = false;
    for (int i = 0; i < relic_count; ++i) {
        relics[i].taken = false;
    }
}

static void update(float dt)
{
    bool jd = jump_down();
    bool ad = attack_down();
    bool dd = dash_down();
    bool was_grounded = player.grounded;
    float fall_speed = player.vy;

    if (keys[27]) {
        exit(0);
    }
    if (!capture_script && (jd || ad || dd || input_axis() != 0)) {
        player_interacted = true;
    }
    if (keys['r'] || keys['R']) {
        reset_player();
    }

    if (jd && !prev_jump_down) {
        player.jump_buffer = 0.11f;
    }
    if (ad && !prev_attack_down && player.attack_timer <= 0.0f) {
        player.attack_timer = 0.34f;
        player.attack_age = 0.0f;
        spawn_slash_sparks(player.x + 5.0f, player.y + 21.0f, player.facing);
    }
    if (dd && !prev_dash_down && player.backdash_timer <= 0.0f) {
        player.backdash_timer = 0.22f;
        player.vx = -player.facing * 132.0f;
        player.vy = player.grounded ? -18.0f : player.vy;
        spawn_backdash_dust(player.x + 5.0f, player.y + 28.0f, player.facing);
    }

    prev_jump_down = jd;
    prev_attack_down = ad;
    prev_dash_down = dd;

    player.jump_buffer = fmaxf(0, player.jump_buffer - dt);
    player.coyote = fmaxf(0, player.coyote - dt);
    if (player.attack_timer > 0.0f) {
        player.attack_age += dt;
    }
    player.attack_timer = fmaxf(0, player.attack_timer - dt);
    player.backdash_timer = fmaxf(0, player.backdash_timer - dt);
    player.landing_timer = fmaxf(0, player.landing_timer - dt);

    int axis = input_axis();
    if (forced_facing) {
        player.facing = forced_facing;
    } else if (axis != 0 && player.backdash_timer <= 0.0f) {
        player.facing = axis;
    }

    float accel = player.grounded ? 1320.0f : 520.0f;
    float friction = player.grounded ? 1500.0f : 120.0f;
    float max_speed = player.grounded ? 96.0f : 88.0f;
    if (player.attack_timer > 0.0f && player.grounded) {
        max_speed = 46.0f;
        accel = 420.0f;
        friction = 1900.0f;
    }

    if (player.backdash_timer <= 0.0f) {
        if (axis != 0) {
            player.vx = approach(player.vx, axis * max_speed, accel * dt);
        } else {
            player.vx = approach(player.vx, 0, friction * dt);
        }
    } else {
        player.vx = approach(player.vx, -player.facing * 76.0f, 160.0f * dt);
    }

    if (player.jump_buffer > 0.0f && (player.grounded || player.coyote > 0.0f)) {
        spawn_jump_dust(player.x + 5.0f, player.y + 28.0f);
        player.vy = -202.0f;
        player.grounded = false;
        player.air_time = 0.0f;
        player.coyote = 0;
        player.jump_buffer = 0;
    }

    float gravity = player.vy > 0.0f ? 760.0f : 560.0f;
    if (fabsf(player.vy) < 34.0f && !player.grounded) {
        gravity *= 0.82f;
    }
    if (!jd && player.vy < -58.0f) {
        player.vy = approach(player.vy, -58.0f, 860.0f * dt);
    }
    player.vy += gravity * dt;
    if (player.vy > 286.0f) {
        player.vy = 286.0f;
    }

    move_x(player.vx * dt);
    move_y(player.vy * dt);
    if (!was_grounded && player.grounded && fall_speed > 80.0f) {
        spawn_land_dust(player.x + 5.0f, player.y + 28.0f, fall_speed);
        player.landing_timer = fall_speed > 190.0f ? 0.11f : 0.07f;
    }

    if (player.grounded) {
        player.air_time = 0.0f;
    } else {
        player.air_time += dt;
    }
    if (fabsf(player.vx) > 4.0f && player.grounded && player.backdash_timer <= 0.0f) {
        float walk_rate = WALK_ANIM_BASE_RATE + fabsf(player.vx) / WALK_ANIM_SPEED_SCALE;
        if (walk_rate > WALK_ANIM_MAX_RATE) {
            walk_rate = WALK_ANIM_MAX_RATE;
        }
        player.anim_time += dt * walk_rate;
    } else {
        player.anim_time += dt;
    }
    sim_frame++;
    update_particles(dt);

    for (int i = 0; i < relic_count; ++i) {
        if (!relics[i].taken && fabsf((player.x + 5) - relics[i].x) < 12 && fabsf((player.y + 12) - relics[i].y) < 16) {
            relics[i].taken = true;
            player.relics++;
        }
    }

    float max_camera = MAP_W * TILE - VIEW_W;
    float desired_lookahead = 0.0f;
    if (fabsf(player.vx) > 24.0f) {
        desired_lookahead = player.facing * 26.0f;
    } else if (input_axis() != 0) {
        desired_lookahead = player.facing * 14.0f;
    }
    if (player.backdash_timer > 0.0f) {
        desired_lookahead *= 0.35f;
    }
    camera_lookahead += (desired_lookahead - camera_lookahead) * fminf(1.0f, 5.5f * dt);

    target_camera_x = player.x - VIEW_W * 0.45f + camera_lookahead;
    if (target_camera_x < 0) {
        target_camera_x = 0;
    }
    if (target_camera_x > max_camera) {
        target_camera_x = max_camera;
    }
    camera_x += (target_camera_x - camera_x) * fminf(1.0f, 9.5f * dt);
    if (fabsf(camera_x - target_camera_x) < 0.05f) {
        camera_x = target_camera_x;
    }
    write_telemetry();
}

static void draw_text(float x, float y, const char *text, Color c)
{
    color(c);
    glRasterPos2f(x, y);
    for (const char *p = text; *p; ++p) {
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *p);
    }
}

static int tune_frame_count(TuneAnim anim)
{
    switch (anim) {
        case TUNE_IDLE: return 6;
        case TUNE_WALK: return WALK_FRAME_COUNT;
        case TUNE_JUMP: return 4;
        case TUNE_SLASH: return 4;
        case TUNE_DASH: return 1;
        default: return 1;
    }
}

static const char *tune_anim_name(TuneAnim anim)
{
    switch (anim) {
        case TUNE_IDLE: return "idle";
        case TUNE_WALK: return "walk";
        case TUNE_JUMP: return "jump";
        case TUNE_SLASH: return "slash";
        case TUNE_DASH: return "dash";
        default: return "unknown";
    }
}

static TuneAnim tune_anim_from_name(const char *name)
{
    if (!name) {
        return tune_anim;
    }
    if (strcmp(name, "idle") == 0) {
        return TUNE_IDLE;
    }
    if (strcmp(name, "walk") == 0) {
        return TUNE_WALK;
    }
    if (strcmp(name, "jump") == 0) {
        return TUNE_JUMP;
    }
    if (strcmp(name, "slash") == 0 || strcmp(name, "attack") == 0) {
        return TUNE_SLASH;
    }
    if (strcmp(name, "dash") == 0) {
        return TUNE_DASH;
    }
    return tune_anim;
}

static void tune_active_pose(float **draw_w, float **draw_h, float **ox, float **oy)
{
    int f = tune_frame % tune_frame_count(tune_anim);
    switch (tune_anim) {
        case TUNE_IDLE:
            *draw_w = &idle_draw_w[f];
            *draw_h = &idle_draw_h[f];
            *ox = &idle_ox[f];
            *oy = &idle_oy[f];
            break;
        case TUNE_WALK:
            *draw_w = &walk_draw_w[f];
            *draw_h = &walk_draw_h[f];
            *ox = &walk_ox[f];
            *oy = &walk_oy[f];
            break;
        case TUNE_JUMP:
            *draw_w = &jump_draw_w[f];
            *draw_h = &jump_draw_h[f];
            *ox = &jump_ox[f];
            *oy = &jump_oy[f];
            break;
        case TUNE_SLASH:
            *draw_w = &slash_draw_w[f];
            *draw_h = &slash_draw_h[f];
            *ox = &slash_ox[f];
            *oy = &slash_oy[f];
            break;
        case TUNE_DASH:
            *draw_w = &dash_draw_w[0];
            *draw_h = &dash_draw_h[0];
            *ox = &dash_ox[0];
            *oy = &dash_oy[0];
            break;
        default:
            *draw_w = &idle_draw_w[0];
            *draw_h = &idle_draw_h[0];
            *ox = &idle_ox[0];
            *oy = &idle_oy[0];
            break;
    }
}

static SrcRect tune_base_src_rect(TuneAnim anim, int frame)
{
    switch (anim) {
        case TUNE_IDLE:
            return HERO_IDLE_BREATH_FRAMES[frame % 6];
        case TUNE_WALK:
            return HERO_WALK_SMOOTH_FRAMES[frame % WALK_FRAME_COUNT];
        case TUNE_JUMP:
            return HERO_JUMP_FRAMES[frame % 4];
        case TUNE_SLASH:
            return HERO_SLASH_FRAMES[frame % 4];
        case TUNE_DASH:
            return HERO_JUMP_FRAMES[1];
        default:
            return HERO_IDLE_FRAMES[1];
    }
}

static void tune_active_source(int **src_x, int **src_y, int **src_w, int **src_h, int *base_w, int *base_h)
{
    int f = tune_frame % tune_frame_count(tune_anim);
    SrcRect base = tune_base_src_rect(tune_anim, f);
    *base_w = base.w;
    *base_h = base.h;
    if (tune_anim == TUNE_IDLE) {
        *src_x = &idle_src_x[f];
        *src_y = &idle_src_y[f];
        *src_w = &idle_src_w[f];
        *src_h = &idle_src_h[f];
    } else if (tune_anim == TUNE_WALK) {
        *src_x = &walk_src_x[f];
        *src_y = &walk_src_y[f];
        *src_w = &walk_src_w[f];
        *src_h = &walk_src_h[f];
    } else if (tune_anim == TUNE_JUMP) {
        *src_x = &jump_src_x[f];
        *src_y = &jump_src_y[f];
        *src_w = &jump_src_w[f];
        *src_h = &jump_src_h[f];
    } else if (tune_anim == TUNE_SLASH) {
        *src_x = &slash_src_x[f];
        *src_y = &slash_src_y[f];
        *src_w = &slash_src_w[f];
        *src_h = &slash_src_h[f];
    } else if (tune_anim == TUNE_DASH) {
        *src_x = &dash_src_x[0];
        *src_y = &dash_src_y[0];
        *src_w = &dash_src_w[0];
        *src_h = &dash_src_h[0];
    } else {
        *src_x = NULL;
        *src_y = NULL;
        *src_w = NULL;
        *src_h = NULL;
    }
}

static void apply_source_crop(SrcRect *src, int crop_x, int crop_y, int crop_w, int crop_h, int texture_w, int texture_h)
{
    int next_x = src->x + crop_x;
    int next_y = src->y + crop_y;
    if (crop_w < 1) {
        crop_w = 1;
    }
    if (crop_h < 1) {
        crop_h = 1;
    }
    if (next_x < 0) {
        crop_w += next_x;
        next_x = 0;
    }
    if (next_y < 0) {
        crop_h += next_y;
        next_y = 0;
    }
    if (next_x + crop_w > texture_w) {
        crop_w = texture_w - next_x;
    }
    if (next_y + crop_h > texture_h) {
        crop_h = texture_h - next_y;
    }
    if (crop_w < 1) {
        crop_w = 1;
    }
    if (crop_h < 1) {
        crop_h = 1;
    }
    src->x = next_x;
    src->y = next_y;
    src->w = crop_w;
    src->h = crop_h;
}

static bool tune_float_table(const char *name, float **values, int *count)
{
    if (strcmp(name, "idle_draw_w") == 0) {
        *values = idle_draw_w;
        *count = 6;
        return true;
    }
    if (strcmp(name, "idle_draw_h") == 0) {
        *values = idle_draw_h;
        *count = 6;
        return true;
    }
    if (strcmp(name, "idle_ox") == 0) {
        *values = idle_ox;
        *count = 6;
        return true;
    }
    if (strcmp(name, "idle_oy") == 0) {
        *values = idle_oy;
        *count = 6;
        return true;
    }
    if (strcmp(name, "walk_draw_w") == 0) {
        *values = walk_draw_w;
        *count = WALK_FRAME_COUNT;
        return true;
    }
    if (strcmp(name, "walk_draw_h") == 0) {
        *values = walk_draw_h;
        *count = WALK_FRAME_COUNT;
        return true;
    }
    if (strcmp(name, "walk_ox") == 0) {
        *values = walk_ox;
        *count = WALK_FRAME_COUNT;
        return true;
    }
    if (strcmp(name, "walk_oy") == 0) {
        *values = walk_oy;
        *count = WALK_FRAME_COUNT;
        return true;
    }
    if (strcmp(name, "jump_draw_w") == 0) {
        *values = jump_draw_w;
        *count = 4;
        return true;
    }
    if (strcmp(name, "jump_draw_h") == 0) {
        *values = jump_draw_h;
        *count = 4;
        return true;
    }
    if (strcmp(name, "jump_ox") == 0) {
        *values = jump_ox;
        *count = 4;
        return true;
    }
    if (strcmp(name, "jump_oy") == 0) {
        *values = jump_oy;
        *count = 4;
        return true;
    }
    if (strcmp(name, "slash_draw_w") == 0) {
        *values = slash_draw_w;
        *count = 4;
        return true;
    }
    if (strcmp(name, "slash_draw_h") == 0) {
        *values = slash_draw_h;
        *count = 4;
        return true;
    }
    if (strcmp(name, "slash_ox") == 0) {
        *values = slash_ox;
        *count = 4;
        return true;
    }
    if (strcmp(name, "slash_oy") == 0) {
        *values = slash_oy;
        *count = 4;
        return true;
    }
    if (strcmp(name, "dash_draw_w") == 0) {
        *values = dash_draw_w;
        *count = 1;
        return true;
    }
    if (strcmp(name, "dash_draw_h") == 0) {
        *values = dash_draw_h;
        *count = 1;
        return true;
    }
    if (strcmp(name, "dash_ox") == 0) {
        *values = dash_ox;
        *count = 1;
        return true;
    }
    if (strcmp(name, "dash_oy") == 0) {
        *values = dash_oy;
        *count = 1;
        return true;
    }
    return false;
}

static bool tune_int_table(const char *name, int **values, int *count)
{
    if (strcmp(name, "idle_src_x") == 0) {
        *values = idle_src_x;
        *count = 6;
        return true;
    }
    if (strcmp(name, "idle_src_y") == 0) {
        *values = idle_src_y;
        *count = 6;
        return true;
    }
    if (strcmp(name, "idle_src_w") == 0) {
        *values = idle_src_w;
        *count = 6;
        return true;
    }
    if (strcmp(name, "idle_src_h") == 0) {
        *values = idle_src_h;
        *count = 6;
        return true;
    }
    if (strcmp(name, "walk_src_x") == 0) {
        *values = walk_src_x;
        *count = WALK_FRAME_COUNT;
        return true;
    }
    if (strcmp(name, "walk_src_y") == 0) {
        *values = walk_src_y;
        *count = WALK_FRAME_COUNT;
        return true;
    }
    if (strcmp(name, "walk_src_w") == 0) {
        *values = walk_src_w;
        *count = WALK_FRAME_COUNT;
        return true;
    }
    if (strcmp(name, "walk_src_h") == 0) {
        *values = walk_src_h;
        *count = WALK_FRAME_COUNT;
        return true;
    }
    if (strcmp(name, "jump_src_x") == 0) {
        *values = jump_src_x;
        *count = 4;
        return true;
    }
    if (strcmp(name, "jump_src_y") == 0) {
        *values = jump_src_y;
        *count = 4;
        return true;
    }
    if (strcmp(name, "jump_src_w") == 0) {
        *values = jump_src_w;
        *count = 4;
        return true;
    }
    if (strcmp(name, "jump_src_h") == 0) {
        *values = jump_src_h;
        *count = 4;
        return true;
    }
    if (strcmp(name, "slash_src_x") == 0) {
        *values = slash_src_x;
        *count = 4;
        return true;
    }
    if (strcmp(name, "slash_src_y") == 0) {
        *values = slash_src_y;
        *count = 4;
        return true;
    }
    if (strcmp(name, "slash_src_w") == 0) {
        *values = slash_src_w;
        *count = 4;
        return true;
    }
    if (strcmp(name, "slash_src_h") == 0) {
        *values = slash_src_h;
        *count = 4;
        return true;
    }
    if (strcmp(name, "dash_src_x") == 0) {
        *values = dash_src_x;
        *count = 1;
        return true;
    }
    if (strcmp(name, "dash_src_y") == 0) {
        *values = dash_src_y;
        *count = 1;
        return true;
    }
    if (strcmp(name, "dash_src_w") == 0) {
        *values = dash_src_w;
        *count = 1;
        return true;
    }
    if (strcmp(name, "dash_src_h") == 0) {
        *values = dash_src_h;
        *count = 1;
        return true;
    }
    return false;
}

static void write_data_float_table(FILE *out, const char *name, float *values, int count)
{
    fprintf(out, "%s %d", name, count);
    for (int i = 0; i < count; ++i) {
        fprintf(out, " %.1f", values[i]);
    }
    fprintf(out, "\n");
}

static void write_data_int_table(FILE *out, const char *name, int *values, int count)
{
    fprintf(out, "%s %d", name, count);
    for (int i = 0; i < count; ++i) {
        fprintf(out, " %d", values[i]);
    }
    fprintf(out, "\n");
}

static void write_tune_data_tables(FILE *out)
{
    fprintf(out, "# Pilgrim animation tuning data. Edit with PILGRIM_TUNE=1 make run, then press S.\n");
    write_data_float_table(out, "idle_draw_w", idle_draw_w, 6);
    write_data_float_table(out, "idle_draw_h", idle_draw_h, 6);
    write_data_float_table(out, "idle_ox", idle_ox, 6);
    write_data_float_table(out, "idle_oy", idle_oy, 6);
    write_data_int_table(out, "idle_src_x", idle_src_x, 6);
    write_data_int_table(out, "idle_src_y", idle_src_y, 6);
    write_data_int_table(out, "idle_src_w", idle_src_w, 6);
    write_data_int_table(out, "idle_src_h", idle_src_h, 6);
    write_data_int_table(out, "walk_src_x", walk_src_x, WALK_FRAME_COUNT);
    write_data_int_table(out, "walk_src_y", walk_src_y, WALK_FRAME_COUNT);
    write_data_int_table(out, "walk_src_w", walk_src_w, WALK_FRAME_COUNT);
    write_data_int_table(out, "walk_src_h", walk_src_h, WALK_FRAME_COUNT);
    write_data_float_table(out, "walk_draw_w", walk_draw_w, WALK_FRAME_COUNT);
    write_data_float_table(out, "walk_draw_h", walk_draw_h, WALK_FRAME_COUNT);
    write_data_float_table(out, "walk_ox", walk_ox, WALK_FRAME_COUNT);
    write_data_float_table(out, "walk_oy", walk_oy, WALK_FRAME_COUNT);
    write_data_float_table(out, "jump_draw_w", jump_draw_w, 4);
    write_data_float_table(out, "jump_draw_h", jump_draw_h, 4);
    write_data_float_table(out, "jump_ox", jump_ox, 4);
    write_data_float_table(out, "jump_oy", jump_oy, 4);
    write_data_int_table(out, "jump_src_x", jump_src_x, 4);
    write_data_int_table(out, "jump_src_y", jump_src_y, 4);
    write_data_int_table(out, "jump_src_w", jump_src_w, 4);
    write_data_int_table(out, "jump_src_h", jump_src_h, 4);
    write_data_float_table(out, "slash_draw_w", slash_draw_w, 4);
    write_data_float_table(out, "slash_draw_h", slash_draw_h, 4);
    write_data_float_table(out, "slash_ox", slash_ox, 4);
    write_data_float_table(out, "slash_oy", slash_oy, 4);
    write_data_int_table(out, "slash_src_x", slash_src_x, 4);
    write_data_int_table(out, "slash_src_y", slash_src_y, 4);
    write_data_int_table(out, "slash_src_w", slash_src_w, 4);
    write_data_int_table(out, "slash_src_h", slash_src_h, 4);
    write_data_float_table(out, "dash_draw_w", dash_draw_w, 1);
    write_data_float_table(out, "dash_draw_h", dash_draw_h, 1);
    write_data_float_table(out, "dash_ox", dash_ox, 1);
    write_data_float_table(out, "dash_oy", dash_oy, 1);
    write_data_int_table(out, "dash_src_x", dash_src_x, 1);
    write_data_int_table(out, "dash_src_y", dash_src_y, 1);
    write_data_int_table(out, "dash_src_w", dash_src_w, 1);
    write_data_int_table(out, "dash_src_h", dash_src_h, 1);
}

static bool save_tune_data(const char *path)
{
    FILE *out = fopen(path, "w");
    if (!out) {
        fprintf(stderr, "Could not write tuning data to %s\n", path);
        return false;
    }
    write_tune_data_tables(out);
    fclose(out);
    fprintf(stderr, "Saved tuning data to %s\n", path);
    return true;
}

static void load_tune_data(const char *path)
{
    char line[512];
    FILE *in = fopen(path, "r");
    int loaded = 0;
    if (!in) {
        return;
    }
    while (fgets(line, sizeof(line), in)) {
        char *token = strtok(line, " \t\r\n");
        char *count_token;
        int file_count;
        float *float_values = NULL;
        int *int_values = NULL;
        int table_count = 0;
        if (!token || token[0] == '#') {
            continue;
        }
        count_token = strtok(NULL, " \t\r\n");
        if (!count_token) {
            continue;
        }
        file_count = atoi(count_token);
        if (tune_float_table(token, &float_values, &table_count)) {
            int n = file_count < table_count ? file_count : table_count;
            for (int i = 0; i < n; ++i) {
                char *value = strtok(NULL, " \t\r\n");
                if (!value) {
                    break;
                }
                float_values[i] = strtof(value, NULL);
                loaded++;
            }
        } else if (tune_int_table(token, &int_values, &table_count)) {
            int n = file_count < table_count ? file_count : table_count;
            for (int i = 0; i < n; ++i) {
                char *value = strtok(NULL, " \t\r\n");
                if (!value) {
                    break;
                }
                int_values[i] = atoi(value);
                loaded++;
            }
        }
    }
    fclose(in);
    if (loaded > 0) {
        fprintf(stderr, "Loaded %d tuning values from %s\n", loaded, path);
    }
}

static void dump_float_table(FILE *out, const char *name, float *values, int count)
{
    fprintf(out, "static float %s[] = { ", name);
    for (int i = 0; i < count; ++i) {
        fprintf(out, "%.1ff%s", values[i], i == count - 1 ? " " : ", ");
    }
    fprintf(out, "};\n");
}

static void dump_int_table(FILE *out, const char *name, int *values, int count)
{
    fprintf(out, "static int %s[] = { ", name);
    for (int i = 0; i < count; ++i) {
        fprintf(out, "%d%s", values[i], i == count - 1 ? " " : ", ");
    }
    fprintf(out, "};\n");
}

static void dump_tune_tables(FILE *out)
{
    fprintf(out, "\n/* pilgrim animation tuning tables */\n");
    dump_float_table(out, "idle_draw_w", idle_draw_w, 6);
    dump_float_table(out, "idle_draw_h", idle_draw_h, 6);
    dump_float_table(out, "idle_ox", idle_ox, 6);
    dump_float_table(out, "idle_oy", idle_oy, 6);
    dump_int_table(out, "idle_src_x", idle_src_x, 6);
    dump_int_table(out, "idle_src_y", idle_src_y, 6);
    dump_int_table(out, "idle_src_w", idle_src_w, 6);
    dump_int_table(out, "idle_src_h", idle_src_h, 6);
    dump_int_table(out, "walk_src_x", walk_src_x, WALK_FRAME_COUNT);
    dump_int_table(out, "walk_src_y", walk_src_y, WALK_FRAME_COUNT);
    dump_int_table(out, "walk_src_w", walk_src_w, WALK_FRAME_COUNT);
    dump_int_table(out, "walk_src_h", walk_src_h, WALK_FRAME_COUNT);
    dump_float_table(out, "walk_draw_w", walk_draw_w, WALK_FRAME_COUNT);
    dump_float_table(out, "walk_draw_h", walk_draw_h, WALK_FRAME_COUNT);
    dump_float_table(out, "walk_ox", walk_ox, WALK_FRAME_COUNT);
    dump_float_table(out, "walk_oy", walk_oy, WALK_FRAME_COUNT);
    dump_float_table(out, "jump_draw_w", jump_draw_w, 4);
    dump_float_table(out, "jump_draw_h", jump_draw_h, 4);
    dump_float_table(out, "jump_ox", jump_ox, 4);
    dump_float_table(out, "jump_oy", jump_oy, 4);
    dump_int_table(out, "jump_src_x", jump_src_x, 4);
    dump_int_table(out, "jump_src_y", jump_src_y, 4);
    dump_int_table(out, "jump_src_w", jump_src_w, 4);
    dump_int_table(out, "jump_src_h", jump_src_h, 4);
    dump_float_table(out, "slash_draw_w", slash_draw_w, 4);
    dump_float_table(out, "slash_draw_h", slash_draw_h, 4);
    dump_float_table(out, "slash_ox", slash_ox, 4);
    dump_float_table(out, "slash_oy", slash_oy, 4);
    dump_int_table(out, "slash_src_x", slash_src_x, 4);
    dump_int_table(out, "slash_src_y", slash_src_y, 4);
    dump_int_table(out, "slash_src_w", slash_src_w, 4);
    dump_int_table(out, "slash_src_h", slash_src_h, 4);
    dump_float_table(out, "dash_draw_w", dash_draw_w, 1);
    dump_float_table(out, "dash_draw_h", dash_draw_h, 1);
    dump_float_table(out, "dash_ox", dash_ox, 1);
    dump_float_table(out, "dash_oy", dash_oy, 1);
    dump_int_table(out, "dash_src_x", dash_src_x, 1);
    dump_int_table(out, "dash_src_y", dash_src_y, 1);
    dump_int_table(out, "dash_src_w", dash_src_w, 1);
    dump_int_table(out, "dash_src_h", dash_src_h, 1);
    fprintf(out, "/* end pilgrim animation tuning tables */\n\n");
}

static bool export_tune_tables(const char *path)
{
    FILE *out = fopen(path, "w");
    if (!out) {
        fprintf(stderr, "Could not write C tuning tables to %s\n", path);
        return false;
    }
    dump_tune_tables(out);
    fclose(out);
    fprintf(stderr, "Exported C tuning tables to %s\n", path);
    return true;
}

static void reset_tune_frame(void)
{
    int f = tune_frame % tune_frame_count(tune_anim);
    switch (tune_anim) {
        case TUNE_IDLE:
            idle_draw_w[f] = DEFAULT_IDLE_DRAW_W[f];
            idle_draw_h[f] = DEFAULT_IDLE_DRAW_H[f];
            idle_ox[f] = DEFAULT_IDLE_OX[f];
            idle_oy[f] = DEFAULT_IDLE_OY[f];
            idle_src_x[f] = DEFAULT_IDLE_SRC_X[f];
            idle_src_y[f] = DEFAULT_IDLE_SRC_Y[f];
            idle_src_w[f] = DEFAULT_IDLE_SRC_W[f];
            idle_src_h[f] = DEFAULT_IDLE_SRC_H[f];
            break;
        case TUNE_WALK:
            walk_src_x[f] = DEFAULT_WALK_SRC_X[f];
            walk_src_y[f] = DEFAULT_WALK_SRC_Y[f];
            walk_src_w[f] = DEFAULT_WALK_SRC_W[f];
            walk_src_h[f] = DEFAULT_WALK_SRC_H[f];
            walk_draw_w[f] = DEFAULT_WALK_DRAW_W[f];
            walk_draw_h[f] = DEFAULT_WALK_DRAW_H[f];
            walk_ox[f] = DEFAULT_WALK_OX[f];
            walk_oy[f] = DEFAULT_WALK_OY[f];
            break;
        case TUNE_JUMP:
            jump_draw_w[f] = DEFAULT_JUMP_DRAW_W[f];
            jump_draw_h[f] = DEFAULT_JUMP_DRAW_H[f];
            jump_ox[f] = DEFAULT_JUMP_OX[f];
            jump_oy[f] = DEFAULT_JUMP_OY[f];
            jump_src_x[f] = DEFAULT_JUMP_SRC_X[f];
            jump_src_y[f] = DEFAULT_JUMP_SRC_Y[f];
            jump_src_w[f] = DEFAULT_JUMP_SRC_W[f];
            jump_src_h[f] = DEFAULT_JUMP_SRC_H[f];
            break;
        case TUNE_SLASH:
            slash_draw_w[f] = DEFAULT_SLASH_DRAW_W[f];
            slash_draw_h[f] = DEFAULT_SLASH_DRAW_H[f];
            slash_ox[f] = DEFAULT_SLASH_OX[f];
            slash_oy[f] = DEFAULT_SLASH_OY[f];
            slash_src_x[f] = DEFAULT_SLASH_SRC_X[f];
            slash_src_y[f] = DEFAULT_SLASH_SRC_Y[f];
            slash_src_w[f] = DEFAULT_SLASH_SRC_W[f];
            slash_src_h[f] = DEFAULT_SLASH_SRC_H[f];
            break;
        case TUNE_DASH:
            dash_draw_w[0] = DEFAULT_DASH_DRAW_W[0];
            dash_draw_h[0] = DEFAULT_DASH_DRAW_H[0];
            dash_ox[0] = DEFAULT_DASH_OX[0];
            dash_oy[0] = DEFAULT_DASH_OY[0];
            dash_src_x[0] = DEFAULT_DASH_SRC_X[0];
            dash_src_y[0] = DEFAULT_DASH_SRC_Y[0];
            dash_src_w[0] = DEFAULT_DASH_SRC_W[0];
            dash_src_h[0] = DEFAULT_DASH_SRC_H[0];
            break;
        default:
            break;
    }
}

typedef enum {
    TRIM_LEFT,
    TRIM_RIGHT,
    TRIM_TOP,
    TRIM_BOTTOM
} TrimEdge;

static void tune_active_texture_size(int *texture_w, int *texture_h)
{
    Texture *texture = &player_texture;
    if (tune_anim == TUNE_IDLE && idle_texture.ready) {
        texture = &idle_texture;
    } else if (tune_anim == TUNE_WALK && walk_texture.ready) {
        texture = &walk_texture;
    }
    *texture_w = texture->ready ? texture->w : 4096;
    *texture_h = texture->ready ? texture->h : 4096;
}

static bool adjust_tune_source_edge(TrimEdge edge, int delta)
{
    float *draw_w;
    float *draw_h;
    float *ox;
    float *oy;
    int *src_x;
    int *src_y;
    int *src_w;
    int *src_h;
    int base_w;
    int base_h;
    int texture_w;
    int texture_h;
    SrcRect base = tune_base_src_rect(tune_anim, tune_frame);

    tune_active_pose(&draw_w, &draw_h, &ox, &oy);
    tune_active_source(&src_x, &src_y, &src_w, &src_h, &base_w, &base_h);
    tune_active_texture_size(&texture_w, &texture_h);
    if (!src_x || !src_y || !src_w || !src_h || *src_w <= 0 || *src_h <= 0) {
        return false;
    }

    switch (edge) {
        case TRIM_LEFT: {
            int next_x = *src_x + delta;
            int next_w = *src_w - delta;
            float pixels_to_world = *draw_w / (float)*src_w;
            if (base.x + next_x < 0 || next_w < 8 || base.x + next_x + next_w > texture_w) {
                return false;
            }
            *src_x = next_x;
            *src_w = next_w;
            *draw_w = pixels_to_world * (float)next_w;
            *ox -= pixels_to_world * (float)delta;
            return true;
        }
        case TRIM_RIGHT: {
            int next_w = *src_w - delta;
            float pixels_to_world = *draw_w / (float)*src_w;
            if (next_w < 8 || base.x + *src_x + next_w > texture_w) {
                return false;
            }
            *src_w = next_w;
            *draw_w = pixels_to_world * (float)next_w;
            return true;
        }
        case TRIM_TOP: {
            int next_y = *src_y + delta;
            int next_h = *src_h - delta;
            float pixels_to_world = *draw_h / (float)*src_h;
            if (base.y + next_y < 0 || next_h < 8 || base.y + next_y + next_h > texture_h) {
                return false;
            }
            *src_y = next_y;
            *src_h = next_h;
            *draw_h = pixels_to_world * (float)next_h;
            *oy -= pixels_to_world * (float)delta;
            return true;
        }
        case TRIM_BOTTOM: {
            int next_h = *src_h - delta;
            float pixels_to_world = *draw_h / (float)*src_h;
            if (next_h < 8 || base.y + *src_y + next_h > texture_h) {
                return false;
            }
            *src_h = next_h;
            *draw_h = pixels_to_world * (float)next_h;
            return true;
        }
        default:
            return false;
    }
}

static void apply_runtime_source_crop(TuneAnim anim, int frame, Texture *texture, SrcRect *src)
{
    int texture_w = texture->ready ? texture->w : 4096;
    int texture_h = texture->ready ? texture->h : 4096;
    switch (anim) {
        case TUNE_IDLE:
            apply_source_crop(src, idle_src_x[frame % 6], idle_src_y[frame % 6], idle_src_w[frame % 6], idle_src_h[frame % 6], texture_w, texture_h);
            break;
        case TUNE_WALK:
            apply_source_crop(src, walk_src_x[frame % WALK_FRAME_COUNT], walk_src_y[frame % WALK_FRAME_COUNT], walk_src_w[frame % WALK_FRAME_COUNT], walk_src_h[frame % WALK_FRAME_COUNT], texture_w, texture_h);
            break;
        case TUNE_JUMP:
            apply_source_crop(src, jump_src_x[frame % 4], jump_src_y[frame % 4], jump_src_w[frame % 4], jump_src_h[frame % 4], texture_w, texture_h);
            break;
        case TUNE_SLASH:
            apply_source_crop(src, slash_src_x[frame % 4], slash_src_y[frame % 4], slash_src_w[frame % 4], slash_src_h[frame % 4], texture_w, texture_h);
            break;
        case TUNE_DASH:
            apply_source_crop(src, dash_src_x[0], dash_src_y[0], dash_src_w[0], dash_src_h[0], texture_w, texture_h);
            break;
        default:
            break;
    }
}

static void draw_tune_player(void)
{
    int f = tune_frame % tune_frame_count(tune_anim);
    Texture *texture = &player_texture;
    SrcRect src = HERO_IDLE_FRAMES[1];
    float *draw_w;
    float *draw_h;
    float *ox;
    float *oy;
    tune_active_pose(&draw_w, &draw_h, &ox, &oy);

    switch (tune_anim) {
        case TUNE_IDLE:
            texture = idle_texture.ready ? &idle_texture : &player_texture;
            src = idle_texture.ready ? HERO_IDLE_BREATH_FRAMES[f] : HERO_IDLE_FRAMES[f % 4];
            if (idle_texture.ready) {
                apply_source_crop(&src, idle_src_x[f], idle_src_y[f], idle_src_w[f], idle_src_h[f], texture->w, texture->h);
            }
            break;
        case TUNE_WALK:
            texture = walk_texture.ready ? &walk_texture : &player_texture;
            src = walk_texture.ready ? HERO_WALK_SMOOTH_FRAMES[f] : HERO_WALK_FRAMES[f % 6];
            if (walk_texture.ready) {
                apply_source_crop(&src, walk_src_x[f], walk_src_y[f], walk_src_w[f], walk_src_h[f], texture->w, texture->h);
            }
            break;
        case TUNE_JUMP:
            src = HERO_JUMP_FRAMES[f];
            apply_source_crop(&src, jump_src_x[f], jump_src_y[f], jump_src_w[f], jump_src_h[f], texture->w, texture->h);
            break;
        case TUNE_SLASH:
            src = HERO_SLASH_FRAMES[f];
            apply_source_crop(&src, slash_src_x[f], slash_src_y[f], slash_src_w[f], slash_src_h[f], texture->w, texture->h);
            break;
        case TUNE_DASH:
            src = HERO_JUMP_FRAMES[1];
            apply_source_crop(&src, dash_src_x[0], dash_src_y[0], dash_src_w[0], dash_src_h[0], texture->w, texture->h);
            break;
        default:
            break;
    }

    rect_alpha(player.x + 4.0f, player.y - 16.0f, 2.0f, 88.0f, (Color){ 85, 120, 155 }, 120);
    rect_alpha(player.x - 43.0f, player.y + 27.0f, 96.0f, 2.0f, (Color){ 155, 120, 85 }, 120);
    texture_src(texture, src, player.x + 5.0f - *ox, player.y + 28.0f - *oy, *draw_w, *draw_h, false);
}

static void draw_tune_overlay(void)
{
    char buffer[160];
    float *draw_w;
    float *draw_h;
    float *ox;
    float *oy;
    int *src_x;
    int *src_y;
    int *src_w;
    int *src_h;
    int base_w;
    int base_h;
    tune_active_pose(&draw_w, &draw_h, &ox, &oy);
    tune_active_source(&src_x, &src_y, &src_w, &src_h, &base_w, &base_h);

    glPushMatrix();
    glLoadIdentity();
    rect_alpha(5, 5, 142, 72, (Color){ 4, 5, 9 }, 168);
    rect_alpha(154, 5, 156, 86, (Color){ 4, 5, 9 }, 158);
    snprintf(buffer, sizeof(buffer), "%s %s frame %d/%d", tune_trim_mode ? "TRIM" : "TUNE", tune_anim_name(tune_anim), tune_frame + 1, tune_frame_count(tune_anim));
    draw_text(10, 18, buffer, (Color){ 244, 224, 172 });
    snprintf(buffer, sizeof(buffer), "w %.1f  h %.1f", *draw_w, *draw_h);
    draw_text(10, 32, buffer, (Color){ 210, 220, 226 });
    snprintf(buffer, sizeof(buffer), "ox %.1f  oy %.1f", *ox, *oy);
    draw_text(10, 46, buffer, (Color){ 210, 220, 226 });
    if (src_x && src_y && src_w && src_h) {
        snprintf(buffer, sizeof(buffer), "src x %d y %d", *src_x, *src_y);
        draw_text(10, 60, buffer, (Color){ 176, 206, 190 });
        snprintf(buffer, sizeof(buffer), "w %d/%d h %d/%d", *src_w, base_w, *src_h, base_h);
        draw_text(160, 18, buffer, (Color){ 176, 206, 190 });
    }
    draw_text(160, 32, tune_trim_mode ? "Arrows: trim edges" : "Arrows: nudge frame", (Color){ 205, 212, 218 });
    draw_text(160, 46, tune_trim_mode ? "Shift restore  Ctrl x4" : "1-5 anim  [/] frame", (Color){ 178, 187, 196 });
    snprintf(buffer, sizeof(buffer), "Mode T: %s", tune_trim_mode ? "trim" : "nudge");
    draw_text(160, 60, buffer, (Color){ 244, 224, 172 });
    draw_text(160, 74, "Reset O  Save S  Load L", (Color){ 205, 212, 218 });
    draw_text(160, 88, "Export P  Scale +/-  Size Z/X C/V", (Color){ 178, 187, 196 });
    glPopMatrix();
}

static void draw_asset_background(void)
{
    if (!background_texture.ready) {
        return;
    }

    float max_camera = MAP_W * TILE - VIEW_W;
    float t = max_camera > 0.0f ? camera_x / max_camera : 0.0f;
    float rendered_width = background_texture.w * ((float)VIEW_H / background_texture.h);
    float visible_u = (float)VIEW_W / rendered_width;
    if (visible_u > 1.0f) {
        visible_u = 1.0f;
    }
    float u0 = t * (1.0f - visible_u);
    float u1 = u0 + visible_u;

    texture_full_crop(&background_texture, camera_x, 0, VIEW_W, VIEW_H, u0, u1);
    rect_alpha(camera_x, 0, VIEW_W, VIEW_H, (Color){ 3, 5, 11 }, 20);
}

static void draw_fallback_cathedral(void)
{
    rect(camera_x, 0, VIEW_W, VIEW_H, (Color){ 10, 11, 20 });
    rect(camera_x, 128, VIEW_W, 52, (Color){ 17, 16, 24 });

    for (int i = -1; i < 7; ++i) {
        float base = floorf(camera_x / 96.0f) * 96.0f + i * 96.0f;
        float par = camera_x * 0.22f;
        float x = base + par;
        rect(x, 28, 38, 110, (Color){ 25, 25, 36 });
        rect(x + 10, 8, 18, 22, (Color){ 25, 25, 36 });
        rect(x + 13, 16, 12, 8, (Color){ 109, 78, 104 });
        rect(x + 8, 58, 8, 40, (Color){ 55, 72, 95 });
        rect(x + 22, 58, 8, 40, (Color){ 112, 78, 88 });
        rect(x + 16, 58, 4, 40, (Color){ 214, 169, 76 });
    }

    for (int i = -1; i < 12; ++i) {
        float x = floorf(camera_x / 48.0f) * 48.0f + i * 48.0f + camera_x * 0.55f;
        rect(x, 96, 20, 44, (Color){ 35, 35, 47 });
        rect(x + 4, 106, 12, 34, (Color){ 18, 18, 27 });
        rect(x + 6, 110, 8, 30, (Color){ 89, 62, 83 });
    }
}

static void draw_background(void)
{
    if (background_texture.ready) {
        draw_asset_background();
    } else {
        draw_fallback_cathedral();
    }
}

static void draw_midground(void)
{
    if (!midground_texture.ready) {
        return;
    }

    float par = camera_x * 0.45f;
    texture_src(&midground_texture, MG_BELL_TOWER, 120 + par, 26, 48, 116, false);
    texture_src(&midground_texture, MG_TALL_RUIN, 330 + par, 42, 86, 108, false);
    texture_src(&midground_texture, MG_CLOISTER, 575 + par, 52, 110, 98, false);
    texture_src(&midground_texture, MG_CHAPEL, 850 + par, 33, 72, 112, false);
    texture_src(&midground_texture, MG_BRIDGE, 1030 + par, 102, 122, 64, false);
    texture_src(&midground_texture, MG_CROSS, 1235 + par, 58, 42, 92, false);
    texture_src(&midground_texture, MG_WALL, 1405 + par, 80, 90, 80, false);
    rect_alpha(camera_x, 0, VIEW_W, VIEW_H, (Color){ 4, 10, 22 }, 52);
}

static void draw_tile(int tx, int ty)
{
    float x = tx * TILE;
    float y = ty * TILE;
    bool ground = ty == MAP_H - 1;
    if (foreground_texture.ready) {
        SrcRect src = ground ? FG_FLOOR : FG_PLATFORM_M;
        if (!ground) {
            bool left = solid_at(tx - 1, ty);
            bool right = solid_at(tx + 1, ty);
            if (!left && right) {
                src = FG_PLATFORM_L;
            } else if (left && !right) {
                src = FG_PLATFORM_R;
            }
        }
        float h = ground ? 18.0f : 16.0f;
        texture_src(&foreground_texture, src, x, y - 1, TILE, h, false);
        return;
    }

    rect_alpha(x, y + 3, TILE, ground ? TILE : 11, (Color){ 17, 18, 23 }, ground ? 210 : 172);
    rect(x, y, TILE, 3, (Color){ 151, 136, 105 });
    rect(x + 1, y + 3, TILE - 2, 2, (Color){ 79, 73, 68 });
    rect_alpha(x, y + (ground ? TILE - 2 : 12), TILE, 2, (Color){ 7, 9, 14 }, 230);
    rect_alpha(x + 1, y + 6, TILE - 2, 1, (Color){ 56, 54, 59 }, 180);
    if ((tx + ty) % 3 == 0) {
        rect_alpha(x + 3, y + 8, 7, 2, (Color){ 9, 11, 16 }, 190);
    }
    if ((tx * 7 + ty) % 5 == 0) {
        rect(x + 11, y + 5, 2, 2, (Color){ 101, 88, 72 });
    }
}

static void draw_map(void)
{
    int first = (int)floorf(camera_x / TILE) - 1;
    int last = (int)ceilf((camera_x + VIEW_W) / TILE) + 1;
    if (first < 0) {
        first = 0;
    }
    if (last > MAP_W) {
        last = MAP_W;
    }
    for (int y = 0; y < MAP_H; ++y) {
        for (int x = first; x < last; ++x) {
            if (solid_at(x, y)) {
                draw_tile(x, y);
            }
        }
    }
}

static void draw_cross(float x, float y, Color c)
{
    rect(x + 3, y, 2, 12, c);
    rect(x, y + 4, 8, 2, c);
}

static void draw_foreground_details(void)
{
    if (foreground_texture.ready) {
        for (int i = 0; i < 8; ++i) {
            float x = 122 + i * 186;
            if (x >= camera_x - 52 && x <= camera_x + VIEW_W + 52) {
                texture_src(&foreground_texture, FG_COLUMN, x, 60, 26, 122, false);
            }
        }
        for (int i = 0; i < 4; ++i) {
            float x = 256 + i * 315;
            if (x >= camera_x - 78 && x <= camera_x + VIEW_W + 78) {
                texture_src(&foreground_texture, FG_ARCH, x, 72, 58, 78, false);
            }
        }
        for (int i = 0; i < 5; ++i) {
            float x = 410 + i * 244;
            if (x >= camera_x - 44 && x <= camera_x + VIEW_W + 44) {
                texture_src(&foreground_texture, FG_BANNER, x, 63, 34, 86, false);
            }
        }
        for (int i = 0; i < 7; ++i) {
            float x = 210 + i * 164;
            if (x >= camera_x - 34 && x <= camera_x + VIEW_W + 34) {
                texture_src(&foreground_texture, FG_VINES, x, 42, 25, 88, false);
            }
        }
        for (int i = 0; i < 4; ++i) {
            float x = 188 + i * 330;
            if (x >= camera_x - 34 && x <= camera_x + VIEW_W + 34) {
                texture_src(&foreground_texture, FG_LANTERN, x, 86, 18, 52, false);
            }
        }
        texture_src(&foreground_texture, FG_ALTAR, 58, 115, 47, 58, false);
        texture_src(&foreground_texture, FG_WINDOW, 1340, 60, 56, 84, false);
        return;
    }

    for (int i = 0; i < 9; ++i) {
        float x = 205 + i * 156;
        if (x < camera_x - 24 || x > camera_x + VIEW_W + 24) {
            continue;
        }
        rect_alpha(x, 46, 5, 96, (Color){ 9, 12, 16 }, 170);
        rect_alpha(x - 8, 142, 21, 6, (Color){ 9, 12, 16 }, 190);
        for (int j = 0; j < 6; ++j) {
            line(x + 5, 56 + j * 12, x + 12 + (j % 2) * 4, 65 + j * 13, (Color){ 35, 67, 44 });
        }
    }

    for (int i = 0; i < 5; ++i) {
        float x = 385 + i * 244;
        if (x < camera_x - 44 || x > camera_x + VIEW_W + 44) {
            continue;
        }
        rect_alpha(x, 68, 22, 58, (Color){ 65, 19, 35 }, 210);
        tri(x, 126, x + 11, 138, x + 22, 126, (Color){ 65, 19, 35 });
        draw_cross(x + 7, 82, (Color){ 185, 136, 56 });
        rect(x + 2, 72, 18, 2, (Color){ 120, 81, 42 });
    }
}

static void draw_relics(void)
{
    for (int i = 0; i < relic_count; ++i) {
        if (relics[i].taken) {
            continue;
        }
        float pulse = sinf(glutGet(GLUT_ELAPSED_TIME) * 0.006f + i) * 2.0f;
        if (foreground_texture.ready) {
            texture_src(&foreground_texture, FG_ALTAR, relics[i].x - 10, relics[i].y - 5 + pulse, 20, 25, false);
            continue;
        }
        rect(relics[i].x - 5, relics[i].y + pulse, 10, 14, (Color){ 108, 74, 40 });
        rect(relics[i].x - 3, relics[i].y + 2 + pulse, 6, 10, (Color){ 210, 158, 64 });
        draw_cross(relics[i].x - 4, relics[i].y + 1 + pulse, (Color){ 246, 225, 153 });
    }
}

static void draw_lantern(float x, float y)
{
    rect_alpha(x - 5, y - 4, 10, 14, (Color){ 245, 178, 67 }, 70);
    rect(x - 3, y, 6, 9, (Color){ 31, 25, 22 });
    rect(x - 2, y + 1, 4, 7, (Color){ 232, 167, 62 });
    rect(x - 4, y - 2, 8, 2, (Color){ 111, 87, 60 });
    line(x - 3, y - 2, x, y - 6, (Color){ 151, 127, 88 });
    line(x + 3, y - 2, x, y - 6, (Color){ 151, 127, 88 });
}

static void draw_player(void)
{
    float x = player.x;
    float y = player.y;
    if (player_texture.ready) {
        SrcRect src = HERO_IDLE_FRAMES[1];
        float draw_w = 50.0f;
        float draw_h = 59.0f;
        float ox = 22.0f;
        float oy = 56.0f;
        Texture *draw_texture = &player_texture;
        float draw_y_offset = 0.0f;

        if (idle_texture.ready) {
            int f = forced_idle_frame >= 0 ? forced_idle_frame % 6 : ((int)(player.anim_time / 0.22f)) % 6;
            src = HERO_IDLE_BREATH_FRAMES[f];
            draw_texture = &idle_texture;
            apply_runtime_source_crop(TUNE_IDLE, f, draw_texture, &src);
            draw_w = idle_draw_w[f];
            draw_h = idle_draw_h[f];
            ox = idle_ox[f];
            oy = idle_oy[f];
        }

        if (forced_jump_frame >= 0) {
            int f = forced_jump_frame % 4;
            src = HERO_JUMP_FRAMES[f];
            draw_texture = &player_texture;
            apply_runtime_source_crop(TUNE_JUMP, f, draw_texture, &src);
            draw_w = jump_draw_w[f];
            draw_h = jump_draw_h[f];
            ox = jump_ox[f];
            oy = jump_oy[f];
            draw_y_offset = 0.0f;
        } else if (player.attack_timer > 0.0f) {
            int f = (int)(player.attack_age / 0.085f);
            if (f < 0) {
                f = 0;
            }
            if (f > 3) {
                f = 3;
            }
            src = HERO_SLASH_FRAMES[f];
            draw_texture = &player_texture;
            apply_runtime_source_crop(TUNE_SLASH, f, draw_texture, &src);
            draw_w = slash_draw_w[f];
            draw_h = slash_draw_h[f];
            ox = slash_ox[f];
            oy = slash_oy[f];
            draw_y_offset = 0.0f;
        } else if (player.landing_timer > 0.0f) {
            src = HERO_JUMP_FRAMES[3];
            draw_texture = &player_texture;
            apply_runtime_source_crop(TUNE_JUMP, 3, draw_texture, &src);
            draw_w = jump_draw_w[3];
            draw_h = jump_draw_h[3];
            ox = jump_ox[3];
            oy = jump_oy[3];
            draw_y_offset = 0.0f;
        } else if (!player.grounded) {
            int f = 0;
            if (player.vy < -115.0f) {
                f = player.air_time < 0.065f ? 0 : 1;
            } else if (player.vy < 45.0f) {
                f = 2;
            } else {
                f = 3;
            }
            src = HERO_JUMP_FRAMES[f];
            draw_texture = &player_texture;
            apply_runtime_source_crop(TUNE_JUMP, f, draw_texture, &src);
            draw_w = jump_draw_w[f];
            draw_h = jump_draw_h[f];
            ox = jump_ox[f];
            oy = jump_oy[f];
            draw_y_offset = 0.0f;
        } else if (player.backdash_timer > 0.0f) {
            src = HERO_JUMP_FRAMES[1];
            draw_texture = &player_texture;
            apply_runtime_source_crop(TUNE_DASH, 0, draw_texture, &src);
            draw_w = dash_draw_w[0];
            draw_h = dash_draw_h[0];
            ox = dash_ox[0];
            oy = dash_oy[0];
            draw_y_offset = 0.0f;
        } else if (fabsf(player.vx) > 8.0f) {
            if (walk_texture.ready) {
                int f = forced_walk_frame >= 0 ? forced_walk_frame % WALK_FRAME_COUNT : ((int)(player.anim_time / WALK_FRAME_SECONDS)) % WALK_FRAME_COUNT;
                src = HERO_WALK_SMOOTH_FRAMES[f];
                apply_runtime_source_crop(TUNE_WALK, f, &walk_texture, &src);
                draw_w = walk_draw_w[f];
                draw_h = walk_draw_h[f];
                ox = walk_ox[f];
                oy = walk_oy[f];
                texture_src(&walk_texture, src, x + 5.0f - draw_ox_for_facing(draw_w, ox, player.facing), y + 28.0f - oy, draw_w, draw_h, player.facing < 0);
                return;
            } else {
                int f = ((int)(player.anim_time / 0.125f)) % 6;
                src = HERO_WALK_FRAMES[f];
                draw_w = 52.0f;
                draw_h = 62.0f;
                static const float fallback_walk_ox[] = { 23.0f, 24.0f, 25.0f, 25.0f, 24.0f, 23.0f };
                ox = fallback_walk_ox[f];
                oy = 58.0f;
            }
            draw_y_offset = 0.0f;
        }

        texture_src(draw_texture, src, x + 5.0f - draw_ox_for_facing(draw_w, ox, player.facing), y + 28.0f - oy + draw_y_offset, draw_w, draw_h, player.facing < 0);
        return;
    }

    bool dash = player.backdash_timer > 0.0f;
    Color cloak = dash ? (Color){ 83, 43, 72 } : (Color){ 74, 24, 42 };
    float bob = player.grounded ? sinf(glutGet(GLUT_ELAPSED_TIME) * 0.012f) * fabsf(player.vx) * 0.006f : 0.0f;
    int f = player.facing;

    rect_alpha(x - 9 * f, y + 13 + bob, 20 * f, 18, cloak, 220);
    tri(x - 9 * f, y + 13 + bob, x - 20 * f, y + 25 + bob, x + 2 * f, y + 30 + bob, cloak);
    rect(x + 1, y + 8 + bob, 10, 19, (Color){ 24, 22, 27 });
    rect(x + 2, y + 9 + bob, 8, 15, (Color){ 45, 39, 35 });
    rect(x + 1, y + 7 + bob, 11, 4, (Color){ 98, 86, 75 });
    rect(x + 3, y + 2 + bob, 7, 7, (Color){ 171, 124, 92 });
    rect(x + 2, y + 1 + bob, 9, 3, (Color){ 20, 17, 18 });
    rect(x + 8, y + 4 + bob, 3, 4, (Color){ 18, 15, 16 });
    rect(x + 3, y + 24 + bob, 3, 7, (Color){ 20, 19, 23 });
    rect(x + 8, y + 24 + bob, 3, 7, (Color){ 20, 19, 23 });
    rect(x + 2, y + 30 + bob, 5, 2, (Color){ 11, 12, 16 });
    rect(x + 7, y + 30 + bob, 6, 2, (Color){ 11, 12, 16 });
    rect(x + 9, y + 13 + bob, 8 * f, 3, (Color){ 123, 91, 61 });
    rect(x + 13 * f, y + 14 + bob, 2 * f, 13, (Color){ 55, 44, 35 });
    rect(x + 12 * f, y + 27 + bob, 4 * f, 2, (Color){ 190, 143, 68 });
    draw_lantern(x + 15 * f, y + 25 + bob);
    rect(x - 3, y + 16 + bob, 3, 5, (Color){ 213, 185, 103 });
    draw_cross(x - 4, y + 14 + bob, (Color){ 238, 209, 127 });

    if (player.attack_timer > 0.0f) {
        float sx = player.facing > 0 ? x + 13 : x - 27;
        rect_alpha(sx - 1, y + 6 + bob, 26, 6, (Color){ 213, 225, 232 }, 70);
        line(sx, y + 12 + bob, sx + 28 * f, y + 2 + bob, (Color){ 232, 237, 233 });
        line(sx, y + 13 + bob, sx + 27 * f, y + 3 + bob, (Color){ 128, 154, 170 });
    }
}

static void draw_candles(void)
{
    for (int i = 0; i < 14; ++i) {
        float x = 92 + i * 101;
        float y = 160;
        if (x < camera_x - 16 || x > camera_x + VIEW_W + 16) {
            continue;
        }
        if (foreground_texture.ready) {
            texture_src(&foreground_texture, (i % 3 == 0) ? FG_CANDELABRA : FG_CANDLES, x - 12, y - 34, 24, 36, false);
            continue;
        }
        rect(x, y - 10, 4, 10, (Color){ 219, 203, 168 });
        rect_alpha(x - 7, y - 20, 18, 22, (Color){ 241, 151, 51 }, 45);
        rect(x + 1, y - 14, 2, 4, (Color){ 244, 177, 61 });
        rect(x, y - 15, 4, 2, (Color){ 247, 220, 113 });
    }
}

static void draw_hud(void)
{
    char buffer[96];
    int elapsed = glutGet(GLUT_ELAPSED_TIME);
    glPushMatrix();
    glLoadIdentity();
    rect_alpha(6, 7, 76, 17, (Color){ 5, 6, 10 }, 170);
    rect_alpha(7, 8, 74, 1, (Color){ 202, 163, 83 }, 130);
    snprintf(buffer, sizeof(buffer), "Relics %d/%d", player.relics, relic_count);
    draw_text(12, 20, buffer, (Color){ 238, 224, 180 });
    int hint_end = player_interacted ? 3200 : 9000;
    int hint_fade_start = player_interacted ? 1400 : 6500;
    if (elapsed < hint_end) {
        float fade = elapsed < hint_fade_start ? 1.0f : 1.0f - (elapsed - hint_fade_start) / (float)(hint_end - hint_fade_start);
        unsigned char panel_a = (unsigned char)(120.0f * fade);
        unsigned char text_a = (unsigned char)(150.0f * fade);
        rect_alpha(6, 150, 112, 15, (Color){ 5, 6, 10 }, panel_a);
        glColor4ub(142, 143, 151, text_a);
        glRasterPos2f(12, 162);
        const char *hint = "A/D  Space  J  K";
        for (const char *p = hint; *p; ++p) {
            glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *p);
        }
    }
    glPopMatrix();
}

static void maybe_capture_frame(void)
{
    const char *path = getenv("PILGRIM_CAPTURE");
    if (!path || captured_frame) {
        return;
    }
    if (capture_frame > 0 && sim_frame < capture_frame) {
        return;
    }

    int w = VIEW_W * SCALE;
    int h = VIEW_H * SCALE;
    unsigned char *pixels = malloc((size_t)w * (size_t)h * 3);
    if (!pixels) {
        return;
    }

    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    FILE *fp = fopen(path, "wb");
    if (fp) {
        fprintf(fp, "P6\n%d %d\n255\n", w, h);
        for (int y = h - 1; y >= 0; --y) {
            fwrite(pixels + (size_t)y * (size_t)w * 3, 1, (size_t)w * 3, fp);
        }
        fclose(fp);
    }
    free(pixels);
    captured_frame = true;
    if (capture_frame > 0) {
        exit(0);
    }
}

static void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    if (tune_mode && isolate_player) {
        glTranslatef(130.0f - player.x, 132.0f - player.y, 0);
        draw_tune_player();
        glLoadIdentity();
        draw_tune_overlay();
        maybe_capture_frame();
        glutSwapBuffers();
        return;
    }
    if (isolate_player) {
        glTranslatef(130.0f - player.x, 132.0f - player.y, 0);
        draw_player();
        maybe_capture_frame();
        glutSwapBuffers();
        return;
    }
    glTranslatef(-camera_x, 0, 0);

    draw_background();
    draw_midground();
    draw_candles();
    draw_relics();
    draw_map();
    draw_foreground_details();
    draw_particles();
    if (tune_mode) {
        draw_tune_player();
    } else {
        draw_player();
    }

    glLoadIdentity();
    draw_hud();
    if (tune_mode) {
        draw_tune_overlay();
    }

    maybe_capture_frame();
    glutSwapBuffers();
}

static void tick(int value)
{
    (void)value;
    int now = glutGet(GLUT_ELAPSED_TIME);
    float frame = (now - last_time_ms) / 1000.0f;
    if (frame > 0.08f) {
        frame = 0.08f;
    }
    last_time_ms = now;
    accumulator += frame;

    while (accumulator >= FIXED_DT) {
        update(FIXED_DT);
        accumulator -= FIXED_DT;
    }

    glutPostRedisplay();
    glutTimerFunc(1, tick, 0);
}

static void reshape(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, VIEW_W, VIEW_H, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void key_down(unsigned char key, int x, int y)
{
    (void)x;
    (void)y;
    if (tune_mode) {
        float *draw_w;
        float *draw_h;
        float *ox;
        float *oy;
        tune_active_pose(&draw_w, &draw_h, &ox, &oy);

        if (key == 27) {
            exit(0);
        } else if (key >= '1' && key <= '5') {
            tune_anim = (TuneAnim)(key - '1');
            tune_frame = 0;
        } else if (key == '[') {
            tune_frame = (tune_frame + tune_frame_count(tune_anim) - 1) % tune_frame_count(tune_anim);
        } else if (key == ']') {
            tune_frame = (tune_frame + 1) % tune_frame_count(tune_anim);
        } else if (key == '=' || key == '+') {
            *draw_w += 0.5f;
            *draw_h += 0.5f;
        } else if ((key == '-' || key == '_') && *draw_w > 4.0f && *draw_h > 4.0f) {
            *draw_w -= 0.5f;
            *draw_h -= 0.5f;
        } else if (key == 'z' || key == 'Z') {
            if (*draw_w > 4.0f) {
                *draw_w -= 0.5f;
            }
        } else if (key == 'x' || key == 'X') {
            *draw_w += 0.5f;
        } else if (key == 'v' || key == 'V') {
            *draw_h += 0.5f;
        } else if ((key == 'c' || key == 'C') && *draw_h > 4.0f) {
            *draw_h -= 0.5f;
        } else if (key == 't' || key == 'T') {
            tune_trim_mode = !tune_trim_mode;
        } else if (key == 'o' || key == 'O') {
            reset_tune_frame();
        } else if (key == 's' || key == 'S') {
            save_tune_data(tune_file_path);
        } else if (key == 'l' || key == 'L') {
            load_tune_data(tune_file_path);
        } else if (key == 'p' || key == 'P') {
            dump_tune_tables(stderr);
            export_tune_tables(tune_export_path);
        } else {
            keys[key] = true;
        }
        glutPostRedisplay();
        return;
    }
    keys[key] = true;
}

static void key_up(unsigned char key, int x, int y)
{
    (void)x;
    (void)y;
    if (tune_mode) {
        keys[key] = false;
        return;
    }
    keys[key] = false;
}

static void special_down(int key, int x, int y)
{
    (void)x;
    (void)y;
    if (tune_mode) {
        float *draw_w;
        float *draw_h;
        float *ox;
        float *oy;
        float step = 0.5f;
        int trim_step = 1;
        int modifiers = glutGetModifiers();
        tune_active_pose(&draw_w, &draw_h, &ox, &oy);
        (void)draw_w;
        (void)draw_h;
        if (modifiers & GLUT_ACTIVE_SHIFT) {
            step = 0.1f;
        } else if (modifiers & GLUT_ACTIVE_CTRL) {
            step = 2.0f;
        }
        if (modifiers & GLUT_ACTIVE_CTRL) {
            trim_step = 4;
        }
        if (modifiers & GLUT_ACTIVE_SHIFT) {
            trim_step = -trim_step;
        }
        if (tune_trim_mode && key == GLUT_KEY_LEFT) {
            adjust_tune_source_edge(TRIM_LEFT, trim_step);
        } else if (tune_trim_mode && key == GLUT_KEY_RIGHT) {
            adjust_tune_source_edge(TRIM_RIGHT, trim_step);
        } else if (tune_trim_mode && key == GLUT_KEY_UP) {
            adjust_tune_source_edge(TRIM_TOP, trim_step);
        } else if (tune_trim_mode && key == GLUT_KEY_DOWN) {
            adjust_tune_source_edge(TRIM_BOTTOM, trim_step);
        } else if (key == GLUT_KEY_LEFT) {
            *ox += step;
        } else if (key == GLUT_KEY_RIGHT) {
            *ox -= step;
        } else if (key == GLUT_KEY_UP) {
            *oy += step;
        } else if (key == GLUT_KEY_DOWN) {
            *oy -= step;
        }
        glutPostRedisplay();
        return;
    }
    if (key >= 0 && key < 256) {
        special_keys[key] = true;
    }
}

static void special_up(int key, int x, int y)
{
    (void)x;
    (void)y;
    if (tune_mode) {
        return;
    }
    if (key >= 0 && key < 256) {
        special_keys[key] = false;
    }
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(VIEW_W * SCALE, VIEW_H * SCALE);
    glutCreateWindow("Pilgrim of the Thorn");
    glClearColor(0.03f, 0.03f, 0.06f, 1.0f);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(1.0f);

    if (!load_png_texture_keyed("assets/cathedral_background.png", &background_texture, false)) {
        fprintf(stderr, "Could not load assets/cathedral_background.png; using procedural fallback background.\n");
    }
    if (!load_png_texture_keyed("assets/midground_sheet_source.png", &midground_texture, true)) {
        fprintf(stderr, "Could not load assets/midground_sheet_source.png; skipping midground parallax.\n");
    }
    if (!load_png_texture_keyed("assets/foreground_sheet_source.png", &foreground_texture, true)) {
        fprintf(stderr, "Could not load assets/foreground_sheet_source.png; using procedural fallback props.\n");
    }
    if (!load_png_texture_keyed("assets/pilgrim_sheet_expanded_source.png", &player_texture, true)) {
        fprintf(stderr, "Could not load assets/pilgrim_sheet_expanded_source.png; using procedural fallback player.\n");
    }
    if (!load_png_texture_keyed("assets/pilgrim_idle_breath_source.png", &idle_texture, true)) {
        fprintf(stderr, "Could not load assets/pilgrim_idle_breath_source.png; using fallback idle frames.\n");
    }
    if (!load_png_texture_keyed("assets/pilgrim_walk_cycle_source.png", &walk_texture, true)) {
        fprintf(stderr, "Could not load assets/pilgrim_walk_cycle_source.png; using fallback walk frames.\n");
    }

    reshape(VIEW_W * SCALE, VIEW_H * SCALE);
    const char *tune_file_env = getenv("PILGRIM_TUNE_FILE");
    if (tune_file_env && tune_file_env[0] != '\0') {
        tune_file_path = tune_file_env;
    }
    const char *tune_export_env = getenv("PILGRIM_TUNE_EXPORT");
    if (tune_export_env && tune_export_env[0] != '\0') {
        tune_export_path = tune_export_env;
    }
    load_tune_data(tune_file_path);
    capture_script = getenv("PILGRIM_SCRIPT");
    const char *capture_frame_env = getenv("PILGRIM_CAPTURE_FRAME");
    if (capture_frame_env) {
        capture_frame = atoi(capture_frame_env);
    }
    const char *forced_walk_env = getenv("PILGRIM_FORCE_WALK_FRAME");
    if (forced_walk_env) {
        forced_walk_frame = atoi(forced_walk_env);
    }
    const char *forced_idle_env = getenv("PILGRIM_FORCE_IDLE_FRAME");
    if (forced_idle_env) {
        forced_idle_frame = atoi(forced_idle_env);
    }
    const char *forced_jump_env = getenv("PILGRIM_FORCE_JUMP_FRAME");
    if (forced_jump_env) {
        forced_jump_frame = atoi(forced_jump_env);
    }
    const char *forced_facing_env = getenv("PILGRIM_FORCE_FACING");
    if (forced_facing_env) {
        forced_facing = atoi(forced_facing_env) < 0 ? -1 : 1;
        player.facing = forced_facing;
    }
    tune_mode = getenv("PILGRIM_TUNE") != NULL;
    if (tune_mode) {
        tune_anim = tune_anim_from_name(getenv("PILGRIM_TUNE_ANIM"));
        const char *tune_frame_env = getenv("PILGRIM_TUNE_FRAME");
        if (tune_frame_env) {
            tune_frame = atoi(tune_frame_env);
        }
        if (tune_frame < 0) {
            tune_frame = 0;
        }
        tune_frame %= tune_frame_count(tune_anim);
    }
    isolate_player = getenv("PILGRIM_ISOLATE_PLAYER") != NULL;
    if (tune_mode) {
        player.x = 42.0f;
        player.y = 148.0f;
        player.vx = 0.0f;
        player.vy = 0.0f;
        player.grounded = true;
        player.facing = forced_facing ? forced_facing : 1;
    }
    telemetry_path = getenv("PILGRIM_TELEMETRY");
    if (telemetry_path) {
        telemetry_file = fopen(telemetry_path, "w");
        if (telemetry_file) {
            fprintf(telemetry_file, "frame x y vx vy grounded air_time camera_x\n");
            atexit(close_telemetry);
        }
    }
    last_time_ms = glutGet(GLUT_ELAPSED_TIME);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(key_down);
    glutKeyboardUpFunc(key_up);
    glutSpecialFunc(special_down);
    glutSpecialUpFunc(special_up);
    glutTimerFunc(1, tick, 0);
    glutMainLoop();
    return 0;
}
