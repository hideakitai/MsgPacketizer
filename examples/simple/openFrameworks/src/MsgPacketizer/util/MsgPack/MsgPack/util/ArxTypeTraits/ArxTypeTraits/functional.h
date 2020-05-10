#pragma once

#ifndef ARX_TYPE_TRAITS_FUNCTIONAL_H
#define ARX_TYPE_TRAITS_FUNCTIONAL_H

#include <Arduino.h>
#include <new.h>

#ifdef ARX_TYPE_TRAITS_DISABLED

namespace std {

    // functional-avr
    // https://github.com/winterscar/functional-avr

    template<size_t size, size_t align>
    struct alignas(align) aligned_storage_t
    {
        char buff[size];
    };

    template<class Sig, size_t sz, size_t algn>
    struct small_task;

    template<class R, class...Args, size_t sz, size_t algn>
    struct small_task<R(Args...), sz, algn>
    {
        struct vtable_t
        {
            void(*mover)(void* src, void* dest);
            void(*destroyer)(void*);
            R(*invoke)(void const* t, Args&&...args);
            template<class T>
            static vtable_t const* get()
            {
                static const vtable_t table =
                {
                    [](void* src, void*dest) {
                        new(dest) T(move(*static_cast<T*>(src)));
                    },
                    [](void* t){
                        static_cast<T*>(t)->~T();
                    },
                    [](void const* t, Args&&...args)->R {
                        return (*static_cast<T const*>(t))(forward<Args>(args)...);
                    }
                };
                return &table;
            }
        };
        vtable_t const* table = nullptr;
        aligned_storage_t<sz, algn> data;

        template<class F,
            class dF = typename decay<F>::type,
            typename enable_if<!is_same<dF, small_task>{}>::type* = nullptr,
            typename enable_if<is_convertible<typename result_of<dF&(Args...)>::type, R >{}>::type* = nullptr
        >
        small_task(F&& f)
        : table(vtable_t::template get<dF>())
        {
            static_assert( sizeof(dF) <= sz, "object too large" );
            static_assert( alignof(dF) <= algn, "object too aligned" );
            new(&data) dF(forward<F>(f));
        }
        small_task(const small_task& o)
        : table(o.table)
        {
            data = o.data;
        }
        small_task(small_task&& o)
        : table(o.table)
        {
            if (table) table->mover(&o.data, &data);
        }
        small_task(){}

        ~small_task()
        {
            if (table) table->destroyer(&data);
        }

        small_task& operator=(const small_task& o)
        {
            this->~small_task();
            new(this) small_task( move(o) );
            return *this;
        }
        small_task& operator=(small_task&& o)
        {
            this->~small_task();
            new(this) small_task( move(o) );
            return *this;
        }
        small_task& operator=(nullptr_t p)
        {
            (void)p;
            this->~small_task();
            return *this;
        }
        explicit operator bool() const { return table; }
        R operator()(Args...args) const
        {
            return table->invoke(&data, forward<Args>(args)...);
        }
    };

    template<class R, class... Args, size_t sz, size_t algn>
    inline bool operator==(const small_task<R(Args...), sz, algn>& __f, nullptr_t)
    { return !static_cast<bool>(__f); }

    /// @overload
    template<class R, class...Args, size_t sz, size_t algn>
    inline bool  operator==(nullptr_t, const small_task<R(Args...), sz, algn>& __f)
    { return !static_cast<bool>(__f); }

    template<class R, class...Args, size_t sz, size_t algn>
    inline bool operator!=(const small_task<R(Args...), sz, algn>& __f, nullptr_t)
    { return static_cast<bool>(__f); }

    /// @overload
    template<class R, class...Args, size_t sz, size_t algn>
    inline bool operator!=(nullptr_t, const small_task<R(Args...), sz, algn>& __f)
    { return static_cast<bool>(__f); }

    template<class Sig>
    using function = small_task<Sig, sizeof(void*)*4, alignof(void*) >;

} // namespace std

#endif // ARX_TYPE_TRAITS_DISABLED
#endif // ARX_TYPE_TRAITS_FUNCTIONAL_H
