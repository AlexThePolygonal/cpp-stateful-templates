#pragma once
#include "ctstd.hpp"

/***
 *      ______ _    _ _   _   ______      _____ _______ _____ 
 *     |  ____| |  | | \ | | |  ____/\   / ____|__   __/ ____|
 *     | |__  | |  | |  \| | | |__ /  \ | |       | | | (___  
 *     |  __| | |  | | . ` | |  __/ /\ \| |       | |  \___ \ 
 *     | |    | |__| | |\  | | | / ____ \ |____   | |  ____) |
 *     |_|     \____/|_| \_| |_|/_/    \_\_____|  |_| |_____/ 
 *                                                            
 *  
 * This project turned out to be VERY FUN
 * Some of the code parts are commented out to serve as examples
 * They have comments describing INTERESTING PITFALLS I encountered
 * C++ is truly an amazing language!
 * 
 *                                                           
 */

/// GENERAL WARNINGS :
/// std=c++20 REQUIRED
static_assert(__cplusplus >= 202002L);
static_assert(!std::is_same_v<decltype([](){}), decltype([](){})>, "decltype([](){}) isn't a unique identifier, the library won't work");


/// expect frequent random crashes
/// Do not forget Wno-non-template-friend

namespace type_var {

    namespace detail {
        // A trivially-constructible container for T
        // Used to force resolution of auto functions by `return Container<T>{}`
        struct IsContainer {};
        template <class T>
        struct Containter : public IsContainer {
            using value = T;
        };
        template <class T> class Addr {};

        // Declares friend functions comprising the global state
        template <class, int>
        struct Flag {
            friend constexpr auto flag(Flag); // the return type of flag(Flag) encodes the data
        };

        // Injects the definitions of friend functions comprising the global state
        template <
            class T, int N, class Val
        >
        struct Writer {
            friend constexpr auto flag(Flag<T, N>) {
                return ctstd::naive_declval<Containter<Val>>(); // The return type is the value of T
            }
            constexpr static int value = N;
        };

        // if the flag(Flag<T, N>) doesn't exist yet, create it, storing Val in it 
        template <
            class T, class Val, 
            int N = 0, class _ = decltype([](){})
        >
        constexpr int assign_reader(float) {
            return Writer<T, N, Val>::value;
        }
        // If flag(Flag<T, N>) exists, pass to N+1
        template <
            class T, class Val, 
            int N = 0,
            class _ = decltype([](){}),
            class = decltype(flag(Flag<T, N>{}))
        >
        constexpr int assign_reader(int) {
            return assign_reader<T, Val, N+1, _>(int{});
        }

        // If the flag(Flag<T, N>) doesn't exist, return None
        template <class, int N = 0, class _ = decltype([](){})>
        constexpr auto load_reader(float) {
            return detail::Containter<ctstd::None>{};
        }
        // If the this is the last N with well-formed flag, return the value encoded in it, otherwise go to N+1
        template<
            class T, int N = 0,
            class _ = decltype([](){}),
            class U = decltype(flag(Flag<T, N>{}))
        >
        constexpr auto load_reader(int) {
            using Res = decltype(load_reader<T, N+1, _>(int{}));
            return ctstd::conditional_using<std::is_same_v<Res, detail::Containter<ctstd::None>>, U, Res>{};
        }
    };
    
    // Assign the value Val to the type T
    template<
        class T, class Val,
        class _ = decltype([](){}), 
        int = detail::assign_reader<detail::Addr<T>, Val, 0, _>(int{})
    >
    struct Assign {};

    template <class T> struct Assignment {
        template <class U, class _ = decltype([](){})> struct call : Assign<T, U, _> {};
    };



    // Get the value assigned to T
    template <
        class T, class _ = decltype([](){}),
        class R = typename decltype(detail::load_reader<detail::Addr<T>, 0, _>(int{}))::value
    >
    using value = R;
};

namespace cexpr_control {

    template <class T>
    struct Delayed {
        template <class ... Args>
        struct call : T::template call<Args...> {};
    };


    namespace detail {
        template <
            class v, 
            class Func, class Args, class Brgs, class _
            >
        struct IfElseImpl {};

        template <class Func, class Args, class Brgs, class _>
        struct IfElseImpl<ctstd::True, Func, Args, Brgs, _> {
            using Res = typename Func:: template call<Args, _>;
        };
        template <class Func, class Args, class Brgs,class _>
        struct IfElseImpl<ctstd::False, Func, Args, Brgs, _> {
            using Res = typename Func:: template call<Brgs, _>;
        };

        template <
            class v, 
            class Func, class ... Args
            >
        struct IfImpl {};

        template <class Func, class ... Args>
        struct IfImpl<ctstd::True, Func, Args...> {
            using Res = typename Func:: template call<Args...>;
        };

        template <class T, class U>
        struct Pair {};

        template <unsigned N>
        struct WrapInt {};
    };
    // If v is true, call Func(ArgsIfTrue)
    // If v is false, call Fun(ArgsIfTrue)
    template <class v, class Func, class ArgsIfTrue, class ArgsIfFalse, class _ =decltype([](){})>
    using if_else = typename detail::IfElseImpl<v, Func, ArgsIfTrue, ArgsIfFalse, _>::Res;

    template <class v, class Func, class Args, class _ =decltype([](){})>
    using if_ = typename detail::IfImpl<v, Func, Args, _>::Res;




    template <
        class func, class args,
        class stopcond, 
        class spec_cond = ctstd::True, 
        class _ = decltype([](){}), unsigned N = 0
    >
    struct DoWhile;

    template <
        class func, class args,
        class stopcond, 
        class spec_cond, 
        class _, unsigned N
    >
    struct DoWhile : 
            func:: template call<args, detail::Pair<_, detail::WrapInt<N>>> 
        {   
            using next_iteration = typename DoWhile<
                    func, args, 
                    stopcond,
                    type_var::value<stopcond, detail::Pair<_, detail::WrapInt<N>>>, 
                    _, N+1
                >::next_iteration;
        };
    
    template <
        class func, class args,
        class stopcond, 
        class _, unsigned N
    >
    struct DoWhile<func, args, stopcond, ctstd::False, _, N> {
        using next_iteration = ctstd::None;
    };
};
