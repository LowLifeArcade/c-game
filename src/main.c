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

static const char *map_rows[MAP_H] = {
    "................................................................................................",
    "................................................................................................",
    "................................................................................................",
    "......................##.................................###..............................###.....",
    ".......................#.................##...............#....................##..........#......",
    "...............##......#............................##....#...........###..................#......",
    "...............#.......#.............##.............#.....#............#.............##....#......",
    ".....##........#.....................#..............#..................#.............#.....#......",
    ".....#.....................##........#.........................##....................#............",
    ".................###.......#...................###.............#..............###.................",
    ".................###...........................###............................###.................",
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
static const SrcRect HERO_WALK_FRAMES[] = {
    { 44, 305, 215, 260 },
    { 272, 305, 215, 260 },
    { 500, 305, 215, 260 },
    { 728, 305, 220, 260 },
    { 942, 305, 225, 260 },
    { 1165, 305, 215, 260 },
};
static const SrcRect HERO_WALK_SMOOTH_FRAMES[] = {
    { 0, 145, 217, 395 },
    { 217, 145, 217, 395 },
    { 434, 145, 217, 395 },
    { 651, 145, 217, 395 },
    { 868, 145, 217, 395 },
    { 1085, 145, 217, 395 },
    { 1302, 145, 217, 395 },
    { 1519, 145, 217, 395 },
    { 1736, 145, 217, 395 },
    { 1953, 145, 217, 395 },
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
static const char *capture_script;
static const char *telemetry_path;
static FILE *telemetry_file;
static float accumulator;
static Texture background_texture;
static Texture foreground_texture;
static Texture player_texture;
static Texture walk_texture;
static Texture midground_texture;
static bool captured_frame;
static bool player_interacted;
static bool isolate_player;

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
    if (axis != 0 && player.backdash_timer <= 0.0f) {
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
        player.anim_time += dt * (0.65f + fabsf(player.vx) / 58.0f);
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
        float draw_y_offset = sinf(player.anim_time * 4.2f) * 0.45f;

        if (player.attack_timer > 0.0f) {
            int f = (int)(player.attack_age / 0.085f);
            if (f < 0) {
                f = 0;
            }
            if (f > 3) {
                f = 3;
            }
            src = HERO_SLASH_FRAMES[f];
            draw_w = f == 2 ? 91.0f : (f == 1 ? 74.0f : 58.0f);
            draw_h = f == 2 ? 60.0f : 57.0f;
            ox = player.facing > 0 ? 23.0f : draw_w - 24.0f;
            oy = 55.0f;
            draw_y_offset = 0.0f;
        } else if (player.landing_timer > 0.0f) {
            src = HERO_JUMP_FRAMES[3];
            draw_w = 61.0f;
            draw_h = 55.0f;
            ox = 28.0f;
            oy = 53.0f;
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
            draw_w = f == 0 ? 54.0f : 61.0f;
            draw_h = f == 0 ? 51.0f : (f == 3 ? 55.0f : 58.0f);
            ox = f == 3 ? 28.0f : 26.0f;
            oy = f == 0 ? 49.0f : (f == 3 ? 53.0f : 55.0f);
            draw_y_offset = 0.0f;
        } else if (fabsf(player.vx) > 8.0f) {
            if (walk_texture.ready) {
                int f = forced_walk_frame >= 0 ? forced_walk_frame % 10 : ((int)(player.anim_time / 0.118f)) % 10;
                src = HERO_WALK_SMOOTH_FRAMES[f];
                draw_w = 49.0f;
                draw_h = 68.0f;
                static const float walk_ox[] = { 23.0f, 23.5f, 24.0f, 24.0f, 23.5f, 23.0f, 22.5f, 22.5f, 23.0f, 23.0f };
                ox = walk_ox[f];
                oy = 64.0f;
                texture_src(&walk_texture, src, x + 5.0f - ox, y + 28.0f - oy, draw_w, draw_h, player.facing < 0);
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

        texture_src(&player_texture, src, x + 5.0f - ox, y + 28.0f - oy + draw_y_offset, draw_w, draw_h, player.facing < 0);
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
    draw_player();

    glLoadIdentity();
    draw_hud();

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
    keys[key] = true;
}

static void key_up(unsigned char key, int x, int y)
{
    (void)x;
    (void)y;
    keys[key] = false;
}

static void special_down(int key, int x, int y)
{
    (void)x;
    (void)y;
    if (key >= 0 && key < 256) {
        special_keys[key] = true;
    }
}

static void special_up(int key, int x, int y)
{
    (void)x;
    (void)y;
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
    if (!load_png_texture_keyed("assets/pilgrim_walk_cycle_source.png", &walk_texture, true)) {
        fprintf(stderr, "Could not load assets/pilgrim_walk_cycle_source.png; using fallback walk frames.\n");
    }

    reshape(VIEW_W * SCALE, VIEW_H * SCALE);
    capture_script = getenv("PILGRIM_SCRIPT");
    const char *capture_frame_env = getenv("PILGRIM_CAPTURE_FRAME");
    if (capture_frame_env) {
        capture_frame = atoi(capture_frame_env);
    }
    const char *forced_walk_env = getenv("PILGRIM_FORCE_WALK_FRAME");
    if (forced_walk_env) {
        forced_walk_frame = atoi(forced_walk_env);
    }
    isolate_player = getenv("PILGRIM_ISOLATE_PLAYER") != NULL;
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
