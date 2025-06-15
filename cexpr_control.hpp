#pragma once
#include "type_var.hpp"

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