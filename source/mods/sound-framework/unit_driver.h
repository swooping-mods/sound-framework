#ifndef MOON_SOUND_UNIT_DRIVER_H
#define MOON_SOUND_UNIT_DRIVER_H
#include "../../runtime/native/update_driver.h"
static int ma_unit_parent,ma_unit_consumer;
static void ma_unit_update(PTR walker){
    if(!__atomic_load_n(&ma_started,__ATOMIC_ACQUIRE))return;
    Context context={0};context.rcx=(U64)walker;ma_before(0,&context);
}
__declspec(dllexport) int MoonLab_AttachUnitFrame_v1(unsigned abi,MoonUpdateFrame callback){
    if(abi!=MOON_UPDATE_DRIVER_ABI||!callback||!__atomic_load_n(&ma_started,__ATOMIC_ACQUIRE)||
       GetCurrentThreadId()!=ma_registration_thread||ma_unit_parent||ma_unit_consumer||!ma_on_update(callback))return 0;
    ma_unit_consumer=1;return 1;
}
static int ma_attach_unit_driver(void){
    if(ma_unit_parent||ma_unit_consumer)return 0;
    if(!moon_update_attach("unit-runtime",MOON_ATTACH_SOUND_FRAME,ma_unit_update))return 0;
    ma_unit_parent=1;return 1;
}
#endif
