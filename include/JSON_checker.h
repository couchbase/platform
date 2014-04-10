/* JSON_checker.h */

#ifdef JSON_checker_EXPORTS

#if defined (__SUNPRO_C) && (__SUNPRO_C >= 0x550)
#define JSON_CHECKER_PUBLIC_API __global
#elif defined __GNUC__
#define JSON_CHECKER_PUBLIC_API __attribute__ ((visibility("default")))
#elif defined(_MSC_VER)
#define JSON_CHECKER_PUBLIC_API __declspec(dllexport)
#else
/* unknown compiler */
#define JSON_CHECKER_PUBLIC_API
#endif

#else

#if defined(_MSC_VER)
#define JSON_CHECKER_PUBLIC_API __declspec(dllimport)
#else
#define JSON_CHECKER_PUBLIC_API
#endif

#endif

#ifdef __cplusplus
extern "C" {
#endif

JSON_CHECKER_PUBLIC_API
int checkUTF8JSON(const unsigned char* data, size_t size);

#ifdef __cplusplus
}
#endif
