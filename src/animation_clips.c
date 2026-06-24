#include "animation_clips.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *clip_anim_name(ClipAnim anim)
{
    switch (anim) {
        case CLIP_IDLE: return "idle";
        case CLIP_WALK: return "walk";
        case CLIP_JUMP: return "jump";
        case CLIP_SLASH: return "slash";
        case CLIP_DASH: return "dash";
        default: return "unknown";
    }
}

ClipAnim clip_anim_from_name(const char *name)
{
    if (name && strcmp(name, "walk") == 0) return CLIP_WALK;
    if (name && strcmp(name, "jump") == 0) return CLIP_JUMP;
    if (name && (strcmp(name, "slash") == 0 || strcmp(name, "attack") == 0)) return CLIP_SLASH;
    if (name && strcmp(name, "dash") == 0) return CLIP_DASH;
    return CLIP_IDLE;
}

static int find_clip(const AnimationLibrary *library, const char *name)
{
    for (int i = 0; i < library->clip_count; ++i) {
        if (strcmp(library->clips[i].name, name) == 0) return i;
    }
    return -1;
}

bool animation_library_load(AnimationLibrary *library, const char *path)
{
    char line[1024];
    char active_names[CLIP_ANIM_COUNT][ANIMATION_NAME_MAX] = {{ 0 }};
    FILE *in = fopen(path, "r");
    if (!in) return false;

    memset(library, 0, sizeof(*library));
    for (int i = 0; i < CLIP_ANIM_COUNT; ++i) library->active[i] = -1;

    while (fgets(line, sizeof(line), in)) {
        char command[32];
        if (line[0] == '#' || sscanf(line, "%31s", command) != 1) continue;

        if (strcmp(command, "clip") == 0) {
            char anim_name[32], name[ANIMATION_NAME_MAX], sheet[ANIMATION_PATH_MAX];
            int count;
            float seconds;
            if (library->clip_count >= ANIMATION_CLIP_MAX) continue;
            if (sscanf(line, "clip %31s %31s %255s %d %f", anim_name, name, sheet, &count, &seconds) != 5) continue;
            if (count < 1 || count > ANIMATION_FRAME_MAX) continue;
            AnimationClip *clip = &library->clips[library->clip_count++];
            clip->anim = clip_anim_from_name(anim_name);
            snprintf(clip->name, sizeof(clip->name), "%s", name);
            snprintf(clip->sheet_path, sizeof(clip->sheet_path), "%s", sheet);
            clip->frame_count = count;
            clip->frame_seconds = seconds;
        } else if (strcmp(command, "frame") == 0) {
            char name[ANIMATION_NAME_MAX];
            int index;
            AnimationFrame frame;
            if (sscanf(line,
                "frame %31s %d %d %d %d %d %d %d %d %d %f %f %f %f",
                name, &index, &frame.x, &frame.y, &frame.w, &frame.h,
                &frame.crop_x, &frame.crop_y, &frame.crop_w, &frame.crop_h,
                &frame.draw_w, &frame.draw_h, &frame.ox, &frame.oy) != 14) continue;
            int clip_index = find_clip(library, name);
            if (clip_index < 0 || index < 0 || index >= library->clips[clip_index].frame_count) continue;
            library->clips[clip_index].frames[index] = frame;
            library->clips[clip_index].defaults[index] = frame;
        } else if (strcmp(command, "active") == 0) {
            char anim_name[32], name[ANIMATION_NAME_MAX];
            if (sscanf(line, "active %31s %31s", anim_name, name) == 2) {
                ClipAnim anim = clip_anim_from_name(anim_name);
                snprintf(active_names[anim], sizeof(active_names[anim]), "%s", name);
            }
        }
    }
    fclose(in);

    for (int anim = 0; anim < CLIP_ANIM_COUNT; ++anim) {
        for (int i = 0; i < library->clip_count; ++i) {
            if ((int)library->clips[i].anim != anim) continue;
            if (library->active[anim] < 0) library->active[anim] = i;
            if (active_names[anim][0] && strcmp(library->clips[i].name, active_names[anim]) == 0) {
                library->active[anim] = i;
            }
        }
    }
    return library->clip_count > 0;
}

bool animation_library_save(const AnimationLibrary *library, const char *path)
{
    FILE *out = fopen(path, "w");
    if (!out) return false;
    fprintf(out, "# Animation clips: clip <anim> <name> <sheet> <frames> <seconds>\n");
    fprintf(out, "# frame <clip> <index> <x> <y> <w> <h> <crop_x> <crop_y> <crop_w> <crop_h> <draw_w> <draw_h> <ox> <oy>\n");
    for (int i = 0; i < library->clip_count; ++i) {
        const AnimationClip *clip = &library->clips[i];
        fprintf(out, "clip %s %s %s %d %.3f\n", clip_anim_name(clip->anim), clip->name,
            clip->sheet_path, clip->frame_count, clip->frame_seconds);
        for (int f = 0; f < clip->frame_count; ++f) {
            const AnimationFrame *frame = &clip->frames[f];
            fprintf(out, "frame %s %d %d %d %d %d %d %d %d %d %.1f %.1f %.1f %.1f\n",
                clip->name, f, frame->x, frame->y, frame->w, frame->h,
                frame->crop_x, frame->crop_y, frame->crop_w, frame->crop_h,
                frame->draw_w, frame->draw_h, frame->ox, frame->oy);
        }
        fprintf(out, "\n");
    }
    for (int anim = 0; anim < CLIP_ANIM_COUNT; ++anim) {
        int index = library->active[anim];
        if (index >= 0) fprintf(out, "active %s %s\n", clip_anim_name((ClipAnim)anim), library->clips[index].name);
    }
    fclose(out);
    return true;
}

AnimationClip *animation_library_active(AnimationLibrary *library, ClipAnim anim)
{
    int index = library->active[anim];
    return index >= 0 ? &library->clips[index] : NULL;
}

const AnimationClip *animation_library_active_const(const AnimationLibrary *library, ClipAnim anim)
{
    int index = library->active[anim];
    return index >= 0 ? &library->clips[index] : NULL;
}

AnimationClip *animation_library_select(AnimationLibrary *library, ClipAnim anim, const char *name)
{
    for (int i = 0; i < library->clip_count; ++i) {
        if (library->clips[i].anim == anim && strcmp(library->clips[i].name, name) == 0) {
            library->active[anim] = i;
            return &library->clips[i];
        }
    }
    return NULL;
}

AnimationClip *animation_library_cycle(AnimationLibrary *library, ClipAnim anim, int direction)
{
    int matches[ANIMATION_CLIP_MAX];
    int count = 0;
    int current = -1;
    for (int i = 0; i < library->clip_count; ++i) {
        if (library->clips[i].anim != anim) continue;
        if (i == library->active[anim]) current = count;
        matches[count++] = i;
    }
    if (count == 0) return NULL;
    if (current < 0) current = 0;
    current = (current + direction + count) % count;
    library->active[anim] = matches[current];
    return &library->clips[matches[current]];
}
