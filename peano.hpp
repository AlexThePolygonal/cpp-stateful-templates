#pragma once
#include "ctstd_base.hpp"

namespace peano {
    struct IsPeanoInteger {};
    struct Zero : IsPeanoInteger {
        using Prev = ctstd::None;
    };

    template <class T> struct Succ : IsPeanoInteger {
        using Prev = T;
    };

    using One = Succ<Zero>;
    using Two = Succ<One>;
    using Three = Succ<Two>;
    using Four = Succ<Three>;
    using Five = Succ<Four>;
    using Six = Succ<Five>;
    using Seven = Succ<Six>;
    using Eight = Succ<Seven>;
    using Nine = Succ<Eight>;
    using Ten = Succ<Nine>;
    using Eleven = Succ<Ten>;
    using Twelve = Succ<Eleven>;

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

    static_assert(std::is_same_v<Twelve, Integer<12>>);
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

    static_assert(std::is_same_v<add<Two, Two>, Four>);

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

    static_assert(std::is_same_v<mult<Two, Two>, Four>);

    template <class T>
    constexpr unsigned cast = cast<typename T::Prev> + 1;

    template <>
    constexpr unsigned cast<Zero> = 0;

    static_assert(cast<Eleven> == 11);

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

    static_assert(std::is_same_v<minus<Two, Three>, Zero>);
    static_assert(std::is_same_v<minus<Three, Three>, Zero>);
    static_assert(std::is_same_v<minus<Four, Three>, One>);


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

    static_assert(std::is_same_v<leq<Four, Eight>, ctstd::True>);
    static_assert(std::is_same_v<leq<Four, Four>, ctstd::True>);
    static_assert(std::is_same_v<leq<Four, Three>, ctstd::False>);

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

    static_assert(std::is_same_v<div<Three, Two>, One>);
    static_assert(std::is_same_v<remainder<Three, Two>, One>);
    static_assert(std::is_same_v<remainder<Two, Two>, Zero>);
};

