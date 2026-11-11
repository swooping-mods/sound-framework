#ifndef MOON_SOUND_ASSETS_REGISTRY_H
#define MOON_SOUND_ASSETS_REGISTRY_H
/* Separate slots leave the immutable startup ABI and cue IDs unchanged. */
static unsigned ma_asset_generations[MA_ASSETS],ma_asset_rolloff[MA_ASSETS];
static char ma_asset_keys[MA_ASSETS][96];
static unsigned ma_asset_handle(unsigned slot){return 0x80000000u|(ma_asset_generations[slot]<<8)|(slot+1);}
static int ma_cue_index(unsigned handle){
    if(handle&&handle<=ma_count&&ma_cues[handle-1].pcm)return (int)handle-1;
    unsigned slot=(handle&255u)-1;
    if(slot<MA_ASSETS&&handle==ma_asset_handle(slot)&&ma_cues[MA_CUES+slot].pcm)return (int)(MA_CUES+slot);
    return -1;
}
static unsigned ma_asset_register(const MoonSoundCue *c,unsigned rolloff){
    if(rolloff>1||!ma_definition(c,8000))return 0;
    for(unsigned i=0;i<MA_TOTAL_CUES;i++)if(ma_cues[i].pcm&&ma_eq(c->key,ma_cues[i].key))return 0;
    for(unsigned i=0;i<MA_ASSETS;i++)if(!ma_cues[MA_CUES+i].pcm&&ma_asset_generations[i]<0x7fffffu){
        unsigned j=0;do{ma_asset_keys[i][j]=c->key[j];}while(c->key[j++]);
        ma_cues[MA_CUES+i]=*c;ma_cues[MA_CUES+i].key=ma_asset_keys[i];
        ma_asset_rolloff[i]=rolloff;ma_asset_generations[i]++;ma_samples+=c->samples;return ma_asset_handle(i);
    }
    return 0;
}
/* The scene-facing caller must stop matching voices and destroy their clip
 * before retiring this definition, so provider PCM can safely be reused. */
static int ma_asset_retire(unsigned handle){
    int index=ma_cue_index(handle);if(index<(int)MA_CUES)return 0;
    ma_samples-=ma_cues[index].samples;ma_cues[index].pcm=0;return 1;
}
#endif
