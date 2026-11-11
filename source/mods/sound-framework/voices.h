#ifndef MOON_SOUND_VOICES_H
#define MOON_SOUND_VOICES_H
static int ma_voice(unsigned i){
    if(ma_voices[i].source)return 1;
    int ok=1;PTR go=da.new_object(find_class("UnityEngine","GameObject"));DRoot root=dhold(go),source_root=0,tr_root=0;
    if(!root){ma_delete(go);return 0;}
    void *a[]={da.string("MoonLab spatial SFX voice")};call(ma_go_ctor,go,a,&ok);
    void *t[]={dtype("UnityEngine","AudioSource")};PTR source=call(ma_add,go,t,&ok),tr=call(ma_transform,go,0,&ok);
    source_root=dhold(source);tr_root=dhold(tr);
    if(!ok||!source_root||!tr_root||!ma_boolean(ma_awake,source,0)||!ma_scalar(ma_spatial,source,1)||!ma_scalar(ma_doppler,source,0)||!ma_integer(ma_rolloff,source,0)){
        ma_delete(dget(root));ddrop(&source_root);ddrop(&tr_root);ddrop(&root);return 0;
    }
    ma_voices[i].go=root;ma_voices[i].source=source_root;ma_voices[i].transform=tr_root;return 1;
}
#endif
