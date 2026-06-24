/*
    Minimal AArch64 (ARM64) platform support for TBB 4.1 on Apple Silicon.
    Provides word-size constants and a yield-based pause.
    macos_common.h (included after this by tbb_machine.h) supplies all
    OSAtomic-based CAS/fetch-add ops and __TBB_USE_GENERIC_* fallback flags.
*/

#ifndef __TBB_machine_gcc_armv8_H
#define __TBB_machine_gcc_armv8_H

#include <stdint.h>

#define __TBB_WORDSIZE 8
#define __TBB_BIG_ENDIAN 0

inline void __TBB_machine_pause(int32_t delay)
{
    while (delay > 0) {
        __asm__ __volatile__("yield" ::: "memory");
        delay--;
    }
}

#define __TBB_Pause(V) __TBB_machine_pause(V)

#endif /* __TBB_machine_gcc_armv8_H */
