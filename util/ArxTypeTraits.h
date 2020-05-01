#pragma once

#ifndef ARX_TYPE_TRAITS_H
#define ARX_TYPE_TRAITS_H

#include <stddef.h>
#include <type_traits>

namespace arx {

	template<typename T> struct remove_reference                  { typedef T type; };
	template<typename T> struct remove_reference<T&>              { typedef T type; };
	template<typename T> struct remove_reference<T&&>             { typedef T type; };

	template <class T> constexpr T&& forward(typename std::remove_reference<T>::type& t) noexcept{
		return static_cast<T&&>(t);
	}
	template <class T> constexpr T&& forward(typename std::remove_reference<T>::type&& t) noexcept{
		//static_assert(!std::is_lvalue_reference<T>::value, "Can not forward an rvalue as an lvalue.");
		return static_cast<T&&>(t);
	}


    template<typename T, T ...Ts>
    struct integer_sequence
    {
        using type = integer_sequence;
        using value_type = T;
        static constexpr size_t size() noexcept { return sizeof...(Ts); }
    };

    template <std::size_t ...Is>
    using index_sequence = integer_sequence<std::size_t, Is...>;


    // https://stackoverflow.com/questions/17424477/implementation-c14-make-integer-sequence

    template<typename S1, typename S2>
    struct concat_impl;

    template<std::size_t... I1, std::size_t... I2>
    struct concat_impl <index_sequence<I1...>, index_sequence<I2...>>
    : index_sequence<I1..., (sizeof...(I1) + I2)...> {};

    template<typename S1, typename S2>
    using concat = typename concat_impl<S1, S2>::type;

    template<std::size_t N>
    struct make_index_sequence_impl;

    template<std::size_t N>
    using make_index_sequence = typename make_index_sequence_impl<N>::type;

    template<std::size_t N>
    struct make_index_sequence_impl
    : concat<make_index_sequence <N/2>, make_index_sequence <N - N/2>> {};

    template<>
    struct make_index_sequence_impl <0> : index_sequence<>{};
    template<>
    struct make_index_sequence_impl <1> : index_sequence<0>{};


    // https://qiita.com/_EnumHack/items/92e6e135174f1f781dbb
    // without decltype(auto)

    template <typename T>
    using decay_t = typename std::decay<T>::type;

    template <class F, class Tuple, std::size_t... I>
    constexpr auto apply_impl(F&& f, Tuple&& t, index_sequence<I...>)
    -> decltype(f(std::get<I>(std::forward<Tuple>(t))...))
    {
        return f(std::get<I>(std::forward<Tuple>(t))...);
    }

    template <class F, class Tuple>
    constexpr auto apply(F&& f, Tuple&& t)
    -> decltype(apply_impl(
        std::forward<F>(f),
        std::forward<Tuple>(t),
        make_index_sequence<std::tuple_size<decay_t<Tuple>>::value>{}
    ))
    {
        return apply_impl(
            std::forward<F>(f),
            std::forward<Tuple>(t),
            make_index_sequence<std::tuple_size<decay_t<Tuple>>::value>()
        );
    }




template <typename T>
using get_type = typename T::type;

template <std::size_t index, typename ... arguments>
using type_at = get_type<std::tuple_element<index, std::tuple<arguments ...>>>;

namespace detail {
    template <typename T>
    struct remove_const_reference {
        using type = T;
    };
    template <typename T>
    struct remove_const_reference<T &> : remove_const_reference<T> {};
    template <typename T>
    struct remove_const_reference<T &&> : remove_const_reference<T> {};
    template <typename T>
    struct remove_const_reference<const T> : remove_const_reference<T> {};
};

template <typename T>
using remove_const_reference = get_type<detail::remove_const_reference<T>>;


template <typename T>
struct is_callable {
	template <typename U, decltype(&U::operator()) = &U::operator()>
	struct checker {};
	template <typename U> static std::true_type  test(checker<U> *);
	template <typename>   static std::false_type test(...);
	static constexpr bool value = decltype(test<T>(nullptr))::value;
};

template <typename R, typename ... Arguments>
struct is_callable<R(*)(Arguments ...)> {
	static constexpr bool value = true;
};

template <typename R, typename ... Arguments>
struct is_callable<R(*&)(Arguments ...)> {
	static constexpr bool value = true;
};

template <typename R, typename ... Arguments>
struct is_callable<R(&)(Arguments ...)> {
	static constexpr bool value = true;
};

template <typename R, typename ... Arguments>
struct is_callable<R(Arguments ...)> {
	static constexpr bool value = true;
};

template <typename R, typename ... Arguments>
struct is_callable<std::function<R(Arguments ...)>> {
	static constexpr bool value = true;
};

namespace detail {
	template <typename ret, typename ... arguments>
	struct function_traits {
		static constexpr std::size_t arity = sizeof...(arguments);
		using result_type = ret;
		using arguments_types_tuple = std::tuple<arguments ...>;
		template <std::size_t index>
		using argument_type = type_at<index, arguments ...>;
		using function_type = std::function<ret(arguments ...)>;
		template <typename function_t>
		static constexpr function_type cast(function_t f) {
			return static_cast<function_type>(f);
		}
	};
};

template <typename T>
struct function_traits : public function_traits<decltype(&T::operator())> {};

template <typename class_type, typename ret, typename ... arguments>
struct function_traits<ret(class_type::*)(arguments ...) const>
: detail::function_traits<ret, arguments ...> {};

template <typename class_type, typename ret, typename ... arguments>
struct function_traits<ret(class_type::*)(arguments ...)>
: detail::function_traits<ret, arguments ...> {};

template <typename ret, typename ... arguments>
struct function_traits<ret(*)(arguments ...)>
: detail::function_traits<ret, arguments ...> {};

template <typename ret, typename ... arguments>
struct function_traits<ret(*&)(arguments ...)>
: detail::function_traits<ret, arguments ...> {};

template <typename ret, typename ... arguments>
struct function_traits<ret(arguments ...)>
: detail::function_traits<ret, arguments ...> {};

template <typename ret, typename ... arguments>
struct function_traits<std::function<ret(arguments ...)>>
: detail::function_traits<ret, arguments ...> {};

template<typename T>
using result_type = typename function_traits<T>::result_type;

template<typename T>
using arguments_types_tuple = typename function_traits<T>::arguments_types_tuple;

template<typename T, std::size_t index>
using argument_type = typename function_traits<T>::template argument_type<index>;

template<typename T>
struct arity {
	static constexpr std::size_t value = function_traits<T>::arity;
};

template <typename function_t>
constexpr auto cast_lambda(function_t f)
-> typename function_traits<function_t>::function_type {
	return static_cast<typename function_traits<function_t>::function_type>(f);
}

}

#endif // ARX_TYPE_TRAITS_H
