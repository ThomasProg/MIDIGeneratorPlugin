#pragma once

#include "utilities.h"
#include <utility>

// .h only header
// wraps utilities.h
// can be included from outside libraries

class API_EXPORT CppStr
{
protected:
    CStr str;

public:
    void StrReset()
    {
        ::CStrReset(&str);
    }

    const char* Str() const
    {
        return str.str;
    }

    size_t Length() const
    {
        return str.length;
    }

    CppStr()
    {
        ::InitCStr(&str);
    }

    CppStr(const CStr& inStr) = delete;

    // CppStr(const CStr& inStr)
    // {
    //     str = inStr;
    // }

    CppStr(CStr&& inStr)
    {
        str = std::move(inStr);
    }

    CppStr& operator=(const CStr& inStr) = delete;

    CppStr& operator=(CStr&& inStr)
    {
        str = std::move(inStr);
        return *this;
    }

    ~CppStr()
    {
        ::DestroyCStr(&str);
    }
};

#include <string>
inline CStr MakeCStr(const std::string inStr)
{
    return MakeCStr(inStr.c_str());
}
#include <sstream>
inline CStr MakeCStr(const std::ostringstream inStrStream)
{
    return MakeCStr(inStrStream.str());
}

inline std::wstring widen( const std::string& str )
{
    std::wostringstream wstm ;
    const std::ctype<wchar_t>& ctfacet = std::use_facet<std::ctype<wchar_t>>(wstm.getloc()) ;
    for( size_t i=0 ; i<str.size() ; ++i ) 
              wstm << ctfacet.widen( str[i] ) ;
    return wstm.str() ;
}


class API_EXPORT CppResult
{
public:
    CResult result;

    CppResult()
    {
        result.message.str = nullptr;
        result.message.length = 0;
    }

    CppResult(const CResult& inRes) = delete;

    CppResult(CResult&& inRes)
    {
        result = std::move(inRes);
        inRes.message.str = nullptr;
        inRes.message.length = 0;
    }

    CppResult(const char* errorMsg) : result({MakeCStr(errorMsg)})
    {

    }

    CppResult& operator=(CppResult&& inRes)
    {
        result = std::move(inRes.result);
        inRes.result.message.str = nullptr;
        inRes.result.message.length = 0;
        return *this;
    }

    CppResult& operator=(const CppResult& inRes) = delete;


    ~CppResult()
    {
        DestroyCResult(&result);
    }

    CResult Release()
    {
        CResult outRes = result;
        result.message.str = nullptr;
        result.message.length = 0;
        return outRes;
    }

    bool IsSuccess() const
    {
        return ResultIsSuccess(&result);
    }

    const char* GetError() const
    {
        return result.message.str;
    }
};

template<typename T>
class API_EXPORT UniquePtr
{
    T* ptr = nullptr;

public:
    T* operator->()
    {
        return ptr;
    }

    const T* operator->() const
    {
        return ptr;
    }

    T& operator*()
    {
        return *ptr;
    }

    const T& operator*() const
    {
        return *ptr;
    }


    UniquePtr() = default;
    UniquePtr(T* rhs) : ptr(rhs)
    {

    }

    UniquePtr<T>& operator=(const UniquePtr<T>& rhs) = delete;
    UniquePtr<T>& operator=(UniquePtr<T>&& rhs)
    {
        if (ptr != nullptr)
        {
            delete ptr;
        }

        ptr = rhs.ptr;
        rhs.ptr = nullptr;

        return *this;
    }

    ~UniquePtr()
    {
        if (ptr != nullptr)
        {
            delete ptr;
        }
    }
};

template <typename T, typename... Args> 
UniquePtr<T> MakeUnique(Args&&... args)
{
    return UniquePtr<T>(new T(std::forward<Args>(args)...));
}

