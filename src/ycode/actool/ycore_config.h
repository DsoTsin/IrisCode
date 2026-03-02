#pragma once

#ifndef YCORE_API
#ifdef _MSC_VER
#define YCORE_API __declspec(dllimport)
#else
#define YCORE_API
#endif
#endif