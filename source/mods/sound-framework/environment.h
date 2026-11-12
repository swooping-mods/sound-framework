/* Internal adapter seam: borrowed actor/world identities are valid only for this
 * Unity-thread callback. No native object layout is exposed to audio policy. */
#ifndef MOON_SOUND_ENVIRONMENT_H
#define MOON_SOUND_ENVIRONMENT_H
typedef struct {
    PTR world;
    int frame;
    unsigned restoring,has_master;
    float delta,scale,effects,master;
} MaEnvironment;
typedef struct {
    void *context;
    int (*local)(void *,PTR *);
    int (*frame)(void *,int *);
    int (*world)(void *,PTR,PTR *);
    int (*restoring)(void *,PTR,unsigned *);
    int (*time)(void *,float *,float *);
    int (*mix)(void *,float *,float *,unsigned *);
} MaEnvironmentReader;
enum {MA_ENV_REJECTED,MA_ENV_READY,MA_ENV_NO_ACTOR,MA_ENV_IGNORED};
static int ma_environment_valid(const MaEnvironment *s){
    return s&&s->restoring<=1&&s->has_master<=1&&
        ma_finite(s->delta)&&s->delta>=0&&ma_finite(s->scale)&&s->scale>=0&&
        ma_finite(s->effects)&&s->effects>=0&&s->effects<=1&&
        (!s->has_master||(ma_finite(s->master)&&s->master>=0&&s->master<=1));
}
static int ma_environment_capture(const MaEnvironmentReader *r,PTR actor,int last,MaEnvironment *out){
    PTR local=0;MaEnvironment next={0};
    if(!r||!out||!r->local||!r->frame||!r->world||!r->restoring||!r->time||!r->mix)return MA_ENV_REJECTED;
    if(!r->local(r->context,&local))return MA_ENV_REJECTED;
    if(!local)return MA_ENV_NO_ACTOR;
    if(local!=actor)return MA_ENV_IGNORED;
    if(!r->frame(r->context,&next.frame))return MA_ENV_REJECTED;
    if(next.frame==last)return MA_ENV_IGNORED;
    if(!r->world(r->context,actor,&next.world))return MA_ENV_REJECTED;
    if(next.world&&(!r->restoring(r->context,next.world,&next.restoring)||
        !r->time(r->context,&next.delta,&next.scale)||
        !r->mix(r->context,&next.effects,&next.master,&next.has_master)))return MA_ENV_REJECTED;
    /* Publish only a complete valid observation, including successful world exit. */
    if(!ma_environment_valid(&next))return MA_ENV_REJECTED;
    *out=next;return MA_ENV_READY;
}
#endif
