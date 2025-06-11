#include "cexpr_lib.hpp"
#include "peano.hpp"
#include <iostream>
#include <array>


#define run_line template <> struct __run_line<decltype([](){})>
#define RE decltype([](){})

// testing whether the assignment mechanism works well with all types, including the special ones like void, const, references, etc.
namespace basic_tests {
    using namespace type_var;
    template <class> struct __run_line {};

    struct a {};
    static_assert(std::is_same_v<
            value<a, RE>, 
            ctstd::None
        >, "the value of initialized variable must be None");

    run_line : Assign<a, a, RE> {};
    static_assert(std::is_same_v<value<value<value<a, RE>, RE>, RE>, a>, "the value of a variable can be the variable name itself");

    run_line : Assign<a, void, RE> {};
    static_assert(std::is_same_v<value<a, RE>, void>, "assignment works with void");
    
    run_line : Assign<a, const int, RE> {};
    static_assert(std::is_same_v<value<a, RE>, const int>, "assignment works with const types");

    run_line : Assign<a, int&, RE> {};
    static_assert(std::is_same_v<value<a, RE>, int&>, "assignment works with reference types");

    run_line : Assign<a, Assign<a, unsigned, RE>, RE> {};
    static_assert(!std::is_same_v<value<a, RE>, unsigned>, "nested assigments are executed before the outer ones");

    run_line : Assign<a, int, RE> {};
    run_line : Assign<a, float, RE> {};
    run_line : Assign<a, int, RE> {};

    static_assert(std::is_same_v<value<a, RE>, int>, "assignments are executed in the order they are written, so the last one is the value of a");
};


// These tests check whether the control flow expressions if_ and if_else work correctly 
namespace cexpr_control_tests {
    using namespace ctstd;
    using namespace type_var;
    using namespace cexpr_control;

    template <class> struct __run_line {};

    struct a {};

    run_line : if_<True, Assignment<a>, float, RE> {};
    run_line : if_<True, Assignment<a>, double, RE> {};
    run_line : if_<True, Assignment<a>, bool, RE> {};
    run_line : if_<False, Assignment<a>, int, RE> {};


    static_assert(std::is_same_v<value<a, RE>, bool>, "if_ works correctly with True and False");

    struct b {};

    run_line : if_else<True, Assignment<b>, float, bool, RE> {};
    run_line : if_else<True, Assignment<b>, double, bool, RE> {};
    run_line : if_else<False, Assignment<b>, bool, float, RE> {};

    static_assert(std::is_same_v<value<b, RE>, float>, "if_else works correctly with True and False");
};

// these tests check the order in which parent template classes are instantiated
// The assignments show that this order depends on the compiler
// Clang instantiates the templates depth-first, while GCC short-circuits the instantiation of non-folded templates
namespace inheritance_order_tests {
    using namespace ctstd;
    using namespace type_var;
    using namespace cexpr_control;

    template <class> struct __run_line {};

    struct a {};

    run_line : Assign<a, unsigned, RE>, Assign<a, int, RE> {};

    static_assert(std::is_same_v<value<a, RE>, int>, "templates of the same depth are instantiated in the order they are written");

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

    struct b{};

    run_line : Assign<a, long, RE> {};

    run_line: 
        if_<True, Delayed<Assignment<b>>, int, RE>, 
        if_<True, Assignment<b>, short, RE>, 
        Assign<b, value<a, RE>, RE>
    {};
#ifdef __clang__
    static_assert(std::is_same_v<value<b, RE>, long>,  "Clang instantiates the templates depth-first, in the order they are written in the parent list");
#elif __GNUG__
    static_assert(std::is_same_v<value<b, RE>, short>, "GCC evaluates the non-recursive template first in the order they are written, then the recursive ones depth-first");
#endif

    run_line:
        Delayed<Assignment<b>>::__call__<short, RE>, 
        Delayed<Delayed<Assignment<b>>>::__call__<int, RE>,
        Delayed<Assignment<b>>::__call__<long, RE>
    {};
    static_assert(std::is_same_v<value<b, RE>, long>, "nested templates are always instantiated depth-first");

    struct d {};
    struct e {};
    
    run_line:
        Assign<d, float, RE>,
        Assign<e, value<d, RE>, RE>
    {};

    static_assert(std::is_same_v<value<e, RE>, float>, "the arguments of the second template evaluated after the first one, so the strategy is depth-first");

    struct g {};
    struct h {};

    run_line : Assign<g, int, RE> {};

    run_line:
        Delayed<Assignment<g>>:: template __call__<float, RE>,
        Assign<h, value<g, RE>, RE>
    {};

#ifdef __clang__
    static_assert(std::is_same_v<value<h, RE>, float>, "Clang starts evaluating the second template only after the first one has been instaintiated fully"); 
#elif __GNUG__
    static_assert(std::is_same_v<value<h, RE>, int>, "GCC starts instantiating the second template and its arguments before the first one");
#endif

    struct i {};
    struct j {};

    run_line : Assign<j, short, RE> {};   
    run_line : Assign<i, bool, RE> {};

    template <class T>
    struct misdirection:
        Assign<j, value<i, RE>, RE> {
    };

    run_line : 
        Assign<i, int, RE>, 
        misdirection<RE> 
    {};

    static_assert(std::is_same_v<value<j, RE>, int>, "functions aren't cached even if their arguments are discarded");

    
    struct k {};
    struct l {};

    run_line : Assign<k, void, RE>, Assign<l, void, RE> {};
    run_line : 
        Assign<k, int, RE>,
        Assign<l, value<k, RE>, RE>
    {};
    static_assert(std::is_same_v<value<k, RE>, int>);

    struct m {};

    run_line :
        Delayed<Assignment<a>>:: template __call__<Assign<m, bool, RE>, RE>,
        Assign<m, int, RE>
    {};

    static_assert(std::is_same_v<value<m, RE>, int>, "");

    struct n {};

    template <class T>
    struct simplified : Delayed<Assignment<n>>:: template __call__<float, RE> {};

    run_line : simplified<RE>, Assign<n, int, RE> {};

#ifdef __clang__
    static_assert(std::is_same_v<value<n, RE>, int>); 
#elif __GNUG__
    static_assert(std::is_same_v<value<n, RE>, float>);
#endif

    struct o {};

    template <class T>
    struct simplified2 : Assign<o, int, RE> {};

    run_line : 
        simplified2<RE>, 
        Assign<o, float, RE> 
    {};

#ifdef __clang__
    static_assert(std::is_same_v<value<o, RE>, float>); 
#elif __GNUG__
    static_assert(std::is_same_v<value<o, RE>, int>);
#endif

    struct p {};
    struct q {};

    run_line :
        Assign<p, short, RE>,
        Assign<q, Assign<q, Assign<p, long, RE>, RE>, RE>,
        Assign<q, Assign<p, int, RE>, RE>
    {};

    static_assert(std::is_same_v<value<p, RE>, int>, "the leaf templates are instantiated in order of occurrence");

    struct r {};
    struct s {};

    run_line :
        Assign<r, short, RE>,
        Assign<s, Assign<r, long, RE>, RE>,
        Assign<s, int, RE>
    {};
    static_assert(std::is_same_v<value<r, RE>, long>, "the leaf templates are instantiated in order of occurrence");
    static_assert(std::is_same_v<value<s, RE>, int>, "The assigns are instantiated in order");

    struct t {};

    run_line :
        Assign<t, short, RE>,
        Assign<t, Assign<t, long, RE>, RE>,
        Assign<t, int, RE>
    {};

    static_assert(std::is_same_v<value<t, RE>, int>, "Assign is a simple template, so it's instantiated in the order of occurrence");


};

namespace peano_order_test {
    using namespace type_var;
    // using namespace peano;

    template <class> struct __run_line {};

    struct a {};
    run_line : Assign<a, peano::Zero, RE> {};

    run_line :
        Assign<a, peano::Succ<value<a, RE>>, RE>,
        Assign<a, peano::Succ<value<a, RE>>, RE>
    {};

    static_assert(std::is_same_v<value<a, RE>, peano::Two>, "Succ is a simple template, so they're instantiated in the order of occurrence");

    run_line : Assign<a, peano::One, RE> {};

    run_line :
        Assign<a, peano::add<value<a, RE>, peano::Two>, RE>,
        Assign<a, peano::Succ<value<a, RE>>, RE>
    {};
    static_assert(std::is_same_v<value<a, RE>, peano::Four>);
    
    run_line : Assign<a, peano::One, RE> {};
    run_line :
        Assign<a, peano::mult<value<a, RE>, peano::Two>, RE>,
        Assign<a, peano::Succ<value<a, RE>>, RE>
    {};
    static_assert(std::is_same_v<value<a, RE>, peano::Three>);

    struct b{};
    struct c{};
    run_line : 
        Assign<a, peano::One, RE>, 
        Assign<b, peano::One, RE>,
        Assign<c, ctstd::True, RE>
    {};

    run_line :            
            Assign<a, 
                peano::Succ<value<a, RE>>, RE
            >,
            Assign<b,
                peano::add<value<b, RE>, value<a, RE>>, RE 
            >,
            Assign<c,
                peano::leq<value<a, RE>, peano::One>, RE
            >
    {};
    static_assert(std::is_same_v<value<a, RE>, peano::Two>);
    static_assert(std::is_same_v<value<b, RE>, peano::Three>);
    static_assert(std::is_same_v<value<c, RE>, ctstd::False>);

    run_line : 
        Assign<a, peano::One, RE>, 
        Assign<b, peano::One, RE>,
        Assign<c, ctstd::True, RE>
    {};

    template <class _>
    struct folded :            
            Assign<a, 
                peano::Succ<value<a, RE>>, RE
            >,
            Assign<b,
                peano::add<value<b, RE>, value<a, RE>>, RE 
            >,
            Assign<c,
                peano::leq<value<a, RE>, peano::One>, RE
            >
    {};

    run_line : folded<RE> {};

    static_assert(std::is_same_v<value<a, RE>, peano::Two>);
    static_assert(std::is_same_v<value<b, RE>, peano::Three>);
    static_assert(std::is_same_v<value<c, RE>, ctstd::False>);

    run_line : 
        Assign<a, peano::One, RE>, 
        Assign<b, peano::One, RE>,
        Assign<c, ctstd::True, RE>
    {};

    struct lambda_folded {
        template <class _>
        struct __call__ : 
            Assign<a, 
                peano::Succ<value<a, RE>>, RE
            >,
            Assign<b,
                peano::add<value<b, RE>, value<a, RE>>, RE 
            >,
            Assign<c,
                peano::leq<value<a, RE>, peano::One>, RE
            >
        {};
    };

    run_line : 
        lambda_folded:: template __call__<RE>
    {};

    static_assert(std::is_same_v<value<a, RE>, peano::Two>);
    static_assert(std::is_same_v<value<b, RE>, peano::Three>);
    static_assert(std::is_same_v<value<c, RE>, ctstd::False>);
};

namespace using_order_test {
    using namespace type_var;
    using namespace cexpr_control;

    template <class> struct __run_line {};


    struct a {};
    run_line {
        using _1 = Assign<a, long, RE>;
        using _2 = Assign<a, int, RE>;
    };
    static_assert(std::is_same_v<value<a, RE>, int>, "templates in using declarations are instantiated in order that they appear");


    struct b {};
    run_line {
        using _2 = Assign<b, int, RE>;
        using _1 = Delayed<Assignment<b>>:: template __call__<long, decltype([](){})>;
    };
    static_assert(std::is_same_v<value<b, RE>, int>, "the order of template instantiation doesn't depend on the complexity of the templates");

    struct c {};
    struct d {};

    run_line {
        template <class _> using _1 = Assign<c, int, _>;

        using _2 = Assign<d, value<c, RE>, RE>;
        using _3 = _1<RE>;
    };
    static_assert(std::is_same_v<value<d, RE>, ctstd::None>, "templates aren't instantiated until they are used");


    struct e{};

    template <class T>
    struct InnerLoop {
        using _1 = Assign<T, int, RE>;
    };

    template <class T>
    struct OuterLoop : InnerLoop<T> {
        using _1 = Assign<T, long, RE>;
    };
    run_line : OuterLoop<e> {};
    static_assert(std::is_same_v<value<e, RE>, long>, "Using declarations are instantiated at the same time as the class they're in");

};

namespace memberfunction_order_test {
    using namespace type_var;
    using namespace cexpr_control;
    template <class> struct __run_line {};


    struct a {};
    run_line {
        auto _1() { return Assign<a, int, RE>();};
        auto _2() { return Assign<a, long, RE>();};
    };
    static_assert(std::is_same_v<value<a, RE>, long>);

    struct b {};
    run_line {
        auto _1() { return Delayed<Assignment<b>>:: template __call__<float, RE>();};
        auto _2() { return Assign<b, long, RE>();};
    };
    static_assert(std::is_same_v<value<a, RE>, long>);

    struct c {};
    run_line {
        auto _1(Assign<c, long, RE> _) { return Assign<c, int, RE>();};
    };
    static_assert(std::is_same_v<value<c, RE>, int>);

    struct d {};
    run_line {
        auto _1(
            Delayed<Assignment<d>>:: template __call__<float, RE> _
        ) { 
            auto todiscard = Delayed<Assignment<d>>:: template __call__<long, RE>();
            return Assign<d, int, RE>();
        };
    };
    static_assert(std::is_same_v<value<d, RE>, int>);

};

namespace mixed_order_test {
    using namespace type_var;
    using namespace cexpr_control;

    template <class> struct __run_line {};

    struct a {};
    run_line : Assign<a, float, RE> {
        using _1 = Assign<a, bool, RE>;
    };
    static_assert(std::is_same_v<value<a, RE>, bool>);

    struct b {};
    run_line : Assign<b, short, RE> {
        using _1 = Assign<b, int, RE>;
        using _2 = Delayed<Assignment<b>>:: template __call__<long, decltype([](){})>;

    };
    static_assert(std::is_same_v<value<b, RE>, int>);

    struct c {};
    run_line : Delayed<Assignment<c>>:: template __call__<long, decltype([](){})> {
        using _1 = Assign<c, int, RE>;
        auto _2() {
            return Assign<c, float, RE>();
        }

    };
    static_assert(std::is_same_v<value<c, RE>, float>);


    struct d{};
    struct e{};

    run_line : Assign<d, void, RE>, Assign<e, void, RE> {};

    template <class T, class U>
    struct recurse : Assign<T, int, RE> {
        using _1 = Assign<T, float, RE>;
    };

    run_line : recurse<d, decltype([](){})> {
        using _2 = Assign<e, value<d, RE>, RE>;
    };

    static_assert(std::is_same_v<value<e, RE>, float>);

    struct f {};

    run_line {
        using _1 = Delayed<Assignment<f>>:: template __call__<long, decltype([](){})> ;
        auto _2() {
            return Assign<f, float, RE>();
        }

    };
    static_assert(std::is_same_v<value<f, RE>, float>); 

};

namespace recursion_test_1 {
    using namespace type_var;
    using namespace cexpr_control;

    template <class> struct __run_line {};

    struct a {};
    struct b {};
    struct c {};
    struct loop_cnt {};

    run_line : Assign<a, peano::Zero, RE> {};
    run_line : Assign<b, peano::Zero, RE> {};
    run_line : Assign<c, ctstd::True, RE> {};
    run_line : Assign<loop_cnt, peano::Zero, RE> {};


    template <int N>
    struct Wrap {
        static constexpr int val = N;
    };


    template <class _>
    struct SumOfFirstNIntegers :
            Assign<a, 
                peano::Succ<value<a, RE>>, RE
            >,
            Assign<c,
                peano::leq<value<a, RE>, peano::Three>, RE
            >
    {
    };

        template <
        class spec_cond = ctstd::True, 
        class _ = decltype([](){}), unsigned N = 0, class print=void
    >
    struct DoWhileI;

    template <
        class spec_cond, 
        class _, unsigned N, class print
    >
    struct DoWhileI : SumOfFirstNIntegers<
                        ::cexpr_control::detail::Pair<
                            ::cexpr_control::detail::WrapInt<N>, _>
                        >, DoWhileI<
                    type_var::value<c, RE>, 
                    _, N+1, value<a, RE>
                >, Assign<loop_cnt, peano::Integer<N>, _>
        {    
        };
    
    template <
        class _, unsigned N, class print
    >
    struct DoWhileI<ctstd::False, _, N, print> {
        using next_iteration = ctstd::None;
    };

    run_line: DoWhileI<> {};


#ifdef __clang__
    static_assert(std::is_same_v<value<a, RE>, peano::Four>);
#elif __GNUG__
    static_assert(std::is_same_v<value<a, RE>, peano::Five>);
#endif
}

namespace recursion_test_2 {
    using namespace type_var;
    using namespace cexpr_control;

    template <class> struct __run_line {};

    struct a {};
    struct b {};
    struct c {};
    run_line : Assign<a, peano::Zero, RE> {};
    run_line : Assign<b, peano::Zero, RE> {};
    run_line : Assign<c, ctstd::True, RE> {};


    struct SumOfFirstIntegers {
        template <class _>
        struct __call__ : 
            Assign<a, 
                peano::Succ<value<a, RE>>, RE
            >,
            Assign<b,
                peano::add<value<b, RE>, value<a, RE>>, RE 
            >,
            Assign<c,
                peano::leq<value<a, RE>, peano::Five>, RE
            >
        {};
    };

    run_line : ::cexpr_control::DoWhile<SumOfFirstIntegers, c, RE> {};

#ifdef __clang__
    static_assert(std::is_same_v<value<b, RE>, peano::Integer<21>>);
#elif __GNUG__
    static_assert(std::is_same_v<value<b, RE>, peano::Integer<28>>);
#endif
}

namespace recursion_test_delayed {
    using namespace type_var;
    using namespace cexpr_control;

    template <class> struct __run_line {};

    struct a {};
    struct b {};
    struct c {};
    struct d {};
    run_line : Assign<a, peano::Zero, RE> {};
    run_line : Assign<b, peano::Zero, RE> {};
    run_line : Assign<c, ctstd::True, RE> {};


    struct SumOfFirstIntegers {
        template <class _>
        struct __call__ : 
            Assign<a, 
                peano::Succ<value<a, RE>>, RE
            >,
            Assign<b,
                peano::add<value<b, RE>, value<a, RE>>, RE 
            >,
            Assign<c,
                peano::leq<value<a, RE>, peano::Five>, RE
            >
        {};
    };

    run_line : ::cexpr_control::DoWhile<Delayed<Delayed<SumOfFirstIntegers>>, c, RE> {};

#ifdef __clang__
    static_assert(std::is_same_v<value<b, RE>, peano::Integer<21>>);
#elif __GNUG__
    static_assert(std::is_same_v<value<b, RE>, peano::Integer<28>>);
#endif
}


namespace big_recursion_test {
    using namespace type_var;
    using namespace cexpr_control;

    template <class> struct __run_line {};

    struct a {};
    struct b {};
    struct c {};

    run_line: Assign<a, peano::Seven, RE> {};
    // 5 > 16 > 8 > 4 > 2 > 1
    // 7 > 22 > 11 > 34 > 17 > 52 > 26 > 13 > 40 > 20 > 10 > 5 > 16 > 8 > 4 > 2 > 1
    run_line: Assign<b, peano::Zero, RE> {};
    run_line: Assign<c, ctstd::True, RE> {};

    struct CollatzStep {
        template <class _>
        struct __call__ :
            if_else<
                ctstd::is_same<
                    peano::remainder<
                        value<a, RE>, peano::Two
                    >, 
                    peano::Zero
                >,
                Assignment<a>,
                peano::div<
                    value<a, RE>, peano::Two
                >,
                peano::Succ<peano::mult<
                    value<a, RE>, peano::Three
                >>,
                RE
            >,
            Assign<b, peano::Succ<value<b, RE>>, RE>,
            Assign<c, ctstd::Not<ctstd::is_same<value<a, RE>, peano::One>>, RE>
        {};
    };

    run_line: ::cexpr_control::DoWhile<CollatzStep, c, RE> {};

#ifdef __clang__
    static_assert(std::is_same_v<value<a, RE>, peano::Integer<1>>);
    static_assert(std::is_same_v<value<b, RE>, peano::Integer<16>>);
#elif __GNUG__
    static_assert(std::is_same_v<value<a, RE>, peano::Integer<2>>);
    static_assert(std::is_same_v<value<b, RE>, peano::Integer<18>>);
#endif
};


namespace random_fun_tests {
    using namespace type_var;
    using namespace cexpr_control;

    template <class> struct __run_line {};

    struct a {};
    struct b {};
    run_line: Assign<b, bool, RE> {};
    
    template <class T> struct Extractable {
        using extract = T;
    };
    template <class T> struct Func : Extractable<RE>, Assign<a, value<b, RE>, RE> {    };

    static_assert(std::is_same_v<value<a, RE>, ctstd::None>);
    run_line: Func<RE> {};
    static_assert(std::is_same_v<value<a, RE>, bool>);

    run_line: Assign<b, int, RE> {};
    run_line: Func<RE> {};
    static_assert(std::is_same_v<value<a, RE>, int>);
};