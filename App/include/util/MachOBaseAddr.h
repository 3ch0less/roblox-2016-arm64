//
//  MachOBaseAddr.h
//  App
//
//  Created by David Stahl on 11/10/14.
//
//

#ifndef App_MachOBaseAddr_h
#define App_MachOBaseAddr_h

#include <stdint.h>

uintptr_t machODynamicBaseAddress(void);

uintptr_t machOTextSize(void);

#endif
