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
#pragma message "Do not forget Wno-non-template-friend"
#pragma message "All template arguments named _ should receive decltype([](){}) only"


namespace type_var {

    namespace detail {
        // A trivially-constructible container for T
        // Used to force resolution of auto functions by `return Container<T>{}`
        template <class T, class _>
        struct Container : _ {
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
                return Container<Val, ctstd::None>(); // The return type is the value of T
            }
            constexpr static int value = N;
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
            return detail::Container<ctstd::None, _>{};
        }
        // If the this is the last N with well-formed flag, return the value encoded in it, otherwise go to N+1
        template <
            class T, int N = 0,
            class _,
            class U = decltype(flag(Flag<T, N>{}))
        >
        constexpr auto load_reader(int) {
            using Res = decltype(load_reader<T, N+1, _>(int{}));
            return ctstd::conditional_using<std::is_same_v<Res, detail::Container<ctstd::None, _>>, U, Res>{};
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
        int = detail::assign_reader<detail::Addr<T>, Val, 0, _>(int{})
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
        class R = typename decltype(detail::load_reader<detail::Addr<T>, 0, _>(int{}))::value
    >
    using value = R;
};

/// Control flow primitives for compile-time imperative programming
/// This namespace provides if/else conditionals and loop constructs that operate
/// on the stateful type variables defined in the type_var namespace.
namespace cexpr_control {

    /// Template wrapper for delayed instantiation of lambda functions
    /// 
    /// This provides an extra layer of indirection that can be useful for:
    /// - Debugging complex template instantiation behaviors
    /// - Controlling the order of template evaluation across different compilers
    /// - Working around compiler-specific instantiation quirks
    /// 
    /// Template parameter:
    /// - T: A lambda function (struct with __call__ template member)
    /// 
    /// Usage: Delayed<SomeFunction>::__call__<Args...> instead of SomeFunction::__call__<Args...>
    /// The behavior is identical, but the extra template layer can affect instantiation order.
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
    /// Conditional execution with both true and false branches (if-then-else)
    /// 
    /// This template provides compile-time conditional execution similar to:
    /// ```cpp
    /// if (condition) {
    ///     Func(ArgsIfTrue);
    /// } else {
    ///     Func(ArgsIfFalse);  
    /// }
    /// ```
    /// 
    /// Template parameters:
    /// - cond: Value of the condition to branch on (ctstd::True or ctstd::False)
    /// - Func: Lambda function to execute (struct with __call__ template member)
    /// - ArgsIfTrue: Arguments to pass to Func if condition is true
    /// - ArgsIfFalse: Arguments to pass to Func if condition is false
    /// - _: Unique lambda type decltype([](){}) required for stateful behavior
    /// 
    /// Returns: The result of calling Func with the appropriate arguments
    /// 
    /// Example:
    /// ```cpp
    /// using result = if_else<ctstd::True, Assignment<var>, int, bool, RE>;
    /// // This assigns int to var because condition is True
    /// ```
    template <class cond, class Func, class ArgsIfTrue, class ArgsIfFalse, class _>
    using if_else = typename detail::IfElseImpl<cond, Func, ArgsIfTrue, ArgsIfFalse, _>::Res;

    /// Conditional execution with only true branch (if-then)
    /// 
    /// This template provides compile-time conditional execution similar to:
    /// ```cpp
    /// if (condition) {
    ///     Func(Args);
    /// }
    /// ```
    /// If the condition is false, nothing happens (returns ctstd::None).
    /// 
    /// Template parameters:
    /// - cond: Value of the condition to branch on (ctstd::True or ctstd::False)
    /// - Func: Lambda function to execute (struct with __call__ template member)
    /// - Args: Arguments to pass to Func if condition is true
    /// - _: Unique lambda type decltype([](){}) required for stateful behavior
    /// 
    /// Returns: The result of calling Func with Args if condition is true, ctstd::None otherwise
    /// 
    /// Example:
    /// ```cpp
    /// using result = if_<ctstd::True, Assignment<var>, float, RE>;
    /// // This assigns float to var because condition is True
    /// 
    /// using result2 = if_<ctstd::False, Assignment<var>, int, RE>;  
    /// // This does nothing because condition is False
    /// ```
    template <class cond, class Func, class Args, class _>
    using if_ = typename detail::IfImpl<cond, Func, Args, _>::Res;

    /// Do-while loop construct for compile-time iteration
    /// 
    /// This template provides compile-time looping similar to:
    /// ```cpp
    /// do {
    ///     func();
    /// } while (value_of_stopcond_variable == True);
    /// ```
    /// 
    /// The loop executes the function at least once, then continues executing
    /// as long as the value stored in the stopcond variable remains ctstd::True.
    /// When the stopcond variable becomes ctstd::False, the loop terminates.
    /// 
    /// Template parameters:
    /// - func: Lambda function to execute each iteration (struct with __call__ template member)
    /// - stopcond: Type variable name whose value determines when to stop looping
    /// - _: Unique lambda type decltype([](){}) required for stateful behavior  
    /// - N: Internal iteration counter (starts at 0, auto-incremented)
    /// - speccond: Current value of stopcond variable (computed automatically)
    /// 
    /// Implementation notes:
    /// - Uses recursive template instantiation where each "iteration" is a new template specialization
    /// - The primary template inherits from both func::__call__ (executes the function) and 
    ///   the next iteration DoWhile<..., N+1, new_condition>
    /// - Recursion terminates when speccond becomes ctstd::False via partial specialization
    /// 
    /// Example usage:
    /// ```cpp
    /// struct counter_var {};
    /// struct stop_var {};
    /// 
    /// // Initialize: counter = 0, stop = True  
    /// struct : Assign<counter_var, peano::Zero, RE> {};
    /// struct : Assign<stop_var, ctstd::True, RE> {};
    /// 
    /// struct IncrementCounter {
    ///     template <class _>
    ///     struct __call__ :
    ///         Assign<counter_var, peano::Succ<value<counter_var, RE>>, RE>,
    ///         Assign<stop_var, peano::leq<value<counter_var, RE>, peano::Five>, RE>
    ///     {};
    /// };
    /// 
    /// // Execute loop: increment counter while counter <= 5
    /// struct : DoWhile<IncrementCounter, stop_var, RE> {};
    /// ```
    template <
        class func,
        class stopcond,
        class _,
        unsigned N = 0,
        class speccond = ctstd::True
    >
    struct DoWhile;

    /// Primary template for DoWhile - executes when loop condition is True
    /// 
    /// This template represents one iteration of the do-while loop. It:
    /// 1. Inherits from func::__call__<RE> to execute the loop body function
    /// 2. Inherits from the next iteration DoWhile<..., N+1, new_condition>
    /// 3. Automatically reads the updated value of stopcond for the next iteration
    /// 
    /// The multiple inheritance causes both the function execution and the next
    /// iteration to be instantiated, creating the loop effect through recursive
    /// template instantiation.
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

    /// Partial specialization for DoWhile - terminates when loop condition is False
    /// 
    /// This specialization provides the base case for the recursive template instantiation.
    /// When the stopcond variable contains ctstd::False, this empty specialization is
    /// chosen instead of the primary template, effectively ending the loop.
    /// 
    /// The empty body means no further function calls or iterations occur.
    template <
        class func,
        class stopcond, 
        class _, 
        unsigned N
    >
    struct DoWhile<func, stopcond, _, N, ctstd::False> {};
};