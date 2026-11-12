#ifndef MOON_SOUND_WORLD_STATE_H
#define MOON_SOUND_WORLD_STATE_H
static int ma_world_enter_observed(PTR world,unsigned restoring){
    if(ma_world&&(!ma_exists(dget(ma_world))||dget(ma_world)!=world))ma_end();
    if(!world||restoring)return 0;
    if(!ma_world){DRoot root=dhold(world);if(!root)return 0;ma_world=root;ma_sealed=1;ma_ready=1;ma_event("world-audio-ready",ma_count,0);}
    return 1;
}
/* Retained for direct native callers; read failure must precede teardown. */
static inline int ma_world_enter(PTR world){
    int ok=1;unsigned restoring=world?(unsigned)boolean(ma_restore,world,&ok):0;
    return ok?ma_world_enter_observed(world,restoring):0;
}
#endif
