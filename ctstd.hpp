#pragma once
#include "ctstd_base.hpp"
#include "type_var.hpp"
#include "peano.hpp"

namespace ctstd {
    using _0 = peano::Zero;
    using _1 = peano::Succ<_0>;
    using _2 = peano::Succ<_1>;
    using _3 = peano::Succ<_2>;
    using _4 = peano::Succ<_3>;
    using _5 = peano::Succ<_4>;
    using _6 = peano::Succ<_5>;
    using _7 = peano::Succ<_6>;
    using _8 = peano::Succ<_7>;
    using _9 = peano::Succ<_8>;
    using _10 = peano::Succ<_9>;
    using _11 = peano::Succ<_10>;
    using _12 = peano::Succ<_11>;

    template <unsigned N>
    using Integer = peano::Integer<N>;


    namespace detail {
        template <class T, class U, class TisRV, class UisRV, class _>
        struct AddImpl {
            using value = peano::add<type_var::value<T, _>, type_var::value<U, _>>;
        };

        template <class T, class U, class _>
        struct AddImpl<T, U, ctstd::False, ctstd::True, _> {
            using value = peano::add<type_var::value<T, _>, U>;
        };
        template <class T, class U, class _>
        struct AddImpl<T, U, ctstd::True, ctstd::False, _> {
            using value = peano::add<T, type_var::value<U, _>>;
        };
        template <class T, class U, class _>
        struct AddImpl<T, U, ctstd::True, ctstd::True, _> {
            using value = peano::add<T, U>;
        };
    };

    template <class T, class U, class _>
    using add = typename detail::AddImpl<T, U, peano::is_peano_integer<T>, peano::is_peano_integer<U>, _>::value;

    namespace detail {
        template <class T, class U, class TisRV, class UisRV, class _>
        struct MultImpl {
            using value = peano::mult<type_var::value<T, _>, type_var::value<U, _>>;
        };

        template <class T, class U, class _>
        struct MultImpl<T, U, ctstd::False, ctstd::True, _> {
            using value = peano::mult<type_var::value<T, _>, U>;
        };
        template <class T, class U, class _>
        struct MultImpl<T, U, ctstd::True, ctstd::False, _> {
            using value = peano::mult<T, type_var::value<U, _>>;
        };
        template <class T, class U, class _>
        struct MultImpl<T, U, ctstd::True, ctstd::True, _> {
            using value = peano::mult<T, U>;
        };
    };
    template <class T, class U, class _>
    using mult = typename detail::MultImpl<T, U, peano::is_peano_integer<T>, peano::is_peano_integer<U>, _>::value;

    namespace detail {
        template <class T, class U, class TisRV, class UisRV, class _>
        struct DivImpl {
            using value = peano::div<type_var::value<T, _>, type_var::value<U, _>>;
        };

        template <class T, class U, class _>
        struct DivImpl<T, U, ctstd::False, ctstd::True, _> {
            using value = peano::div<type_var::value<T, _>, U>;
        };
        template <class T, class U, class _>
        struct DivImpl<T, U, ctstd::True, ctstd::False, _> {
            using value = peano::div<T, type_var::value<U, _>>;
        };
        template <class T, class U, class _>
        struct DivImpl<T, U, ctstd::True, ctstd::True, _> {
            using value = peano::div<T, U>;
        };
    };
    template <class T, class U, class _>
    using divide = typename detail::DivImpl<T, U, peano::is_peano_integer<T>, peano::is_peano_integer<U>, _>::value;

    namespace detail {
        template <class T, class U, class TisRV, class UisRV, class _>
        struct RemainderImpl {
            using value = peano::remainder<type_var::value<T, _>, type_var::value<U, _>>;
        };

        template <class T, class U, class _>
        struct RemainderImpl<T, U, ctstd::False, ctstd::True, _> {
            using value = peano::remainder<type_var::value<T, _>, U>;
        };
        template <class T, class U, class _>
        struct RemainderImpl<T, U, ctstd::True, ctstd::False, _> {
            using value = peano::remainder<T, type_var::value<U, _>>;
        };
        template <class T, class U, class _>
        struct RemainderImpl<T, U, ctstd::True, ctstd::True, _> {
            using value = peano::remainder<T, U>;
        };
    };
    template <class T, class U, class _>
    using remainder = typename detail::RemainderImpl<T, U, peano::is_peano_integer<T>, peano::is_peano_integer<U>, _>::value;

    namespace detail {
        template <class T, class U, class TisRV, class UisRV, class _>
        struct MinusImpl {
            using value = peano::minus<type_var::value<T, _>, type_var::value<U, _>>;
        };

        template <class T, class U, class _>
        struct MinusImpl<T, U, ctstd::False, ctstd::True, _> {
            using value = peano::minus<type_var::value<T, _>, U>;
        };
        template <class T, class U, class _>
        struct MinusImpl<T, U, ctstd::True, ctstd::False, _> {
            using value = peano::minus<T, type_var::value<U, _>>;
        };
        template <class T, class U, class _>
        struct MinusImpl<T, U, ctstd::True, ctstd::True, _> {
            using value = peano::minus<T, U>;
        };
    };
    template <class T, class U, class _>
    using minus = typename detail::MinusImpl<T, U, peano::is_peano_integer<T>, peano::is_peano_integer<U>, _>::value;   

    namespace detail {
        template <class T, class U, class TisRV, class UisRV, class _>
        struct LeqImpl {
            using value = peano::leq<type_var::value<T, _>, type_var::value<U, _>>;
        };
        template <class T, class U, class _>
        struct LeqImpl<T, U, ctstd::False, ctstd::True, _> {
            using value = peano::leq<type_var::value<T, _>, U>;
        };
        template <class T, class U, class _>
        struct LeqImpl<T, U, ctstd::True, ctstd::False, _> {
            using value = peano::leq<T, type_var::value<U, _>>;
        };
        template <class T, class U, class _>
        struct LeqImpl<T, U, ctstd::True, ctstd::True, _> {
            using value = peano::leq<T, U>;
        };
    };
    template <class T, class U, class _>
    using leq = typename detail::LeqImpl<T, U, peano::is_peano_integer<T>, peano::is_peano_integer<U>, _>::value;

    namespace detail {
        template <class T, class U, class TisRV, class UisRV, class _>
        struct EqImpl {
            using value = ctstd::is_same<type_var::value<T, _>, type_var::value<U, _>>;
        };
        template <class T, class U, class _>
        struct EqImpl<T, U, ctstd::False, ctstd::True, _> {
            using value = ctstd::is_same<type_var::value<T, _>, U>;
        };
        template <class T, class U, class _>
        struct EqImpl<T, U, ctstd::True, ctstd::False, _> {
            using value = ctstd::is_same<T, type_var::value<U, _>>;
        };
        template <class T, class U, class _>
        struct EqImpl<T, U, ctstd::True, ctstd::True, _> {
            using value = ctstd::is_same<T, U>;
        };
    };
    template <class T, class U, class _>
    using eq = typename detail::EqImpl<T, U, peano::is_peano_integer<T>, peano::is_peano_integer<U>, _>::value;


    //     // Custom logical operations

    // template <class T>
    // using Not = typename detail::NotImpl<T>::value;
    // template <class T, class U>
    // using And = typename detail::AndImpl<T, U>::value;
    // template <class T, class U>
    // using Or = typename detail::OrImpl<T, U>::value;
    // template <class T, class U>
    // using Xor = typename detail::XorImpl<T, U>::value;
};
