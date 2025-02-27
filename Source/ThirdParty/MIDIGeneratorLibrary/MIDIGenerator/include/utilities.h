#pragma once

#include <stdint.h>

struct CStr
{
    size_t length;
    char* str;
};

void InitCStr(CStr* str);
void DestroyCStr(CStr* str);
void CStrReset(CStr* str);

CStr MakeCStr(const char* inStr);

struct CResult
{
    CStr message;
};

inline bool ResultIsSuccess(const CResult* inResult)
{
    return inResult->message.str == nullptr && inResult->message.length == 0;
}

