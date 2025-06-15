# C++ Stateful Metaprogramming can be made into an imperative language

Since 2015, it has been known that templates in C++ admit _state_, in the sense that there exist well-defined `constexpr` functions, the outputs of which change depending on the current state of the program.
For example, there exists a function `next` such that the code below compiles.
```cpp
template <class _ = ...>
constexpr int next() { ... }

// next has a different value each time even though it's a constant expression
static_assert(next<>() == 1);
static_assert(next<>() == 2);
static_assert(next<>() == 3);
```

Much more is possible. Inspired by prior work, I have developed an entire imperative language for C++ metaprogramming, implemented as a header-only library for the C++20 standard. The language is dynamically typed. While the syntax is unorthodox, its functionality resembles that of Python. It has all of the primitives which you need for coding, like conditionals and while loops. The statefulness implementation is traditional and uses friend injection, which I will detail in a later section.

Additionally, this approach reveals several fascinating footguns which I didn't know before. The compile times are as large as expected, and we make heavy use of compiler-dependent behaviour. Because of this, I **do not** recommend using any of this code in production.

## Getting Started

To try out the functionality yourself, clone this repository and compile `main.cpp` with, for example
```bash
g++ -std=c++20 -O0 -Wno-non-template-friend main.cpp -o a.out
```
or 
```bash
clang++ -std=c++20 -O0 main.cpp -o a.out
```
No complicated build system is necessary, as the library is header-only. 

## Public interface

The way that the language works can be seen in the [tests.hpp](https://github.com/AlexThePolygonal/cpp-stateful-templates/blob/master/tests.hpp) file. Because the lib is compile-time, the tests go into header files.

### ```namespace type_var```

The primitive stateful operations are defined in the namespace `type_var`.

In the stateful metaprogramming paradigm, each type name can be thought of as a _variable_ which refers to another entity, similar to how variables work in Python. Because C++ template parameters themselves are not type-restricted, there are effectively no conditions on what a 'variable' can refer to.
For example:
```cpp

struct a{}; // this declares a new variable `a`

// this declares a new variable `b`;
struct b : a {
    // b can contain anything at all
    int c = 0;
    void d() {};
};

// both d and d::e are both variables
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

A _lambda function_ is a typename which has a special member template `__call__`, which is a function primitive. 
```cpp

// Bar is a lambda function
struct Bar {
    template <class T, class _>
    using __call__ = Foo<T, _>;
};
```

The final template argument to these 'functions' must always be `decltype([](){})`, and it is obligatory to write it out explicitly.

The C++20 standard, 7.5.5.1, says: The type of a lambda-expression [...] is a _unique_, unnamed non-union class type.
Because of this, each occurrence of `decltype([](){})` must have a different type.
Without this unique argument, your metaprogram won't have any mutating state.
For readability, I use the the macro `RE` instead.

By default, each variable contains the special value `ctstd::None`.
To assign a different value to a 'variable', instantiate the `type_var::Assign` template class. Ensure that the final template argument is `decltype([](){})`.

For example, 
```c++
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
```c++
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
Keep in mind that unlike traditional programming, in the metaprogramming paradigm any type can play the role of a variable, and all operations and function calls are done by way of template instantiation.

| Feature          | Traditional C++ Variable | Metaprogramming "Variable" |
|------------------|--------------------------|----------------------------|
| Declaration      | `int x;`                 | `struct x {};`             |
| Values           |  Bits                    | C++ types                  |
| Typing           | Fixed at compile time    | Dynamic (can refer to any kind of value) |
| Assignment       | `x = 10;`                | `struct s: Assign<x,int,RE>{};` |




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
This pattern becomes particularly useful when implementing control flow mechanisms.

In the tests, I use a special macro `run_line` to execute the functions.
```c++
template <class> struct __run_line {};

// run_line expands to an _explicit template instantiation_ of __run_line
// https://en.cppreference.com/w/cpp/language/template_specialization.html
#define run_line template <> struct __run_line<decltype([](){})>

// instantiates Assign as a parent class of __run_line
run_line: type_var::Assign<int, bool, RE> {};
```
The code `template <> struct a<T>` is [explicit template instantiation](https://en.cppreference.com/w/cpp/language/template_specialization.html), where instead of waiting for the template struct `a` to be used with the argument `T`, we explicitly state that `a<T>` needs to be instantiated, immediately.

This is the basis of mutable state manipulation. But for a complete language, we need to construct control flow primitives.

### ```namespace cexpr_control```

This namespace contains the implementations of if and while.

The `Delayed<Func>` lambda function wraps another lambda function `Func` within an additional layer of template indirection. While it doesn't alter the core logic, it can be a useful tool for debugging.

The function `if_<cond, Func, Args, _>` does exactly what you'd expect. If `cond` is `ctstd::True`, then `Func` will be called with the argument `Args`, and the last argument is always `decltype([](){})`. To pass multiple arguments to Func, they should be bundled into an Argpass<...> template, which Func would then need to unpack.

The function `if_else` is analogous. If the condition is `True`, it will call `Func` with `ArgsIfTrue`, if the condition is `False`, it will call it `Func` with `ArgsIfFalse`.

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

The DoWhile function primitive operates on a similar principle.
It will repeatedly call Func (with empty arguments, or predefined arguments if designed so) as long as value<StopCondVar, RE> evaluates to ctstd::True.
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

It's important to note a distinction: the if_ function typically branches based on an immediate boolean 'value' (e.g., `ctstd::True`), while DoWhile often takes a 'variable' (e.g., `int` or `Foo`) whose stored 'value' is checked in each iteration

### ```namespace ctstd```

This namespace contains the primitive "types" and "values" of the language.

For consistency, they're re-implemented from scratch. I believe it enhances the Pythonic flavour.

- `ctstd::None` is analogous to `None` in Python.
- `True` and `False` are boolean values. 
- `And`, `Or`, `Xor`, `Not` are primitive functions on the booleans.

Arithmetic is implemented using Peano axioms:

- `_0, ..., _12` and `Integer<N>` represent compile-time integers.
- `add, mult, divide, remainder, leq` are the primitive arithmetic and comparison operations on these integers, functioning as expected.

Lists aren't implemented yet.

The following helper templates are provided for constructing functions that accept multiple arguments. For now, they weren't of much use during the testing.

- `Argpass<...>` is a template for passing tuples of values
- `last`, `first`, `tail` allow to extract values from an Argpass tuple.
- `Lambda` transforms a function with multiple arguments into a lambda function which accepts a single Argpass.


### Function execution behaviour

Stateful metaprogramming is not an intended feature of C++. Standard C++ implicitly assumes templates are stateless, and compilers have no obligation to instantiate templates in a specific order that would preserve stateful semantics. Because of it, most of the code in this repository is UB.

To be exact, the standard doesn't give any information on the order in which different templates have to be instantiated, as it assumes that it cannot matter. 

In practice, template instantiation order behaves predictably on each compiler. I have tested on Clang 19.1.1 and GCC 14.2.0, and the behaviour is similar on the earlier versions, excluding random compiler crashes.

Clang appears to instantiate templates largely in the order they appear in the code, recursively. It generally instantiates base classes before processing members of a derived class. For example:
```cpp
struct do : 
    Foo<RE>, // clang always instantiates Foo, starting with its parent classes 
    Bar<RE> // Then, clang instantiates Bar in the same way
{};

```

GCC behaves in a bizarre way. It behaves identically to Clang, except when instantiating parent template classes, it sometimes evaluates all of the "simple-looking" arguments, then processes the rest sequentially. I was, unfortunately, unable to find a description of the heuristics that cause this behaviour.

This divergence in instantiation order can lead to different, and very surprising, behaviors for stateful 'functions' on GCC compared to Clang, as demonstrated in the recursive tests
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

On GCC, the "simple" templates `add<a, _1, RE>` and `add<b, a, RE>` are resolved fully before any of the `Assign` operations are processed. In other words, the `value<a, RE>` lookups are resolved upfront based on the state before any of the `Assign`s take effect.
Consequently, the assignments operate on stale data.

## Implementation details

The method was discovered long ago, and there are several blogposts detailing the key idea, «friend injection». [This blogpost](https://mc-deltat.github.io/articles/stateful-metaprogramming-cpp20) has a list of references for those interested. I also recommend reading [this](https://b.atch.se/posts/non-constant-constant-expressions/) as an introduction.

TODO