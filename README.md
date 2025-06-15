# C++ Stateful Metaprogramming can be made into an imperative language

Since 2015, we know that templates in C++ admit _state_, in the sense that there exist well-defined `constexpr` functons, the outputs of which change depending on the current state of the program.
For example, there exists a function `next` such that the code below compiles.
```cpp
template <class _ = ...>
constexpr int next() { ... }

// next has a different value each time even though it's a constant expression
static_assert(next<>() == 1);
static_assert(next<>() == 2);
static_assert(next<>() == 3);
```

Much more is possible. Inspired by old blog posts, I was able to create an entire imperative language for C++ metaprogramming, implemented as a header-only library for the C++20 standard. The language is dynamically typed and, even though the syntax is unorthodox, it might look like Python at first glance. It has all of the primitives which you need for coding, like conditionals and while loops. 

Also, it allows us to discover several fascinating footguns which I didn't know before.

## Public interface

The way that the language works can be seen in the [tests.hpp](https://github.com/AlexThePolygonal/cpp-stateful-templates/blob/master/tests.hpp) file. Because the lib is compile-time, the tests go into header files.

### ```namespace type_var```

The primitive stateful operations are defined in the namespace `type_var`.

In the stateful metaprogramming paradigm, each type name can be thought of  _variable_ which refers to another variable, as in Python. Because C++ templates are untyped, there are no restrictions on the contents of a variable at all.
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

A _function primitive_ is a template class or a `using` declaration. By the _execution of a function_ I mean the instantiation of templates in its parent classes or member functions.
```cpp

// Foo is a function
template <class T, class _>
struct Foo : ... {};

// we execute Foo(int) by instantiating the template
using call_foo_1 = Foo<int, decltype([](){})>

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

The last argument of a function is always equal to `decltype([](){})`, and it is obligatory to write it out in full.
The `decltype([](){})` is a unique type, that is, each of its occurences in the program text has a different type. If you don't use it, your program won't have any mutating state.
For readability, I use the the macro `RE` instead.

By default, each variable contains the special value `ctstd::None`.
To assign a different value to it, instantiate  the template class `Assign` with the last argument `decltype([](){})`.

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

The value of a variable `struct a {};` is exactly equal to `value<a, decltype([](){})>`. 

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

Assignments to the value `T` can also be perfomed by a lambda function `Assignment<T>`, which 
```cpp

struct a {};

using Func = Assignment<a>;

struct do_assignment : template Func::__call__<int> {};

static_assert(
    std::is_same_v<
        value<a, RE>,
        int
    >, "the value of a is now int"
)
```
This will be useful later, when we encounter flow control for functions.


In the tests, I use a special macro `run_line` to execute the functions.
```c++
template <class> struct __run_line {};

// run_line expands to an _explicit template instantion_ of __run_line
// https://en.cppreference.com/w/cpp/language/template_specialization.html
#define run_line template <> struct __run_line<decltype([](){})>

// instantiates Assign as a parent class of __run_line
run_line: type_var::Assign<int, bool, RE> {};
```

This is a good start. But for a genuine language, we require control flow primitives.

### ```namespace cexpr_control```

This namespace contains the implementations of if and while.

The lambda function `Delayed<Func>` wraps the lambda function `Func` inside a layer of template misdirection. It doesn't do anything, but allows us to debug complicated compiler behaviours.

The function `if_<cond, Func, Args, _>` is exactly what it says on the tin. If `cond` is `ctstd::True`, then `Func` will be called with the argument `Args`, and the last argument is always `decltype([](){})`. To pass multiple arguments, fold the arguments into a argument-passing template `Argpass<...>` and unfold it later.

The function `if_else` is analogous. If the condition is `True`, it will call `Func` with `ArgsIfTrue`, if `False`, it will call it with `ArgsIfFalse`.

To repeat myself, the call
```cpp 
using _1 = if_<True, Foo, Args, RE>;
```
has the same function as the following imperative code
```cpp
if (true) {
    Foo(Args);
}
```

The function `DoWhile` works on a similar principle. 
It will call `func` with empty argument while `value<stopcond, RE>` is `True`, then stop.
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

I wish to highlight that the `if_` function branches on raw values, while the `DoWhile` function takes a variable and branches on the value stored in the variable.

### ```namespace ctstd```

This namespace contains the primitive "types" and "values" of the language.

For purity's sake, they're re-implemented from scratch. I believe it enhances the Pythonic flavour.

- `ctstd::None` is analogous to `None` in Python.
- `True` and `False` are boolean values. 
- `And`, `Or`, `Xor`, `Not` are primitive functions on the booleans.

Arithmetic is implemented using the Peano constructions.

- `_0, ..., _12` and `Integer<N>`are simple integers
- `add, mult, divide, remainder, leq` are the primitive operations on the integers, they do exactly what it says on the tin.

Lists aren't implemented yet.

These special helper templates are needed if you want to construct functions which take multiple arguments. For now, they weren't of much use during the testing.

- `Argpass<...>` is a template for passing tuples of values
- `last`, `first`, `tail` allow to extract values from an Argpass tuple.
- `Lambda` transforms a function with multiple arguments into a lambda function which accepts a single Argpass.


### Function execution behaviour

Stateful metaprogramming isn't an intended part of C++. Because templates are assumed to be stateless, the compiler has no obligation to execute them in any reasonable order, and almost all of the code in this repository is UB.

In practice, template instantiation behaves predictably on each compiler. I have tested on clang++-19 and gcc-14, and the behaviour is the same on earlier version, excluding random compiler crashes.

Clang executes all templates in the order that they appear, recursively. First, Clang instantiates the parent classes, then the classes occuring in the body of the class.

GCC behaves in a bizarre way. It behaves identically to Clang, except when instantiating parent template classes, it first evaluates all of the """simple""" arguments, then processes the rest sequentially. I was unable to find any way to describe it.

Because of this, functions on gcc work weirdly, which can be seen in the recursive tests.
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

On GCC, the simple templates `value<a, RE>` and `value<b, RE>` are instantiated instantly, all of them before any of the `Assign`s are.
Because of it, all of the arguments of the assignments have the same value.


## Implementation details

The method was discovered long ago, and there are several blogposts detailing the key idea, «friend injection». [This blogpost](https://mc-deltat.github.io/articles/stateful-metaprogramming-cpp20) has a list of references for those interested. I also recommend reading [this](https://b.atch.se/posts/non-constant-constant-expressions/) as an introduction.

TODO