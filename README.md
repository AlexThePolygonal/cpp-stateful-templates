# The Full Power of C++ Stateful Metaprogramming

If you have never heard of stateful metaprogramming in C++ before, there is a very good reason for that. The correctness of stateful metaprogramming is debatable, and I really don't recommend using any such code in production, period. However, if you want to understand the inner workings of C++ better, 

It was discovered long ago, and there are several blogposts detailing the key idea. (This blogpost)[https://mc-deltat.github.io/articles/stateful-metaprogramming-cpp20] has a list of references for those interested. I also recommend reading (this post)[https://b.atch.se/posts/non-constant-constant-expressions/] as an introduction.

Most of them implement stateful counters, but don't go much further than that. So I decided to implement the standard primitives of an imperative language, like conditionals, loops and recursion, because I thought it would make an interesting challenge.

It was indeed a very interesting challenge, and in the end I was able to write a [C++ library](github.com/placeholder/TODO) which works for C++20 or higher, modulo a couple footguns.

I describe below how it works.

## User Interface

Most of the behaviour of the library can be seen in the [tests.hpp](todo) file. Because the lib is compile-time, the tests go into header files.

### ```namespace type_var```

The basic primitives are defined in the namespace `type_var`.

In the stateful metaprogramming paradigm, each type name can be thought of  _variable_ which refers to another variable, as in Python. Because C++ templates are untyped, there are no restrictions on the contents of a variable at all.
For example, `struct a {};` declares a new variable `a`, and `class b : a { int c = 0; };` declares a new variable `b`. 

A _function primitive_ is a template class or a `using` declaration. By the _execution of a function_ I mean the instantiation of templates in its parent classes or member functions.
A _lambda function_ is a type which has a special template function `__call__`.
The last argument of a function is always equal to `decltype([](){})`, and it is obligatory to write it out in full. I use `#define RE decltype([](){})` to save on some mental space.
The `decltype([](){})` is a unique identifier, such that each of its occurences in the program text has a different type. If you don't use it, your program will not have any state.

By default, each variable contains the special value `ctstd::None`.
To assign a different value to it, instantiate  the template class `Assign` with the last argument `decltype([](){})`.

For example, 
```c++

// we declare a variable
struct a {};

// we _call_ the assignment _function_
struct do_assign1 : type_var::Assign<a, int, decltype([](){})> {};
// now `a` holds the value "int"

struct do_assign2 : type_var::Assign<a, void, decltype([](){})> {};
// now `a` holds the value "a"
```

The value of a variable `struct a {};` is exactly equal to `value<a, decltype([](){})>`. Again, the last argument is necessary and has to be written explicitly each time. For readability, I use the the macro `RE` in the instead.

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


In the tests, I use a special macro `run_line` to execute the functions.
```c++
// defines an empty template-class
template <class> struct __run_line {};

// run_line expands to an _explicit template instantion_
// https://en.cppreference.com/w/cpp/language/template_specialization.html
#define run_line template <> struct __run_line<decltype([](){})>

// instantiates Assign as a parent class of __run_line
run_line: type_var::Assign<int, bool, RE> {};
```

This is a good start. But for a genuine language, we require conditional primitives.

### ```namespace cexpr_control```


## Implementation details