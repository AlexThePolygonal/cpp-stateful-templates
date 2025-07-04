#pragma once
#include <utility>
/// GENERAL WARNINGS :
/// std=c++20 REQUIRED
static_assert(__cplusplus >= 202002L);
/// expect frequent random crashes
/// Do not forget Wno-return-type & Wno-non-template-friend




namespace ctstd {
    
    struct None {};

    namespace detail {  struct IsBoolean {};  };
    struct True : detail::IsBoolean {};
    struct False : detail::IsBoolean {};

    
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
    using is_same = typename detail::is_same_impl<T, U>::value;

    namespace detail {
        template<typename B>
        True test_ptr_conv(const volatile B*);
        template<typename>
        False test_ptr_conv(const volatile void*);
    
        template<typename B, typename D>
        auto test_is_base_of(int) -> decltype(test_ptr_conv<B>(static_cast<D*>(nullptr)));
        template<typename, typename>
        auto test_is_base_of(...) -> True; // private or ambiguous base
    }
 
    template<typename Base, typename Derived>
    using is_base_of = decltype(detail::test_is_base_of<Base, Derived>(0));




    template <class T>
    using is_boolean = ctstd::is_base_of<detail::IsBoolean, T>;


    namespace detail {
        template <class T>
        struct ToBoolImpl {
        };
        template <>
        struct ToBoolImpl<True> {
            static constexpr bool value = true;
        };
        template <>
        struct ToBoolImpl<False> {
            static constexpr bool value = false;
        };
    };
    template <class T>
    static constexpr bool to_bool = detail::ToBoolImpl<T>::value;

    namespace detail {
        template <class T>
        struct NotImpl_ {};

        template <>
        struct NotImpl_<True> { using value = False; };
        template <>
        struct NotImpl_<False> { using value = True; };

        template <class T, class U>
        struct AndImpl_ {};
    
        template <>
        struct AndImpl_<True, True> { using value = True; };
        template <>
        struct AndImpl_<True, False> { using value = False; };
        template <>
        struct AndImpl_<False, True> { using value = False; };
        template <>
        struct AndImpl_<False, False> { using value = False; };

        template <class T, class U>
        struct OrImpl_ {};
        template <>
        struct OrImpl_<True, True> { using value = True; };
        template <>
        struct OrImpl_<True, False> { using value = True; };
        template <>
        struct OrImpl_<False, True> { using value = True; };
        template <>
        struct OrImpl_<False, False> { using value = False; };

        template <class T, class U>
        struct XorImpl_ {};
        template <>
        struct XorImpl_<True, True> { using value = False; };
        template <>
        struct XorImpl_<True, False> { using value = True; };
        template <>
        struct XorImpl_<False, True> { using value = True; };
        template <>
        struct XorImpl_<False, False> { using value = False; };

        template <class T>
        using Not_ = typename ctstd::detail::NotImpl_<T>::value;
        template <class T, class U>
        using And_ = typename ctstd::detail::AndImpl_<T, U>::value;
        template <class T, class U>
        using Or_ = typename ctstd::detail::OrImpl_<T, U>::value;
        template <class T, class U>
        using Xor_ = typename ctstd::detail::XorImpl_<T, U>::value;
    };

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
};


namespace argpass {
    // Basic container for variadic packs
    template <class ... > struct Argpass; 

    namespace detail {

        template <class T>
        [[maybe_unused]] auto remove_nodiscard (T&& t) { return std::forward<T>(t); }

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

    template <template <class ...> class f>
    struct Lambda {
        template <class ... Args>
        using __call__ = f<Args...>;
    };


    // namespace detail {
    //     template <template <class ...> class f, class ... Args>
    //     struct Curry {
    //         template <class ... Brgs>
    //         using __call__ = f<Args..., Brgs...>;
    //     };
    // };

    namespace detail {
        template<typename... Args>
        [[maybe_unused]] auto args_last(Args&&... args) {
            return (argpass::detail::remove_nodiscard(std::forward<Args>(args)), ...);
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
};