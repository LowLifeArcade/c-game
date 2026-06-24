#include "../src/animation_clips.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    AnimationLibrary library;
    AnimationLibrary reloaded;
    AnimationClip *clip;

    assert(animation_library_load(&library, "assets/pilgrim_animation_clips.txt"));
    clip = animation_library_active(&library, CLIP_WALK);
    assert(clip && strcmp(clip->name, "pilgrim_walk_8_consistent") == 0 && clip->frame_count == 8);

    clip = animation_library_cycle(&library, CLIP_WALK, 1);
    assert(clip && strcmp(clip->name, "pilgrim_walk_8_original_mix") == 0 && clip->frame_count == 8);
    clip = animation_library_cycle(&library, CLIP_WALK, 1);
    assert(clip && strcmp(clip->name, "pilgrim_walk_4") == 0 && clip->frame_count == 4);
    clip = animation_library_cycle(&library, CLIP_WALK, 1);
    assert(clip && strcmp(clip->name, "pilgrim_walk_8_consistent") == 0 && clip->frame_count == 8);

    assert(animation_library_select(&library, CLIP_JUMP, "pilgrim_jump_4"));
    assert(animation_library_select(&library, CLIP_SLASH, "pilgrim_slash_4"));
    assert(animation_library_save(&library, "/tmp/pilgrim_animation_clips_test.txt"));
    assert(animation_library_load(&reloaded, "/tmp/pilgrim_animation_clips_test.txt"));
    assert(animation_library_active(&reloaded, CLIP_WALK)->frame_count == 8);
    assert(animation_library_active(&reloaded, CLIP_JUMP)->frame_count == 4);
    assert(animation_library_active(&reloaded, CLIP_SLASH)->frame_count == 4);

    puts("animation clip manifest, selection, cycling, and persistence: ok");
    return 0;
}
