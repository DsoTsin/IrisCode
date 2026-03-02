#ifndef __IRISBUILD_COMPILER_SUPPORT__
#define __IRISBUILD_COMPILER_SUPPORT__
#pragma once

#ifdef _MSC_VER
# define IB_PACKED(d) __pragma(pack(push, 1)) d __pragma(pack(pop))
# define IB_PACKED_START __pragma(pack(push, 1))
# define IB_PACKED_END   __pragma(pack(pop))
#else
# define IB_PACKED(d) d __attribute__((packed))
# define IB_PACKED_START _Pragma("pack(push, 1)")
# define IB_PACKED_END   _Pragma("pack(pop)")
#endif

#endif