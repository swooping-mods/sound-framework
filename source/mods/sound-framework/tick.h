#ifndef MOON_SOUND_TICK_H
#define MOON_SOUND_TICK_H
static void ma_tick(const MaEnvironment *state){
    /* Injection callers receive the same fail-closed validation as capture. */
    if(!ma_environment_valid(state))return;
    if(!ma_world_enter_observed(state->world,state->restoring))return;
    unsigned paused=state->scale<=0;
    if(paused!=ma_paused){
        int success=1;
        for(unsigned i=0;i<MA_VOICES;i++)if(ma_voices[i].active){
            int ok=1;call(paused?ma_pause:ma_unpause,dget(ma_voices[i].source),0,&ok);if(!ok)success=0;
        }
        /* Pause/UnPause are idempotent. Retry all voices on the next observation
         * after partial failure; do not advance the audio clock prematurely. */
        if(!success)return;
        ma_paused=paused;ma_event(paused?"audio-paused":"audio-resumed",0,0);
    }
    if(!paused&&state->delta>0&&state->delta<1)ma_clock+=state->delta;
    ma_effects_gain=state->effects;
    if(state->has_master&&ma_test_master){ma_event("mix-settings-ppm",(unsigned)(state->effects*1000000),(unsigned)(state->master*1000000));ma_test_master=0;}
    for(unsigned i=0;i<MA_VOICES;i++)if(ma_voices[i].active){
        if(ma_voices[i].until<=ma_clock){ma_voices[i].active=0;continue;}
        ma_scalar(ma_volume,dget(ma_voices[i].source),ma_voices[i].gain*ma_effects_gain);
    }
}
#endif
