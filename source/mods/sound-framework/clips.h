#ifndef MOON_SOUND_CLIPS_H
#define MOON_SOUND_CLIPS_H
static PTR ma_clip(unsigned n){
    if(ma_clips[n])return dget(ma_clips[n]);
    const MoonSoundCue *c=&ma_cues[n];int ok=1,samples=(int)c->samples,rate=(int)c->rate,channels=1,offset=0;unsigned char stream=0;
    void *a[]={da.string(c->key),&samples,&channels,&rate,&stream};PTR clip=call(ma_create,0,a,&ok);
    DRoot root=dhold(clip);if(!ok||!root){ma_delete(clip);ddrop(&root);return 0;}
    PTR array=ma_array(find_class("System","Single"),c->samples);DRoot hold=dhold(array);
    if(!hold){ma_delete(clip);ddrop(&root);return 0;}
    float *dst=(float *)((unsigned char *)dget(hold)+32);
    for(unsigned i=0;i<c->samples;i++)dst[i]=(float)c->pcm[i]/32768.f;
    void *b[]={dget(hold),&offset};PTR result=call(ma_data,dget(root),b,&ok);ddrop(&hold);
    if(!ok||!result||!*(unsigned char *)unbox(result)){ma_delete(dget(root));ddrop(&root);ma_event("clip-upload-failed",n+1,0);return 0;}
    ma_clips[n]=root;ma_event("clip-uploaded",n+1,0);return dget(root);
}
#endif
