#pragma once
// VMProtect stub for non-Windows platforms (macOS ARM64 port)
// All VMProtect macros are no-ops on Mac — the SDK is Windows-only
#ifndef _WIN32

#define VMProtectBeginMutation(x)       ((void)0)
#define VMProtectBeginVirtualization(x) ((void)0)
#define VMProtectBeginUltra(x)          ((void)0)
#define VMProtectBeginVirtualizationLockByKey(x) ((void)0)
#define VMProtectBeginUltraLockByKey(x) ((void)0)
#define VMProtectEnd()                  ((void)0)

#define VMProtectIsProtected()          (0)
#define VMProtectIsDebuggerPresent(x)   (0)
#define VMProtectIsVirtualMachinePresent() (0)
#define VMProtectIsValidImageCRC()      (1)
#define VMProtectDecryptStringA(x)      (x)
#define VMProtectDecryptStringW(x)      (x)
#define VMProtectFreeString(x)          ((void)0)

#else
// Real SDK on Windows
#include <windows.h>
// (actual VMProtect SDK headers would be linked here)
#endif
