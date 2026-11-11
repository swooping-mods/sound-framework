#ifndef MOON_SOUND_REGISTRY_H
#define MOON_SOUND_REGISTRY_H
#include "sdk/sound.h"
#include "sdk/assets.h"
#define MA_SAMPLE_LIMIT 3000000u
static MoonSoundCue ma_cues[MA_TOTAL_CUES];
static unsigned ma_count,ma_sealed,ma_samples;
static void (*ma_updates[32])(void *);static unsigned ma_update_count;
static int ma_on_update(void (*callback)(void *)){
    unsigned count=__atomic_load_n(&ma_update_count,__ATOMIC_RELAXED);
    if(!callback||ma_sealed||count==32)return 0;
    for(unsigned i=0;i<count;i++)if(ma_updates[i]==callback)return 0;
    ma_updates[count]=callback;__atomic_store_n(&ma_update_count,count+1,__ATOMIC_RELEASE);return 1;
}
static inline void ma_dispatch_updates(void *walker){
    unsigned count=__atomic_load_n(&ma_update_count,__ATOMIC_ACQUIRE);
    for(unsigned i=0;i<count;i++)ma_updates[i](walker);
}
static struct {unsigned long long emitter;unsigned cue;float until;} ma_limits[128];
/* Preserve every live cooldown entry. Saturation rejects rather than evicting
 * an unexpired emitter and letting it bypass its declared cooldown. */
static int ma_cooldown_slot(unsigned cue,unsigned long long emitter,float clock){
    int available=-1;
    for(unsigned i=0;i<128;i++){
        if(ma_limits[i].cue==cue&&ma_limits[i].emitter==emitter&&ma_limits[i].until>clock)return -1;
        if(available<0&&ma_limits[i].until<=clock)available=(int)i;
    }
    return available;
}
static int ma_finite(float x){union {float f;unsigned u;}v={x};return (v.u&0x7f800000u)!=0x7f800000u;}
static int ma_key(const char *s){
    if(!s)return 0;unsigned n=0,dots=0;
    while(*s&&n<96){char c=*s++;if(c=='.')dots++;else if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='-'||c=='_'))return 0;n++;}
    return n>2&&n<96&&!*s&&dots;
}
static int ma_eq(const char *a,const char *b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
static int ma_definition(const MoonSoundCue *c,unsigned min_rate){
    if(!c||c->abi!=MA_ABI||c->size!=sizeof(*c)||!ma_key(c->key)||!c->pcm||
       c->rate<min_rate||c->rate>96000||!c->samples||c->samples>c->rate*4||ma_samples+c->samples>MA_SAMPLE_LIMIT||
       !ma_finite(c->gain)||c->gain<0||c->gain>1||!ma_finite(c->min_distance)||c->min_distance<.1f||
       !ma_finite(c->max_distance)||c->max_distance<=c->min_distance||c->max_distance>100||
       !ma_finite(c->cooldown)||c->cooldown<0||c->cooldown>10)return 0;
    return 1;
}
static unsigned ma_register(const MoonSoundCue *c){
    if(ma_sealed||ma_count==MA_CUES||!ma_definition(c,22050))return 0;
    for(unsigned i=0;i<MA_TOTAL_CUES;i++)if(ma_cues[i].pcm&&ma_eq(c->key,ma_cues[i].key))return 0;
    ma_cues[ma_count]=*c;ma_samples+=c->samples;return ++ma_count;
}
static int ma_position(const float *p){
    if(!p)return 0;
    for(unsigned i=0;i<3;i++)if(!ma_finite(p[i])||p[i]<-10000000||p[i]>10000000)return 0;
    return 1;
}
#endif
