// #pragma once

// template<typename T>
// class API_EXPORT Vector
// {
// private:
//     T* _begin;
//     T* _end;
//     T* _endCapacity;

// public:
//     Vector()
//     {
//         constexpr size_t baseSize = 32;
//         _begin = new T[baseSize];
//         _end = _begin;
//         _endCapacity = _begin + baseSize;
//     }

//     size_t size() const
//     {
//         return _end - _begin;
//     }

//     T* data()
//     {
//         return _begin;
//     }

//     const T* data() const
//     {
//         return _begin;
//     }

//     T& operator[](size_t index)
//     {
//         return _begin[index];
//     }

//     const T& operator[](size_t index) const
//     {
//         return _begin[index];
//     }

//     // @TODO
//     void reserve(size_t reserveSize)
//     {
//         _begin = new T[reserveSize]
//     }

//     // @TODO
//     class iterator
//     {
//     private:
//         T* _data;

//     public:
//         iterator(T* in) : _data(in)
//         {
            
//         }

//         iterator operator+(size_t i)
//         {
//             return iterator(_data + i);
//         }

//         friend Vector<T>;
//     };

//     using const_iterator = iterator;

//     iterator begin()
//     {
//         return iterator(_begin);
//     }

//     iterator end()
//     {
//         return iterator(_end);
//     }

//     // @TODO
//     void insert(const_iterator pos, const T& value)
//     {
//         if (_end != _endCapacity)
//         {
            
//         }

//         T* it = _end;
//         while (it != pos._data)
//         {
//             *it = std::move(*(it-1));
//             it--;
//         }

//         *it = value;
//     }

//     // @TODO
//     void insert(const_iterator pos, T&& value)
//     {
//         T* it = _end;
//         while (it != pos._data)
//         {
//             *it = std::move(*(it-1));
//             it--;
//         }

//         *it = std::move(value);
//     }

//     // @TODO
//     void erase(const_iterator eraseBegin, const_iterator eraseEnd)
//     {
//         T* copySrc = eraseEnd._data;
//         T* copyDst = eraseBegin._data;

//         while (copyDst != eraseEnd)
//         {

//         }

//         // end -= 
//     }
// };