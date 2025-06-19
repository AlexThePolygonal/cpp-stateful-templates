#pragma once
#include "ctstd_base.hpp"

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
// #pragma message "Do not forget Wno-non-template-friend"
// #pragma message "All template arguments named _ should receive decltype([](){}) only"


namespace type_var {

    namespace friend_injection {
        // A trivially-constructible container for T
        // Used in `return Container<T>{}` to make all classes trivially constructible
        template <class T, class _>
        struct Container : _ {
            using value = T;
        };
        // Encapsulates `T` to avoid potential name clashes
        template <class T> class Addr {};

        // Declares friend functions comprising the global state
        template <class, int>
        struct Flag {
            // the return type of flag(Flag) encodes the value corresponding to T
            friend constexpr auto flag(Flag);
        };

        // Injects the definition of flag(Flag) into the global namespace
        template <
            class T, // name of the "variable"
            int N, // the current index of the flag
            class Val // the value to assign to the "variable"
        >
        struct Writer {
            friend constexpr auto flag(Flag<T, N>) {
                // The return type is the value of T
                return Container<Val, ctstd::None>();
            }
            constexpr static int value = N; // needed to force the instantiation of Writer
        };

        // if the flag(Flag<T, N>) doesn't exist yet, create it, storing Val in it 
        template <
            class T, class Val, 
            int N = 0, class _
        >
        constexpr int assign_reader(float) {
            return Writer<T, N, Val>::value;
        }
        // If flag(Flag<T, N>) exists, pass to N+1
        template <
            class T, class Val, 
            int N = 0,
            class _,
            class = decltype(flag(Flag<T, N>{}))
        >
        constexpr int assign_reader(int) {
            return assign_reader<T, Val, N+1, _>(int{});
        }

        // If the flag(Flag<T, N>) doesn't exist, return None
        template <class, int N = 0, class _>
        constexpr auto load_reader(float) {
            return Container<ctstd::None, _>{};
        }
        // If the this is the last N with well-formed flag, return the value encoded in it, otherwise go to N+1
        template <
            class T, int N = 0,
            class _,
            class U = decltype(flag(Flag<T, N>{}))
        >
        constexpr auto load_reader(int) {
            using Res = decltype(load_reader<T, N+1, _>(int{}));
            return ctstd::conditional_using<std::is_same_v<Res, Container<ctstd::None, _>>, U, Res>{};
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
        class _ = decltype([](){}),
        int = friend_injection::assign_reader<friend_injection::Addr<T>, Val, 0, _>(int{})
    >
    struct Assign : _ {};

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
        class T, class _ = decltype([](){}),
        class R = typename decltype(friend_injection::load_reader<friend_injection::Addr<T>, 0, _>(int{}))::value
    >
    using value = R;
};
