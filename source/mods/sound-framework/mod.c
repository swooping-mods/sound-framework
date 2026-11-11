/* Shared native audio service; the loader's existing Update dispatcher owns
 * transport. Registry work is inert at startup, Unity work is main-thread only. */
#include "../../runtime/native/gameplay.h"
#include "../../runtime/native/local_actor.h"
#include "../../runtime/native/services.h"
#include "reflection.h"
#include "registry.h"
#include "assets_registry.h"
int _fltused=0;
static PTR ma_frame;static int ma_last_frame=-1;
static PTR ma_log,ma_world_field,ma_restore,ma_alive,ma_destroy,ma_delta,ma_scale,ma_settings,ma_effects;
static PTR ma_create,ma_data,ma_go_ctor,ma_add,ma_transform,ma_position_set;
static PTR ma_play,ma_is_playing,ma_stop_call,ma_pause,ma_unpause,ma_set_clip,ma_volume,ma_pitch;
static PTR ma_awake,ma_spatial,ma_doppler,ma_min,ma_max,ma_rolloff;
static PTR (*ma_array)(PTR,U64);
static DRoot ma_world,ma_clips[MA_TOTAL_CUES];
static struct {DRoot go,source,transform;U64 emitter;unsigned cue,active;float until,gain;} ma_voices[MA_VOICES];
static DWORD ma_thread,ma_registration_thread;static unsigned ma_started,ma_ready,ma_paused,ma_test;
static float ma_clock,ma_effects_gain=1;
static PTR ma_test_listener_volume,ma_test_source_volume,ma_test_master;
static void ma_event(const char *event,unsigned cue,unsigned voice){
    if(!ma_log)return;char b[256],*p=cat(b,"{\"event\":\"");p=cat(p,event);p=cat(p,"\",\"cue\":");p=num(p,cue);p=cat(p,",\"voice\":");p=num(p,voice);p=cat(p,"}\n");DWORD n=0;WriteFile(ma_log,b,(DWORD)(p-b),&n,0);
}
static int ma_exists(PTR p){int ok=1;void *a[]={p};PTR r=p?call(ma_alive,0,a,&ok):0;return ok&&r&&*(unsigned char *)unbox(r);}
static void ma_delete(PTR p){if(!p)return;int ok=1;void *a[]={p};call(ma_destroy,0,a,&ok);}
static int ma_scalar(PTR m,PTR self,float value){int ok=1;void *a[]={&value};call(m,self,a,&ok);return ok;}
static int ma_integer(PTR m,PTR self,int value){int ok=1;void *a[]={&value};call(m,self,a,&ok);return ok;}
static int ma_boolean(PTR m,PTR self,unsigned char value){int ok=1;void *a[]={&value};call(m,self,a,&ok);return ok;}
static int ma_place(PTR tr,const float *p){int ok=1;void *a[]={(void *)p};call(ma_position_set,tr,a,&ok);return ok;}
static void ma_end(void){
    for(unsigned i=0;i<MA_VOICES;i++){
        ma_delete(dget(ma_voices[i].go));ddrop(&ma_voices[i].source);ddrop(&ma_voices[i].transform);ddrop(&ma_voices[i].go);
    }
    for(unsigned i=0;i<MA_TOTAL_CUES;i++){ma_delete(dget(ma_clips[i]));ddrop(&ma_clips[i]);}
    ddrop(&ma_world);dzero(ma_voices,sizeof(ma_voices));dzero(ma_limits,sizeof(ma_limits));
    ma_clock=0;ma_ready=0;ma_paused=0;ma_event("world-audio-reset",0,0);
}
#include "clips.h"
#include "voices.h"
#include "emit.h"
#include "preparation.h"
static void ma_move(U64 emitter,const float p[3]){
    if(GetCurrentThreadId()!=ma_thread||!ma_ready||!emitter||!ma_position(p))return;
    for(unsigned i=0;i<MA_VOICES;i++)if(ma_voices[i].active&&ma_voices[i].emitter==emitter)ma_place(dget(ma_voices[i].transform),p);
}
#include "assets_runtime.h"
static const MoonSoundAssets ma_assets_api={MA_ASSET_ABI,sizeof(MoonSoundAssets),ma_register_asset,ma_release_asset,ma_playing};
__declspec(dllexport) const MoonSoundAssets *MoonLab_SoundAssets_v1(unsigned abi,unsigned size){
    return abi==MA_ASSET_ABI&&size==sizeof(MoonSoundAssets)&&__atomic_load_n(&ma_started,__ATOMIC_ACQUIRE)?&ma_assets_api:0;
}
static const MoonSoundPreparation ma_preparation_api={MA_PREPARATION_ABI,sizeof(MoonSoundPreparation),ma_prepare};
__declspec(dllexport) const MoonSoundPreparation *MoonLab_SoundPreparation_v1(unsigned abi,unsigned size){
    return abi==MA_PREPARATION_ABI&&size==sizeof(MoonSoundPreparation)&&__atomic_load_n(&ma_started,__ATOMIC_ACQUIRE)?&ma_preparation_api:0;
}
static const MoonSounds ma_api={MA_ABI,sizeof(MoonSounds),ma_register,ma_emit,ma_move,ma_stop_emitter,ma_on_update};
__declspec(dllexport) const MoonSounds *MoonLab_Sounds_v1(unsigned abi,unsigned size){return abi==MA_ABI&&size==sizeof(MoonSounds)&&__atomic_load_n(&ma_started,__ATOMIC_ACQUIRE)?&ma_api:0;}
#include "world_state.h"
static void ma_tick(PTR walker){
    PTR world=0;field_value(walker,ma_world_field,&world);
    if(!ma_world_enter(world))return;int ok=1;
    float dt=dnumber(ma_delta,0,&ok),scale=dnumber(ma_scale,0,&ok);if(!ok)return;
    unsigned paused=scale<=0;
    if(paused!=ma_paused){for(unsigned i=0;i<MA_VOICES;i++)if(ma_voices[i].active)call(paused?ma_pause:ma_unpause,dget(ma_voices[i].source),0,&ok);ma_paused=paused;ma_event(paused?"audio-paused":"audio-resumed",0,0);}
    if(!paused&&dt>0&&dt<1)ma_clock+=dt;
    PTR settings=call(ma_settings,0,0,&ok);float effects=0;if(settings)field_value(settings,ma_effects,&effects);
    if(ok&&settings&&ma_finite(effects)&&effects>=0&&effects<=1)ma_effects_gain=effects;
    if(ma_test&&settings&&ma_test_master){
        /* Read-only capture diagnostics: never override the player's mix. */
        float master=0;field_value(settings,ma_test_master,&master);
        if(ma_finite(master)&&master>=0&&master<=1&&ma_finite(effects)&&effects>=0&&effects<=1){ma_event("mix-settings-ppm",(unsigned)(effects*1000000),(unsigned)(master*1000000));ma_test_master=0;}
    }
    for(unsigned i=0;i<MA_VOICES;i++)if(ma_voices[i].active){
        if(ma_voices[i].until<=ma_clock){ma_voices[i].active=0;continue;}
        ma_scalar(ma_volume,dget(ma_voices[i].source),ma_voices[i].gain*ma_effects_gain);
    }
    return;
}
static int ma_before(int id,Context *c){
    (void)id;if(!__atomic_load_n(&ma_started,__ATOMIC_ACQUIRE))return 0;
    if(!ma_thread)ma_thread=GetCurrentThreadId();if(ma_thread!=GetCurrentThreadId())return 0;
    PTR local=0;if(!moon_local_actor_read(&local))return 0;
    if(!local){if(ma_world)ma_end();return 0;}
    if(local!=(PTR)c->rcx)return 0;
    int ok=1;int frame=integer(ma_frame,0,&ok);if(!ok||frame==ma_last_frame)return 0;ma_last_frame=frame;
    ma_tick((PTR)c->rcx);
    ma_dispatch_updates((PTR)c->rcx);
    return 0;
}
#include "unit_driver.h"
static void ma_frame_update(void *walker,void *user){(void)user;Context context={0};context.rcx=(U64)walker;ma_before(0,&context);}
__declspec(dllexport) int MoonLab_ModStart(PTR domain,PTR game){
    if(__atomic_load_n(&ma_started,__ATOMIC_ACQUIRE))return 1;
    ma_registration_thread=GetCurrentThreadId();
    char folder[1000],path[1100];DWORD n=GetEnvironmentVariableA("MOONLAB_EVIDENCE",folder,1000);if(!n||n>=1000)return 0;
    cat(cat(path,folder),"\\sound-framework.jsonl");ma_log=CreateFileA(path,0x40000000,1,0,1,0x80,0);
    if(ma_log==(PTR)(U64)-1)return 0;
    if(!bind_runtime(domain,game)||!dapi(game)||!moon_local_actor_bind())return 0;
#define MA(F,NS,K,N,S) F=dmethod(NS,K,N,S);if(!F){ma_event("missing-" K "-" N,0,0);return 0;}
    MA(ma_restore,"MoonCleaners","MoonWorld","get_IsRestoringRendering","");
    MA(ma_settings,"MoonCleaners","MoonUserSettings","get_Current","");
    MA(ma_alive,"UnityEngine","Object","op_Implicit","UnityEngine.Object");MA(ma_destroy,"UnityEngine","Object","Destroy","UnityEngine.Object");
    MA(ma_frame,"UnityEngine","Time","get_frameCount","");
    MA(ma_delta,"UnityEngine","Time","get_deltaTime","");MA(ma_scale,"UnityEngine","Time","get_timeScale","");
    MA(ma_create,"UnityEngine","AudioClip","Create","System.String;System.Int32;System.Int32;System.Int32;System.Boolean");
    MA(ma_data,"UnityEngine","AudioClip","SetData","System.Single[];System.Int32");
    MA(ma_go_ctor,"UnityEngine","GameObject",".ctor","System.String");MA(ma_add,"UnityEngine","GameObject","AddComponent","System.Type");
    MA(ma_transform,"UnityEngine","GameObject","get_transform","");MA(ma_position_set,"UnityEngine","Transform","set_position","UnityEngine.Vector3");
    MA(ma_is_playing,"UnityEngine","AudioSource","get_isPlaying","");
    MA(ma_play,"UnityEngine","AudioSource","Play","");MA(ma_stop_call,"UnityEngine","AudioSource","Stop","");
    MA(ma_pause,"UnityEngine","AudioSource","Pause","");MA(ma_unpause,"UnityEngine","AudioSource","UnPause","");
    MA(ma_set_clip,"UnityEngine","AudioSource","set_clip","UnityEngine.AudioClip");
    MA(ma_volume,"UnityEngine","AudioSource","set_volume","System.Single");MA(ma_pitch,"UnityEngine","AudioSource","set_pitch","System.Single");
    MA(ma_awake,"UnityEngine","AudioSource","set_playOnAwake","System.Boolean");
    MA(ma_spatial,"UnityEngine","AudioSource","set_spatialBlend","System.Single");MA(ma_doppler,"UnityEngine","AudioSource","set_dopplerLevel","System.Single");
    MA(ma_min,"UnityEngine","AudioSource","set_minDistance","System.Single");MA(ma_max,"UnityEngine","AudioSource","set_maxDistance","System.Single");
    MA(ma_rolloff,"UnityEngine","AudioSource","set_rolloffMode","UnityEngine.AudioRolloffMode");
#undef MA
    ma_world_field=get_field(find_class("MoonCleaners","MoonWalker"),"world");
    ma_effects=get_field(find_class("MoonCleaners","MoonUserSettings"),"effectsVolume");
    ma_array=(PTR(*)(PTR,U64))GetProcAddress(game,"il2cpp_array_new");
    if(!ma_world_field||!ma_effects||!ma_array)return 0;
    char test[8];n=GetEnvironmentVariableA("MOONLAB_TEST_TOOLS",test,sizeof(test));ma_test=n==1&&test[0]=='1';
    if(ma_test){
        ma_test_listener_volume=dmethod("UnityEngine","AudioListener","get_volume","");
        ma_test_source_volume=dmethod("UnityEngine","AudioSource","get_volume","");
        ma_test_master=get_field(find_class("MoonCleaners","MoonUserSettings"),"masterVolume");
    }
    if(moon_runtime_loader_present()){
        if(!moon_runtime_subscribe_frame(ma_frame_update,0))return 0;
    }else if(ma_attach_unit_driver())ma_event("unit-update-driver",0,0);
    else {
        if(!prepare_hook(method("MoonCleaners","MoonWalker","Update",0),100671733,0,game))return 0;
        before_hook=ma_before;if(!install_hooks())return 0;
    }
    if(moon_runtime_loader_present()){
        const MoonRuntimeApiV1 *runtime=moon_runtime_connect();
        const MoonServiceLimitV1 sound_limits[]={{"max_cues",MA_CUES},{"max_voices",MA_VOICES}};
        const MoonServiceLimitV1 asset_limits[]={{"max_assets",MA_ASSETS},{"max_samples",MA_SAMPLE_LIMIT}};
        MoonServicePublicationLimitsV1 sounds={.base={.size=sizeof(sounds),.service_id="sound",.abi=MA_ABI,.api_size=sizeof(ma_api),.api=&ma_api},.limit_count=sizeof(sound_limits)/sizeof(sound_limits[0]),.reserved=0,.limits=sound_limits};
        MoonServicePublicationLimitsV1 assets={.base={.size=sizeof(assets),.service_id="sound-assets",.abi=MA_ASSET_ABI,.api_size=sizeof(ma_assets_api),.api=&ma_assets_api},.limit_count=sizeof(asset_limits)/sizeof(asset_limits[0]),.reserved=0,.limits=asset_limits};
        MoonServicePublicationV1 preparation={.size=sizeof(preparation),.service_id="sound-preparation",.abi=MA_PREPARATION_ABI,.api_size=sizeof(ma_preparation_api),.api=&ma_preparation_api};
        if(!runtime||!runtime->publish(&sounds.base)||!runtime->publish(&assets.base)||!runtime->publish(&preparation))return 0;
    }
    __atomic_store_n(&ma_started,1,__ATOMIC_RELEASE);ma_event("ready",0,0);return 1;
}
