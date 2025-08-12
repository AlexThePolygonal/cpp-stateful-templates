# Emulating an Imperative Language with C++ Templates
<!-- # Exploring Undefined Behaviour with Stateful C++ templates -->

Since 2015, we know that templates in C++ can maintain state: there exist well-defined `constexpr` functions, the outputs of which change depending on the current state of the program.
For example, it is possible to implement a template class `next` such that the code below compiles.

```cpp
template <class _ = /* implementation details */>
struct next {
    const static int value = /*impl details*/;
}

// next has a different value each time even though it's a constant expression
static_assert(next<>::value == 1);
static_assert(next<>::value == 2);
static_assert(next<>::value == 3);
```

Much more is possible. Inspired by older blog posts, I show how to expand this trick to emulate a rudimentary dynamically typed language with C++ template metaprogramming. The syntax is rather unorthodox. We implement conditionals and loops, and leave lists and dicts as an exercise. The statefulness implementation uses friend injection, a well-established trick, which I will explain in the [section on implementation details](#implementation-details).

Obviously, **do not use this code in production, ever**! However, you can use it for research, or for compiler stress-tests. It allows us to observe how compilers sequence template instantiation and opens a fascinating whole new class of UB. I suspect that we can observe unintended behaviour in Clang that way, but due to the overwhelming amount of UB I can't be sure. 

Since this is an exploratory project, I didn't investigate the compilation times, but I assure you that they are very large. For example, the tests take 3 seconds to run on my core i7 machine. For the same reason, I didn't use modern features like concepts.

#### Running the code

To try out the functionality, clone this repository and compile `main.cpp` with, for example

```bash
g++ -std=c++20 -O0 -Wno-non-template-friend main.cpp -o a.out
```

or

```bash
clang++ -std=c++20 -O0 main.cpp -o a.out
```

No complex build system is required, as the library is header-only. I have tested this code with gcc 15.1.1, 14.2.0, 13.3.0 and clang 19.1.1, 18.1.3, 16.0.6 and 15.0.7, and the library behaves the same way across different versions of the same compiler.

## Public interface & semantics

The way that the language works can be seen in the [tests.hpp](https://github.com/AlexThePolygonal/cpp-stateful-templates/blob/master/tests.hpp) file. Because the library is compile-time, the tests go into header files.

### ``namespace type_var``

The primitive stateful operations are defined in `type_var`.

From this point onward, each type name can be thought of as a _variable_ which refers to another entity, similar to how variables work in Python. Because C++ templates accept any valid type, there are effectively no conditions on what a 'variable' can refer to.
For example:

```cpp
struct a{}; // this declares a new variable `a`

// this declares a new variable `b`;
struct b : a {
    // b can contain anything at all
    int c = 0;
    void d() {};
};

// both d and d::e are variables
struct d {
    struct e {};
};
```

A _function primitive_ is either a template class or a using declaration that instantiates a template. By the _execution of a function_ I mean the instantiation of the templates involved, including those in its parent classes, member types/aliases and member functions.

```cpp
// Foo is a function
template <class T, class _>
struct Foo : ... {};

// we execute Foo(int) by instantiating the template
using call_foo_1 = Foo<int, decltype([](){})>;

// we execute Foo(bool)
struct call_foo_2 : Foo<bool, decltype([](){})> {};
```

A _lambda function_ is a type that has a special member template `__call__`, which is a function primitive.

```cpp
// Bar is a lambda function
struct Bar {
    template <class T, class _>
    using __call__ = Foo<T, _>;
};
```

The final argument of all functions, called `class _`, must always be `decltype([](){})`, and it is obligatory to write it out explicitly. Using it as the default argument doesn't work on Clang even though it should. 

Each occurrence of decltype([](){}) generates a unique type (C++20 Standard, 7.5.5.1), forcing template re-instantiation. If it is not used, the compiler is free to assume that in the sequence `Assign<a, int>, Assign<a, bool>, Assign<a, int>` the third class is already instantiated, because it looks the same as the first.

For readability, I use the macro `#define RE decltype([](){})` instead. In the later code examples, I will avoid listing the necessary includes and defines for the same reason.

By default, each variable contains the special value `ctstd::None`.
To assign a different value to a 'variable', instantiate the `type_var::Assign` class template. Ensure that the final template argument is `RE`.

For example,

```cpp
#include "type_var.hpp"
#define RE decltype([](){})

// we declare a variable
struct a {};

// we _call_ the assignment _function_
struct do_assign1 : type_var::Assign<a, int, RE> {};
// now `a` holds the value "int"

struct do_assign2 : type_var::Assign<a, void, RE> {};
// now `a` holds the value "void"
```

The current value of a variable `struct a {};` can be retrieved using `value<a, decltype([](){})>`.

Example:

```cpp
#include "type_var.hpp"
#define RE decltype([](){})
using namespace type_var;

// we declare a variable
struct a {};

static_assert(
    std::is_same_v<
        value<a, RE>,
        ctstd::None
    >, "the value of an uninitialized variable is None"
)

// we assign int to `a`
struct do_assign1 : Assign<a, int, RE> {};


static_assert(
    std::is_same_v<
        value<a, RE>,
        int
    >, "the value of a is now int"
)
```

Keep in mind that in the metaprogramming paradigm any type can play the role of a variable, and all operations and function calls are done by template instantiation. If you want to look at code examples, skip to [section on examples](#examples).

| Feature     | Traditional C++ Variable | Metaprogramming "Variable"               |
| ----------- | ------------------------ | ---------------------------------------- |
| Declaration | `int x;`               | `struct x {};`                         |
| Values      | Bits                     | C++ types                                |
| Typing      | Fixed at compile time    | Dynamic (can refer to any kind of value) |
| Assignment  | `x = 10;`              | `struct s: Assign<x,int,RE>{};`        |

Assignments to the variable `T` can also be performed by a lambda function `Assignment<T>`.

```cpp
struct a {};

using Func = Assignment<a>;

struct do_assignment : Func::template __call__<int, RE> {};

static_assert(
    std::is_same_v<
        value<a, RE>,
        int
    >, "the value of a is now int"
);
```

This lambda is useful for implementing control flow mechanisms.

In the tests, I use a special macro `run_line` to execute the functions.

```cpp
template <class> struct __run_line {};

// run_line expands to an _explicit template instantiation_ of __run_line
// https://en.cppreference.com/w/cpp/language/template_specialization.html
#define run_line template <> struct __run_line<decltype([](){})>

// instantiates Assign as a parent class of __run_line
run_line: type_var::Assign<int, bool, RE> {};
```

The code `template <> struct a<T>` is [explicit template instantiation](https://en.cppreference.com/w/cpp/language/template_specialization.html), where instead of waiting for the template struct `a` to be used with the argument `T`, we explicitly state that `a<T>` needs to be instantiated, immediately.

This is the basis of mutable state manipulation. But for a complete language, we need to construct control flow primitives.

### ``namespace cexpr_control``

This namespace contains the implementations of if and while.

The `Delayed<Func>` lambda function wraps another lambda function `Func` within an additional layer of template indirection. While it doesn't alter the core logic, it can be a useful tool for debugging.

The function `if_<cond, Func, Args, _>` does exactly what you'd expect. If `cond` is `ctstd::True`, then `Func` will be called with the argument `Args`, and the last argument is always `decltype([](){})`. To pass multiple arguments to Func, they should be bundled into an Argpass<...> template, which Func would then need to unpack. It is used with an underscore to avoid collision with the default `if` keyword.

The function `if_else` is analogous. If the condition is `True`, it will call `Func` with `ArgsIfTrue`, if the condition is `False`, it will call `Func` with `ArgsIfFalse`.

To reiterate, the call

```cpp
using _1 = if_<True, Foo, Args, RE>;
```

achieves a similar effect to the following imperative code

```cpp
if (true) {
    Foo(Args);
}
```

The `DoWhile<stop_condition, Func>` function primitive operates on a similar principle.
It will repeatedly call the function `Func` (with empty arguments) as long as the value of the variable `stop_condition` evaluates to `ctstd::True`.
The code

```cpp
using _1 = DoWhile<is_nonempty, Foo, RE>;
```

has the same function as the following imperative code

```cpp
do {
    Foo();
} while (is_nonempty);
```

An important distinction is that the if_ function typically branches based on an immediate boolean 'value' (e.g., `ctstd::True`), while DoWhile often takes a 'variable' (e.g., `int` or `Foo`) whose stored 'value' is checked in each iteration

### ``namespace ctstd``

This namespace contains the primitive "types" and "values" of the language.

For consistency, they're re-implemented from scratch. I believe it enhances the Pythonic flavor.

- `ctstd::None` is analogous to `None` in Python.
- `True` and `False` are boolean values.
- `And`, `Or`, `Xor`, `Not` are primitive functions on the booleans.

Arithmetic is implemented using Peano axioms:

- `_0, ..., _12` and `Integer<N>` represent compile-time integers.
- `add, mult, divide, remainder, leq` are the primitive arithmetic and comparison operations on these integers, functioning as expected.

Lists and dicts aren't implemented yet.

The following helper templates are provided for constructing functions that accept multiple arguments. They weren't of much use during the testing of the stateful templates, but maybe they will prove useful in future work.

- `Argpass<...>` is a template for passing tuples of values
- `last`, `first`, `tail` allow to extract values from an Argpass tuple.
- `Lambda` transforms a function with multiple arguments into a lambda function which accepts a single Argpass.

### Examples

First, we compute the sum of the first $n$ natural numbers.
```cpp
int i = 0;
int sum = 0;
do {
    sum += i;
    i++;
} while (i <= 5);
```
It can be implemented using stateful templates as follows:

```c++
// boilerplate
#define run_line template <> struct __run_line<decltype([](){})>
#define RE decltype([](){})

using namespace type_var;
using namespace ctstd;
using namespace cexpr_control;

template <class> struct __run_line {};

// variable init
struct i {};
struct sum {};
struct i_leq_5 {};
run_line : Assign<i, peano::_0, RE> {};
run_line : Assign<sum, peano::_0, RE> {};
run_line : Assign<i_leq_5, ctstd::True, RE> {};

// a single step of the loop
struct SumOfFirstIntegers {
    template <class _>
    struct __call__ : 
        Assign<i, 
            peano::Succ<value<i, RE>>, RE           // i = i+1;
        >,
        Assign<sum,
            peano::add<value<sum, RE>, value<i, RE>>, RE // sum = sum + i;
        >,
        Assign<i_leq_5,
            peano::leq<value<i, RE>, peano::_5>, RE // set condition i <= 5
        >
    {};
};

// Run the loop
run_line : DoWhile<
    SumOfFirstIntegers, 
    i_leq_5, 
RE> {};


#ifdef __clang__
static_assert(std::is_same_v<value<sum, RE>, peano::Integer<21>>); // clang computes the answer correctly
#elif __GNUG__
static_assert(std::is_same_v<value<sum, RE>, peano::Integer<28>>); // GCC runs an additional step, adding 7. Why??
#endif

```

The code below implements the Collatz sequence using the new control flow primitives.
It's the biggest imperative construct I have tested and it shows everything we have looked at so far.

```c++
// boilerplate
#define run_line template <> struct __run_line<decltype([](){})>
#define RE decltype([](){})

using namespace type_var;
using namespace ctstd;
using namespace cexpr_control;

template <class> struct __run_line {};

// initialization
struct a {}; // the variable of the Collatz sequence
struct steps {}; // the step count
struct didnt_reach_one {}; // whether we reached 1

// Initialization
run_line: Assign<a, _7, RE> {}; 
run_line: Assign<steps, _0, RE> {};
run_line: Assign<didnt_reach_one, True, RE> {};


// The Collatz step is a single step of the Collatz sequence.
struct CollatzStep {
    template <class _>
    struct __call__ :
        if_else<                               // if (
            eq<remainder<a, _2, RE>, _0, RE>,  //     a % 2 == 0) {
            Assignment<a>,                     //
            divide<a, _2, RE>,                 //        a =  a / 2;
            add<mult<a, _3, RE>, _1, RE>,      // } else { a = 3 * a + 1; }
            RE
        >,
        Assign<steps, add<steps, _1, RE>, RE>,         // b = b + 1;
        Assign<didnt_reach_one, Not<eq<a, _1, RE>, RE>, RE>  // c = (a != 1);
    {};
};

// run CollatzStep while didnt_reach_one is True
run_line: DoWhile<
    CollatzStep, 
    didnt_reach_one, 
RE> {};

// It should take 16 steps
// 7 > 22 > 11 > 34 > 17 > 52 > 26 > 13 > 40 > 20 > 10 > 5 > 16 > 8 > 4 > 2 > 1

#ifdef __clang__
static_assert(to_bool<eq<a, Integer<1>, RE>>); 
static_assert(to_bool<eq<steps, Integer<16>, RE>>); // Clang behaves as expected
#elif __GNUG__
static_assert(to_bool<eq<a, Integer<2>, RE>>);
static_assert(to_bool<eq<steps, Integer<18>, RE>>); // wtf happens here?? read the next section...
#endif

```
Unfortunately, Clang and GCC compile this code differently. Still, if the code was invalid, it would most likely run into an infinite loop or emit an error, it wouldn't output 18, a number very close to the true result, 16. For some reason, GCC decides to run two additional Collatz steps after a reaches 1.
This is consistent across all choices of initial values for `a`: the number of steps is always off by two. I attempt to explain why in the next subsection.

### Template instantiation ordering

Stateful metaprogramming is not an intended feature of C++. The standard doesn't give any information on the order in which different templates have to be instantiated, as it assumes that it cannot matter. Then, while stateful metaprogramming is well-defined, the behaviour of `Assign<>` and `value<>` is UB.

In practice, template instantiation order behaves predictably on each compiler. I have tested it thoroughly on Clang 19.1.1 and GCC 14.2.0, and the behaviour is identical on the preceding versions of the compilers, except that the Clang frontend crashes unpredictably on some of the earlier versions.

#### Clang is simple

Clang instantiates templates in the order they appear in the code, recursively. It instantiates base classes before processing members of a derived class. For example:

```cpp
struct do : 
    Foo<RE>, // clang always instantiates Foo, starting with its parent classes 
    Bar<RE> // Then, clang instantiates Bar in the same way
{};
```

#### GCC is complex

GCC uses complex optimization heuristics in addition to the naïve implementation. It behaves identically to Clang, except when instantiating parent template classes, it sometimes evaluates all of the "simple-looking" arguments, then processes the rest sequentially. I was, unfortunately, unable to find a description of the heuristics that cause this behaviour.

This leads to different, and very surprising, results of stateful 'functions' on GCC and Clang, and you can see it in the recursive tests.

```cpp
struct SumOfFirstIntegers {
    template <class _>
    struct __call__ :
        // a = a + 1; 
        Assign<a, add<a, _1, RE>, RE>,
        // b = b + a;
        Assign<b, add<b, a, RE>, RE>,
        // c = (a <= 5);
        Assign<c, leq<a, _5, RE>, RE>
    {};
};
```

This function is a single step of the loop which is used to compute the sum of the first 5 peano integers. `a` is the counter, `b` is the accumulator variable, and `c` is the stopping condition.

On GCC, the "simple" templates `add<a, _1, RE>` and `add<b, a, RE>`, and the `value<a, RE>` lookups inside are resolved before any of the `Assign` operations take effect.
Consequently, the assignments operate on stale data.

Another example of instant instantiation of simple templates is this:

```cpp
run_line: 
    Assign<a, short, RE>, 
    if_<True, Assignment<a>, int, RE>, 
    Assign<a, long, RE> 
{};
#ifdef __clang__
    static_assert(std::is_same_v<value<a, RE>, long>, "Clang instantiates the templates depth-first, in the order they are written in the parent list");
#elif __GNUG__
    static_assert(std::is_same_v<value<a, RE>, int>, "GCC evaluates the non-recursive template first in the order they are written, then the recursive one");
#endif
```

GCC doesn't want to instantiate parent classes in the order that they appear, even though it is logical, and instead construct the "simpler" `Assign`s first, deferring the "complicated" `if_<True, Assignment<a>>` for later. I have written "simple" and "complicated" in quotes because I was unable to find a simple description of the conditions under which GCC defers template instantiation to a later stage.

### Uniqueness issues

Normally, you would expect that explicitly writing out `RE` each time is unnecessary, as we can use it as the default template argument:

```cpp
template <class Var, class _ = decltype([](){})>
using value = detail::ValueImpl<Var, _>::value;
```

Each time the template is instantiated, a new lambda type must be used.
However, Clang might decide to ignore its obligations and re-use the same lambda type in a new template, maybe because the type `ValueImpl<...>::value` does not depend on `_`. I expect this to be a Clang bug, however, I couldn't find a way to trigger it without using stateful loops, and the order of template instantiations in them is UB.

Debugging this has been a nightmare, because, as you might have noticed in the previous section, template instantiations aren't sequenced in a predictable manner.

## Implementation details

I will give an informal overview of the key stateful metaprogramming method, called «friend injection». It allows us to construct the principal primitives, `value` and `Assign`.

Since the method itself was discovered long ago, there are several blogposts detailing it. [This blogpost](https://mc-deltat.github.io/articles/stateful-metaprogramming-cpp20) has a list of references for those interested. I also recommend reading [this](https://b.atch.se/posts/non-constant-constant-expressions/) as an introduction.

For my implementation, which is slightly more convoluted, read [type_var.hpp](https://github.com/AlexThePolygonal/cpp-stateful-templates/blob/master/type_var.hpp).

To have state and templates like `value<T, _>`, first, we need some way of finding the type corresponding to `T`. Normally, templates are stateless and finding a template construct requires knowing all of its arguments.
As an example, we could try to construct `value` and `Assign` like this

```cpp
template <>
struct Assign<T, Val1, RE> {
    using value = Val1;
};

template <>
struct Assign<T, Val2, RE> {
    using value = Val2;
};

// can't be completed :(
using value = Assign<T, ...>::value;
```

As you see, to fill the ellipsis in the last line, we need to "find" the assign template where the value `Val2` is stored, and for that, we need to know both `Val2` and the `RE` we used to construct it.

However, global functions don't have that problem:

```cpp
template <>
Val2 foo<T, Val2, RE>(T t) {
    return Val2{};
};

using value = decltype(foo(T{}));
```

We can find the `Val2` using only `T` by global function lookup.
Then we can "store" type names as result types of functions in the global namespace.
To be able to update them, we can index them by integers and search for the topmost one using SFINAE. I will call these functions flag functions, and they will look like this:

```cpp
Value flag(Flag<T, N>) { return Value{}; }
```

The class `Flag<T, N>` is just a container for the name `T` and the integer `N`.
The flag functions go from 0 to N, and the global namespace will contain the following definitions:

```cpp
auto flag(Flag<T, 0>) { return Value0{}; }
auto flag(Flag<T, 1>) { return Value1{}; }
auto flag(Flag<T, 2>) { return Value2{}; }
```

To find the topmost flag, we leverage SFINAE to iterate across all flags starting from 0, and stopping at the last one.

```cpp
template <class T, int N = 0, class U = decltype(flag(Flag<T, N>{}))>
constexpr auto reader(int) {
    return reader<T, N+1>(int{});
}

template <class T, int N = 0>
constexpr auto reader(float) {
    return decltype(flag(Flag<T, N-1>));
}

using value = decltype(reader<T>(int{}));
```

When `reader<T>(int{})` is invoked, it first attempts to instantiate the `reader<T>(int)` function, which is only possible if `flag(Flag<T,N>)` exists. If so, it calls the next reader function, incrementing `N` by one. If the flag function at this index `N` doesn't exist, it is unable to construct the `reader(int)` functions, so it casts the `int{}` to `float` and instantiates `reader(float)` instead. Then the `reader(float)` function retrieves the value of the topmost flag at index `N-1`.

However, we can't explicitly write the `flag` functions in the global namespace.
The `friend` keyword allows us to declare such functions inside a class and inject them into the global namespace, where they can be found by argument-dependent lookup.
Then, we can define this function later.

```cpp
template <class, int>
struct Flag {
    // injects the flag function definition into the global namespace
    // Surprisingly, we do not need to specify its type.
    friend constexpr auto flag(Flag);
};

template <class T, int N, class Val>;
struct Writer {

    // defines the injected flag function, setting its result type to `Val`
    friend constexpr auto flag(Flag<T, N>) {
        return Val{};
    }
};
```

Combining these elements, we can construct `Assign` as well. First, we find the top `N` such that `flag(Flag<T, N>)` exists, then we instantiate a new `Writer`, injecting a new `flag` instance into the global namespace. The return type of this new flag will be the "value" assigned to `T`.

On GCC it triggers a warning, because GCC believes that if a class template has non-template friend functions, the programmer probably made an error.
We silence it with the -Wno-non-template-friend compiler option.

Note that this implementation is deeply inefficient: performing $N$ assignments requires instantiating $O(N)$ different classes and doing $O(N^2)$ work in total.

## Conclusion

This is the most _fun_ template metaprogramming exercise I've ever tried, and the fact that I was able to make recursion work, despite the _interesting_ behaviour of the compilers, makes me happy.

The UB we are able to observe is very interesting, but I doubt that we can make use of these techniques in a standard-conforming manner, and the behaviour isn't consistent across major compilers.

If you have any ideas on why GCC instantiates templates in this complicated order, please let me know, I really want to understand it! Also, if you have any thoughts on which parts of the code are Standard-conforming and which are not, don't hesitate to share them.

On the other hand, if you have any opinions on the design of the C++ language, you can address them to the C++ Standards Committee or, maybe, switch to Rust.