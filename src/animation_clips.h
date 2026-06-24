#ifndef PILGRIM_ANIMATION_CLIPS_H
#define PILGRIM_ANIMATION_CLIPS_H

#include <stdbool.h>

#define ANIMATION_CLIP_MAX 16
#define ANIMATION_FRAME_MAX 32
#define ANIMATION_NAME_MAX 32
#define ANIMATION_PATH_MAX 256

typedef enum {
    CLIP_IDLE,
    CLIP_WALK,
    CLIP_JUMP,
    CLIP_SLASH,
    CLIP_DASH,
    CLIP_ANIM_COUNT
} ClipAnim;

typedef struct {
    int x, y, w, h;
    int crop_x, crop_y, crop_w, crop_h;
    float draw_w, draw_h;
    float ox, oy;
} AnimationFrame;

typedef struct {
    ClipAnim anim;
    char name[ANIMATION_NAME_MAX];
    char sheet_path[ANIMATION_PATH_MAX];
    int frame_count;
    float frame_seconds;
    AnimationFrame frames[ANIMATION_FRAME_MAX];
    AnimationFrame defaults[ANIMATION_FRAME_MAX];
} AnimationClip;

typedef struct {
    AnimationClip clips[ANIMATION_CLIP_MAX];
    int clip_count;
    int active[CLIP_ANIM_COUNT];
} AnimationLibrary;

const char *clip_anim_name(ClipAnim anim);
ClipAnim clip_anim_from_name(const char *name);
bool animation_library_load(AnimationLibrary *library, const char *path);
bool animation_library_save(const AnimationLibrary *library, const char *path);
AnimationClip *animation_library_active(AnimationLibrary *library, ClipAnim anim);
const AnimationClip *animation_library_active_const(const AnimationLibrary *library, ClipAnim anim);
AnimationClip *animation_library_select(AnimationLibrary *library, ClipAnim anim, const char *name);
AnimationClip *animation_library_cycle(AnimationLibrary *library, ClipAnim anim, int direction);

#endif
