#pragma once

struct Allocator;

template<typename T>
struct DynamicArray
{
    Allocator* allocator;
    T* data;
    unsigned num;
    unsigned capacity;

    void grow()
    {
        T* old_data = data;
        unsigned new_capacity = max(capacity * 2, num + 5);
        data = (T*)allocator->alloc(new_capacity * sizeof(T));
        memcpy(data, old_data, num * sizeof(T));
        allocator->dealloc(old_data);
        capacity = new_capacity;
    }

    void insert(const T&v, unsigned i)
    {
        while (data == nullptr || num == capacity || i + 1 > capacity)
            grow();

        *(data + i) = v;
    }

    void add(const T& v)
    {
        if (data == nullptr || num == capacity)
            grow();

        *(data + num) = v;
        ++num;
    }

    T* push()
    {
        if (data == nullptr || num == capacity)
            grow();

        T* p = data + num;
        ++num;
        return p;
    }

    T* push_init()
    {
        if (data == nullptr || num == capacity)
            grow();

        T* p = data + num;
        memset(p, 0, sizeof(T));
        ++num;
        return p;
    }

    void remove(unsigned i)
    {
        if (num == 1 || (num > 1 && i == num - 1))
        {
            --num;
            return;
        }

        memcpy(data + i, data + num - 1, sizeof(T));
        --num;
    }

    DynamicArray<T> clone(Allocator* new_allocator = nullptr) const
    {
        Allocator* allocator_to_use = new_allocator == nullptr ? allocator : new_allocator;
        DynamicArray<T> c = {};
        c.allocator = allocator_to_use;
        c.data = (T*)allocator_to_use->alloc(num * sizeof(T));
        c.capacity = num;
        c.num = num;
        memcpy(c.data, data, num * sizeof(T));
        return c;
    }

    T* clone_raw(Allocator* alloc = nullptr) const
    {
        T* p = (T*)(alloc == nullptr ? allocator : alloc)->alloc(num * sizeof(T));
        memcpy(p, data, num * sizeof(T));
        return p;
    }

    T& last()
    {
        return data[num - 1];
    }

    T& operator[](unsigned i)
    {
        return data[i];
    }

    const T& operator[](unsigned i) const
    {
        return data[i];
    }
};

template<typename T>
inline DynamicArray<T> dynamic_array_create(Allocator* allocator)
{
    DynamicArray<T> da = {};
    da.allocator = allocator;
    return da;
}

template<typename T>
inline void dynamic_array_destroy(DynamicArray<T>* da)
{
    da->allocator->dealloc(da->data);
}
