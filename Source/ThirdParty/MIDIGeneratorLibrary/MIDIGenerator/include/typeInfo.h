#pragma once

#include "fwd.h"

class Object;
using ObjectHandle = Object*;

typedef struct TypeInfo
{
    // only supports 1 parent
    const TypeInfo* parent;
} API_EXPORT TypeInfo;

API_EXPORT const TypeInfo* getTypeInfo(ObjectHandle obj);

API_EXPORT const bool isParent(const TypeInfo* obj, const TypeInfo* parent);
