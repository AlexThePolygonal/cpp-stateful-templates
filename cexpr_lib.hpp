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
/// #pragma message "Do not forget Wno-non-template-friend"
/// #pragma message "All template arguments named _ should receive decltype([](){}) only"


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
                return Containter<Val>(); // The return type is the value of T
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
    
    /// Core assignment primitive: stores the value Val in the type variable T
    /// This is the fundamental operation that enables stateful metaprogramming by using
    /// friend injection to create global state at compile time.
    /// 
    /// Template parameters:
    /// - T: The type variable to assign to (acts as a variable name)
    /// - Val: The value/type to store in T (can be any type)
    /// - _: Unique lambda type decltype([](){}) required for stateful behavior
    /// 
    /// Usage: struct assignment : Assign<variable_name, value_type, RE> {};
    /// After instantiation, value<variable_name, RE> will return value_type
    template<
        class T, class Val,
        class _, 
        int = detail::assign_reader<detail::Addr<T>, Val, 0, _>(int{})
    >
    struct Assign {};

    /// Function object wrapper for delayed assignment operations
    /// Creates a callable template that can be used with control flow primitives
    /// like if_, if_else, and loop constructs that expect function objects.
    /// 
    /// Template parameter:
    /// - T: The type variable to assign to
    /// 
    /// The nested 'call' template takes:
    /// - U: The value/type to assign to T  
    /// - _: Unique lambda type decltype([](){}) required for stateful behavior
    /// 
    /// Usage examples:
    /// - if_<condition, Assignment<var>, new_value, RE>
    /// - Delayed<Assignment<var>>::call<new_value, RE>
    template <class T> struct Assignment {
        template <class U, class _> struct __call__ : Assign<T, U, _> {};
    };



    /// Retrieves the current value stored in a type variable T
    /// This is the fundamental read operation that accesses the global state
    /// created by friend injection. Uses SFINAE and overload resolution to
    /// find the most recently assigned value.
    /// 
    /// Template parameters:
    /// - T: The type variable to read from (the variable name)
    /// - _: Unique lambda type decltype([](){}) required for stateful behavior
    /// 
    /// Returns: The type/value most recently assigned to T via Assign<T, Val, _>
    /// If no assignment has been made, returns ctstd::None
    /// 
    /// Usage: using current_value = value<variable_name, RE>;
    template <
        class T, class _,
        class R = typename decltype(detail::load_reader<detail::Addr<T>, 0, _>(int{}))::value
    >
    using value = R;
};

namespace cexpr_control {

    template <class T>
    struct Delayed {
        template <class ... Args>
        struct __call__ : T::template __call__<Args...> {};
    };


    namespace detail {
        template <
            class v, 
            class Func, class Args, class Brgs, class _
            >
        struct IfElseImpl {};

        template <class Func, class Args, class Brgs, class _>
        struct IfElseImpl<ctstd::True, Func, Args, Brgs, _> {
            using Res = typename Func:: template __call__<Args, _>;
        };
        template <class Func, class Args, class Brgs,class _>
        struct IfElseImpl<ctstd::False, Func, Args, Brgs, _> {
            using Res = typename Func:: template __call__<Brgs, _>;
        };

        template <
            class v, 
            class Func, class ... Args
            >
        struct IfImpl {};

        template <class Func, class ... Args>
        struct IfImpl<ctstd::False, Func, Args...> {
            using Res = ctstd::None;
        };


        template <class Func, class ... Args>
        struct IfImpl<ctstd::True, Func, Args...> {
            using Res = typename Func:: template __call__<Args...>;
        };

        template <class T, class U>
        struct Pair {};

        template <unsigned N>
        struct WrapInt {};
    };
    // If v is true, call Func(ArgsIfTrue)
    // If v is false, call Fun(ArgsIfTrue)
    template <class cond, class Func, class ArgsIfTrue, class ArgsIfFalse, class _>
    using if_else = typename detail::IfElseImpl<cond, Func, ArgsIfTrue, ArgsIfFalse, _>::Res;

    template <class cond, class Func, class Args, class _>
    using if_ = typename detail::IfImpl<cond, Func, Args, _>::Res;

    template <
        class func,
        class stopcond,
        class _,
        unsigned N = 0,
        class speccond = ctstd::True
    >
    struct DoWhile;

    template <
        class func,
        class stopcond,
        class _,
        unsigned N,
        class speccond
    > 
    struct DoWhile: 
        func:: template __call__<
            decltype([](){})
        >,
        DoWhile<
            func, 
            stopcond, 
            _,
            N+1,             
            typename type_var::value<stopcond, decltype([](){})>
        > 
    {};

    
    template <
        class func,
        class stopcond, 
        class _, 
        unsigned N
    >
    struct DoWhile<func, stopcond, _, N, ctstd::False> {};
};
