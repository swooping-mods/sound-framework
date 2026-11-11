#ifndef MOON_SOUND_ASSETS_RUNTIME_H
#define MOON_SOUND_ASSETS_RUNTIME_H
static unsigned ma_register_asset(const MoonSoundCue *cue,unsigned rolloff){
    if(GetCurrentThreadId()!=ma_thread||!ma_ready)return 0;
    return ma_asset_register(cue,rolloff);
}
static int ma_release_asset(unsigned handle){
    if(GetCurrentThreadId()!=ma_thread)return 0;
    int index=ma_cue_index(handle);if(index<(int)MA_CUES)return 0;
    int ok=1;
    for(unsigned i=0;i<MA_VOICES;i++)if(ma_voices[i].active&&ma_voices[i].cue==handle){
        call(ma_stop_call,dget(ma_voices[i].source),0,&ok);if(!ok)return 0;ma_voices[i].active=0;
    }
    ma_delete(dget(ma_clips[index]));ddrop(&ma_clips[index]);
    return ma_asset_retire(handle);
}
static int ma_playing(U64 emitter){
    if(GetCurrentThreadId()!=ma_thread||!emitter)return 0;
    for(unsigned i=0;i<MA_VOICES;i++)if(ma_voices[i].active&&ma_voices[i].emitter==emitter&&ma_voices[i].until>ma_clock)return 1;
    return 0;
}
#endif
