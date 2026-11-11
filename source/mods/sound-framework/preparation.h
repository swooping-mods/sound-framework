#ifndef MOON_SOUND_PREPARATION_H
#define MOON_SOUND_PREPARATION_H
#include "sdk/preparation.h"
static int ma_prepare(unsigned cue,unsigned voices){
    if(GetCurrentThreadId()!=ma_thread||!ma_ready||voices>MA_VOICES)return 0;
    int index=cue?ma_cue_index(cue):-1;if(cue&&index<0)return 0;
    if(cue&&!ma_clip((unsigned)index))return 0;
    for(unsigned i=0;i<voices;i++)if(!ma_voice(i))return 0;
    return 1;
}
#endif
