#pragma once

#ifndef WIN32_NO_STATUS
#define WIN32_NO_STATUS
#include "phnt_windows.h"
#include "phnt.h"
#undef WIN32_NO_STATUS
#include <ntstatus.h>
#endif

// offsetof()
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

//memory module base support
#include "MemoryModule.h"

//LDR_DATA_TABLE_ENTRY
#include "LdrEntry.h"

//rtl inverted function table for exception handling
#include "InvertedFunctionTable.h"

//base address index
#include "BaseAddressIndex.h"

//tls support
#include "MmpTls.h"

//DotNet support
#include "MmpDotNet.h"

//MemoryModulePP api interface
#include "Loader.h"

//utils
#include "Utils.h"

//global data
#include "MmpGlobalData.h"

#ifdef __cplusplus
}
#endif