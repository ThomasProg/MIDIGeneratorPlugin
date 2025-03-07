#pragma once

#include <stdint.h>
#include "fwd.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    size_t length;
    char* str;
} API_EXPORT CStr;

API_EXPORT void InitCStr(CStr* str);
API_EXPORT void DestroyCStr(CStr* str);
API_EXPORT void CStrReset(CStr* str);

API_EXPORT CStr MakeCStr(const char* inStr);
API_EXPORT CStr CreateCStr();

typedef struct
{
    CStr message;
} API_EXPORT CResult;

static inline bool ResultIsSuccess(const CResult* inResult)
{
    return inResult->message.str == nullptr && inResult->message.length == 0;
}

static inline void DestroyCResult(CResult* result)
{
    // assert(result != nullptr);
    DestroyCStr(&result->message);
}

#ifdef __cplusplus
}  // End extern "C"
#endif