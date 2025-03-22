#pragma once
#include <utility>
/// GENERAL WARNINGS :
/// std=c++20 REQUIRED
static_assert(__cplusplus >= 202002L);
/// expect frequent random crashes
/// Do not forget Wno-return-type & Wno-non-template-friend




namespace ctstd {
    // Pythonesque None
    // The result for ill-formed operations
    struct None {};

    struct True {};
    struct False {};    

    namespace detail {
        template <class T, class U>
        struct is_same_impl {
            using value = ctstd::False;
        };
        template <class T>
        struct is_same_impl<T, T> {
            using value = ctstd::True;
        };
    };
    template <class T, class U>
    using is_same = detail::is_same_impl<T, U>::value;


    template <class T>
    [[maybe_unused]] auto remove_nodiscard (T&& t) { return std::forward<T>(t); }

    template <class T>
    T naive_declval();

    namespace detail {
        template <bool cond, class T_true, class T_false>
        struct conditional_using_impl {
            using value = None;
        };

        template <class T, class U>
        struct conditional_using_impl<true, T, U> {
            using value = T;
        };
        template <class T, class U>
        struct conditional_using_impl<false, T, U> {
            using value = U;
        }; 
    };

    template <bool v, class T_true, class T_false>
    using conditional_using = typename detail::conditional_using_impl<v, T_true, T_false>::value;

    struct NoneFunc {
        template <class>
        struct call {};
    };
};

// Basic container for variadic packs
template <class ... > struct Argpass; 

namespace detail {
    template <class T, class U>
    struct Concatter {};

    template <class ... Ts, class ... Us>
    struct Concatter<Argpass<Ts...>, Argpass<Us...>> {
        using value = Argpass<Ts..., Us...>;
    };
};

template <class T, class U> using concat = typename detail::Concatter<T, U>::value;


namespace detail {
    template <template <class ...> class, class Args>
    struct PassArgpass {};

    template <template <class ...> class f, class ... Args>
    struct PassArgpass<f, Argpass<Args...>> {
        using value = f<Args...>;
    };
};

template <template <class ...> class f, class Args>
using pass_args = typename detail::PassArgpass<f, Args>::value;

namespace detail {
    template <template <class ...> class f, class ... Args>
    struct Curry {
        template <class ... Brgs>
        using call = f<Args..., Brgs...>;
    };
};

namespace detail {
    template<typename... Args>
    [[maybe_unused]] auto args_last(Args&&... args) {
        return (ctstd::remove_nodiscard(std::forward<Args>(args)), ...);
    }
    template <class ... Args>
    using pack_last_nonzero = decltype(detail::args_last(std::declval<Args>()...));

    template <class ... Args>
    using pack_last = ctstd::conditional_using<
        sizeof...(Args) == 0,
        ctstd::None, 
        pack_last_nonzero<Args...>
    >;


    template <class T, class ... Args>
    struct PackFirstNonempty {
        using value = T;
        using tail = Argpass<Args...>;
    };



    template <class ... Args>
    using pack_first = ctstd::conditional_using<
        sizeof...(Args) == 0, 
        ctstd::None, 
        typename PackFirstNonempty<Args..., int>::value
    >;

    template <class ... Args>
    using pack_tail = typename pass_args<
        PackFirstNonempty, 
        ctstd::conditional_using<
            sizeof...(Args) == 0, 
            Argpass<ctstd::None>, 
            Argpass<Args...>
        >
    >::tail;

};


template <class ... Args>
struct Argpass {
    constexpr static decltype(sizeof...(Args)) size = sizeof...(Args);
    /// FUN FACT! All of these things get implicitly instantiated and spit errors
    /// Also, they will probably eat all the memory of the compiler
    /// They have to be free functions
    // template <class> using last = detail::pack_last<Args...>;
    // template <class> using first = detail::pack_first<Args...>;
    // template <class> using tail = detail::pack_tail<Args...>;
};

template <class Args> using last = pass_args<detail::pack_last, Args>;
template <class Args> using first = pass_args<detail::pack_first, Args>;
template <class Args> using tail = pass_args<detail::pack_tail, Args>;



// template <template<class...> class Func>
// struct FuncObj {
//     template <class Args, class CpsF, class CpsArgs, class = decltype([](){})>
//     struct call : pass_args<Func, concat<Args, Argpass<CpsF, CpsArgs>>> {};

//     template <class Args, class = decltype([](){})>
//     struct __call__ : pass_args<Func, Args> {};


//     // template <class Argpass>
//     // struct curry {
//     //     template <class Brgpass>
//     //     struct call : pass_args<Func, concat<Argpass, Brgpass>> {};
//     // };
// };