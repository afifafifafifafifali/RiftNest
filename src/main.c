#include "version_check/check_qemu.h"
#include "misc/riftprint.h"


int main(void){
    int av_rtn = check_qemuavail();
    if(av_rtn != 0){
        riftprint("CRITICAL: QEMU (x86_64) is not available. Error Code: 0xA0001");
        return 1;

    }
    int ver_rtn = check_qemu_ver();
    if(ver_rtn != 0){
        riftprint("CRITICAL: QEMU (x86_64) does not match minimum version 10.0.0. Error code: 0xA0002");
    }
    
}