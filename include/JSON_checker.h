/* JSON_checker.h */

#pragma once

#include <platform/visibility.h>

#if defined(JSON_checker_EXPORTS)
#define JSON_CHECKER_PUBLIC_API EXPORT_SYMBOL
#else
#define JSON_CHECKER_PUBLIC_API IMPORT_SYMBOL
#endif

#ifdef __cplusplus
extern "C" {
#endif

JSON_CHECKER_PUBLIC_API
int checkUTF8JSON(const unsigned char* data, size_t size);

#ifdef __cplusplus
}
#endif
