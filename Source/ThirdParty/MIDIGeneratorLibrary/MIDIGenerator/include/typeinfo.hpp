#pragma once

#include "typeInfo.h"

class API_EXPORT Object
{
public:
    virtual ~Object() = default;

    static const TypeInfo& getStaticTypeInfo();
    virtual const TypeInfo& getTypeInfo();
};

template<typename T>
T* dynamicCast(Object* o)
{
    const TypeInfo& objTypeInfo = o->getTypeInfo();
    const TypeInfo& parentTypeInfo = T::getStaticTypeInfo();

    if (isParent(&objTypeInfo, &parentTypeInfo))
    {
        return static_cast<T*>(o);
    }
    return nullptr;
}