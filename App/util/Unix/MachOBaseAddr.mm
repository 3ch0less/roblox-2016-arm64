//
//  MachOBaseAddr.m
//  App
//
//  Created on 11/10/14.
//
//

#import <Foundation/Foundation.h>
#include <mach-o/getsect.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <stdint.h>

uintptr_t staticBaseAddress(void)
{
    // getsegbyname returns segment_command_64* on LP64, segment_command* on 32-bit
    const auto* command = getsegbyname("__TEXT");
    return command ? (uintptr_t)command->vmaddr : 0;
}

intptr_t imageSlide(void)
{
    return _dyld_get_image_vmaddr_slide(0);
}

uintptr_t machODynamicBaseAddress(void)
{
    return staticBaseAddress() + (uintptr_t)imageSlide();
}

uintptr_t machOTextSize(void)
{
    const auto* command = getsegbyname("__TEXT");
    return command ? (uintptr_t)command->vmsize : 0;
}
