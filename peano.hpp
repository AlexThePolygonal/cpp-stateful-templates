#pragma once
#include "ctstd_base.hpp"

namespace peano {

    namespace detail { struct IsPeanoInteger {}; };
    struct Zero : detail::IsPeanoInteger {
        using Prev = ctstd::None;
    };
    template <class T>
    using is_peano_integer = ctstd::is_base_of<detail::IsPeanoInteger, T>;


    using _0 = Zero;

    template <class T> struct Succ : detail::IsPeanoInteger {
        using Prev = T;
    };

    using _1 = Succ<Zero>;
    using _2 = Succ<_1>;
    using _3 = Succ<_2>;
    using _4 = Succ<_3>;
    using _5 = Succ<_4>;
    using _6 = Succ<_5>;
    using _7 = Succ<_6>;
    using _8 = Succ<_7>;
    using _9 = Succ<_8>;
    using _10 = Succ<_9>;
    using _11 = Succ<_10>;
    using _12 = Succ<_11>;

    namespace detail {
        template <unsigned N>
        struct IntegerImpl {
            using value = Succ<typename IntegerImpl<N-1>::value>;
        };
        template <>
        struct IntegerImpl<0> {
            using value = Zero;
        };
    };

    template <unsigned N>
    using Integer = typename detail::IntegerImpl<N>::value;

    static_assert(std::is_same_v<_12, Integer<12>>);
    namespace detail {
        template <class A, class B>
        struct Add {
            using value = typename Add<typename A::Prev, Succ<B>>::value;
        };
        template <class B>
        struct Add<Zero, B> {
            using value = B;
        };
    };

    template <class A, class B>
    using add = typename detail::Add<A, B>::value;

    static_assert(std::is_same_v<add<_2, _2>, _4>);

    namespace detail {
        template <class A, class B>
        struct Mult {
            using value = add<typename Mult<typename A::Prev, B>::value, B>;
        };
        template <class B>
        struct Mult<Zero, B> {
            using value = Zero;
        };
    };
    template <class A, class B>
    using mult = typename detail::Mult<A, B>::value;

    static_assert(std::is_same_v<mult<_2, _2>, _4>);

    template <class T>
    constexpr unsigned cast = cast<typename T::Prev> + 1;

    template <>
    constexpr unsigned cast<Zero> = 0;

    static_assert(cast<_11> == 11);

    namespace detail {
        template <class A, class B>
        struct Minus {
            using value = typename Minus<typename A::Prev, typename B::Prev>::value;
        };
        template <class A>
        struct Minus<A, A> {
            using value = Zero;
        };
        template <class A>
        struct Minus<A, Zero> {
            using value = A;
        };
        template <class B>
        struct Minus<Zero, B> {
            using value = Zero;
        };
    };
    
    template <class A, class B>
    using minus = typename detail::Minus<A, B>::value;

    static_assert(std::is_same_v<minus<_2, _3>, _0>);
    static_assert(std::is_same_v<minus<_3, _3>, _0>);
    static_assert(std::is_same_v<minus<_4, _3>, _1>);


    namespace detail {
        template <class A, class B>
        struct Leq {
            using value = typename Leq<A, typename B::Prev>::value;
        };
        template <class A>
        struct Leq<A, A> {
            using value = ctstd::True;
        };
        template <class A>
        struct Leq<A, ctstd::None> {
            using value = ctstd::False;
        };
    };

    template <class A, class B>
    using leq = typename detail::Leq<A, B>::value;

    static_assert(std::is_same_v<leq<_4, _8>, ctstd::True>);
    static_assert(std::is_same_v<leq<_4, _4>, ctstd::True>);
    static_assert(std::is_same_v<leq<_4, _3>, ctstd::False>);

    namespace detail {
        template <class A, class B>
        struct Div {
            using value = typename ctstd::conditional_using<
                std::is_same_v<leq<A, B>, ctstd::True> && !std::is_same_v<A, B>,
                Zero,
                Succ<typename Div<minus<A, B>, B>::value>
            >;
            using remainder = typename ctstd::conditional_using<
                std::is_same_v<leq<A, B>, ctstd::True> && !std::is_same_v<A, B>,
                A,
                typename Div<minus<A, B>, B>::remainder
            >;
        };
        template <class B> 
        struct Div<Zero, B> {
            using value = Zero;
            using remainder = Zero;
        };
    };
    template <class A, class B>
    using div = typename detail::Div<A, B>::value;

    template <class A, class B>
    using remainder = typename detail::Div<A, B>::remainder;

    static_assert(std::is_same_v<div<_3, _2>, _1>);
    static_assert(std::is_same_v<remainder<_3, _2>, _1>);
    static_assert(std::is_same_v<remainder<_2, _2>, _0>);
};

