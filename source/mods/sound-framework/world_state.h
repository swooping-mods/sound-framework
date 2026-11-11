#ifndef MOON_SOUND_WORLD_STATE_H
#define MOON_SOUND_WORLD_STATE_H
static int ma_world_enter(PTR world){
    if(ma_world&&(!ma_exists(dget(ma_world))||dget(ma_world)!=world))ma_end();
    if(!world)return 0;int ok=1;if(boolean(ma_restore,world,&ok)||!ok)return 0;
    if(!ma_world){DRoot root=dhold(world);if(!root)return 0;ma_world=root;ma_sealed=1;ma_ready=1;ma_event("world-audio-ready",ma_count,0);}
    return 1;
}
#endif
