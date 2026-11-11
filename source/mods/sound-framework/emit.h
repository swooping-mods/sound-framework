#ifndef MOON_SOUND_EMIT_H
#define MOON_SOUND_EMIT_H
static int ma_emit(unsigned cue,U64 emitter,const float p[3],float pitch,float gain){
    if(GetCurrentThreadId()!=ma_thread||!ma_ready||ma_paused||ma_cue_index(cue)<0||!emitter||!ma_position(p)||
       !ma_finite(pitch)||pitch<.5f||pitch>2||!ma_finite(gain)||gain<0||gain>1)return 0;
    unsigned index=(unsigned)ma_cue_index(cue);const MoonSoundCue *c=&ma_cues[index];
    int cooldown=ma_cooldown_slot(cue,emitter,ma_clock);if(cooldown<0)return 0;
    unsigned voice=MA_VOICES;
    for(unsigned i=0;i<MA_VOICES;i++)if(!ma_voices[i].active||ma_voices[i].until<=ma_clock){voice=i;break;}
    if(voice==MA_VOICES)return 0; /* Preserve existing transients under load. */
    PTR clip=ma_clip(index);if(!clip||!ma_voice(voice)){ma_event("playback-unavailable",cue,voice);return 0;}
    PTR source=dget(ma_voices[voice].source);int ok=1;call(ma_stop_call,source,0,&ok);
    if(!ok){ma_event("playback-stop-failed",cue,voice);return 0;}
    ma_voices[voice].active=0;void *args[]={clip};call(ma_set_clip,source,args,&ok);
    ok=ma_place(dget(ma_voices[voice].transform),p)&&ok;
    ok=ma_scalar(ma_pitch,source,pitch)&&ma_scalar(ma_volume,source,c->gain*gain*ma_effects_gain)&&
       ma_scalar(ma_min,source,c->min_distance)&&ma_scalar(ma_max,source,c->max_distance)&&
       ma_integer(ma_rolloff,source,index<MA_CUES?0:(int)ma_asset_rolloff[index-MA_CUES])&&ok;
    if(!ok){ma_event("playback-setup-failed",cue,voice);return 0;}
    /* Publish ownership before Play: an exception may occur after the native
     * source starts. A failed rollback stays visible to asset release. */
    ma_voices[voice].emitter=emitter;ma_voices[voice].cue=cue;ma_voices[voice].active=1;
    ma_voices[voice].gain=c->gain*gain;ma_voices[voice].until=ma_clock+(float)c->samples/(float)c->rate/pitch+.02f;
    call(ma_play,source,0,&ok);
    if(!ok){
        int stopped=1;call(ma_stop_call,source,0,&stopped);if(stopped)ma_voices[voice].active=0;
        ma_event("playback-failed",cue,voice);return 0;
    }
    unsigned j=(unsigned)cooldown;ma_limits[j].cue=cue;ma_limits[j].emitter=emitter;ma_limits[j].until=ma_clock+c->cooldown;
    if(ma_test){
        int measured=1;float listener=ma_test_listener_volume?dnumber(ma_test_listener_volume,0,&measured):-1;
        float source_gain=ma_test_source_volume?dnumber(ma_test_source_volume,source,&measured):-1;
        if(measured&&listener>=0&&listener<=1&&source_gain>=0&&source_gain<=1)
            ma_event("mix-source-listener-ppm",(unsigned)(source_gain*1000000),(unsigned)(listener*1000000));
        int playing_ok=1;int playing=boolean(ma_is_playing,source,&playing_ok);
        ma_event(playing_ok&&playing?"cue-played":"cue-not-playing",cue,voice);
    }return 1;
}
static void ma_stop_emitter(U64 emitter){
    if(GetCurrentThreadId()!=ma_thread||!emitter)return;
    for(unsigned i=0;i<MA_VOICES;i++)if(ma_voices[i].active&&ma_voices[i].emitter==emitter){
        int ok=1;call(ma_stop_call,dget(ma_voices[i].source),0,&ok);
        if(ok)ma_voices[i].active=0;else ma_event("emitter-stop-failed",ma_voices[i].cue,i);
    }
}
#endif
