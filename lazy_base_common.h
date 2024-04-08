//
//  lazy_base_common.h
//

#ifndef __LAZY_BASE_COMMON_H__
#define __LAZY_BASE_COMMON_H__

#include <stdint.h>

#define LAZY_DISALLOW_ASSIGN(TypeName) void operator=(const TypeName&) = delete

#define LAZY_DISALLOW_COPY_AND_ASSIGN(TypeName)     \
    TypeName(const TypeName&) = delete;             \
    LAZY_DISALLOW_ASSIGN(TypeName)


#if defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64)
#ifndef LAZY_API
#ifdef LAZY_EXPORTS
#define LAZY_API __declspec(dllexport)
#else
#define LAZY_API __declspec(dllimport)
#endif
#endif // LAZY_API
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX // Make sure we don't get min/max macros
#define NOMINMAX
#endif // NOMINMAX
#else
// iOS / Android
#define LAZY_API
#endif


#endif /* __LAZY_BASE_COMMON_H__ */
