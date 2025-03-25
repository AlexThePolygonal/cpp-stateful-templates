#include "cexpr_lib.hpp"
#include "peano.hpp"
#include <iostream>
#include <array>

#define run_line template <> struct __run_line<decltype([](){})>
#define RE decltype([](){})

// here we are going to test whether the assignment mechanism works well
namespace basic_tests {
    using namespace type_var;
    template <class> struct __run_line {};

    struct a {};
    static_assert(std::is_same_v<value<a, RE>, ctstd::None>, "the value of initialized variable must be None");

    run_line : Assign<a, a, RE> {};
    static_assert(std::is_same_v<value<value<value<a, RE>, RE>, RE>, a>, "the value of a variable can be the variable name itself");

    run_line : Assign<a, void, RE> {};
    static_assert(std::is_same_v<value<a, RE>, void>, "the value of a variable can be void");
    
    run_line : Assign<a, const int, RE> {};
    static_assert(std::is_same_v<value<a, RE>, const int>, "the value of a variable can be const int");

    run_line : Assign<a, int&, RE> {};
    static_assert(std::is_same_v<value<a, RE>, int&>);

    run_line : Assign<a, Assign<a, unsigned, RE>, RE> {};
    static_assert(!std::is_same_v<value<a, RE>, unsigned>);

    run_line : Assign<a, int, RE> {};
    run_line : Assign<a, float, RE> {};
    run_line : Assign<a, int, RE> {};

    static_assert(std::is_same_v<value<a, RE>, int>, "");

    struct b {};
    static_assert(std::is_same_v<value<b, RE>, ctstd::None>);

    struct c {};
    run_line : Assign<c, void, RE> {};
};

namespace cexpr_control_tests {
    using namespace ctstd;
    using namespace type_var;
    using namespace cexpr_control;

    template <class> struct __run_line {};

    struct a {};

    run_line : if_<True, Assignment<a>, float, RE> {};
    run_line : if_<True, Assignment<a>, double, RE> {};
    run_line : if_<True, Assignment<a>, float, RE> {};

    static_assert(std::is_same_v<value<a, RE>, float>);

    struct b {};

    run_line : if_else<True, Assignment<b>, float, bool, RE> {};
    run_line : if_else<True, Assignment<b>, double, bool, RE> {};
    run_line : if_else<True, Assignment<b>, float, bool, RE> {};

    static_assert(std::is_same_v<value<b, RE>, float>);

    struct c {};
    struct d {};

};

namespace inheritance_order_tests {
    using namespace ctstd;
    using namespace type_var;
    using namespace cexpr_control;

    template <class> struct __run_line {};

    struct a {};
    run_line : Assign<a, unsigned, RE>, Assign<a, int, RE> {};
    static_assert(std::is_same_v<value<a, RE>, int>);

    run_line: 
        Assign<a, short, RE>, 
        if_<True, Assignment<a>, int, RE>, 
        Assign<a, long, RE> 
    {};
#ifdef __clang__
    static_assert(std::is_same_v<value<a, RE>, long>, "Clang instantiates the templates depth-first");
#elif __GNUG__
    static_assert(std::is_same_v<value<a, RE>, int>, "GCC short-circuits the instantiation of non-folded templates");
#endif

    struct b{};
    run_line: 
        if_<True, Assignment<b>, short, RE>, 
        if_<True, Delayed<Assignment<b>>, int, RE>, 
        Assign<b, long, RE>
    {};
#ifdef __clang__
    static_assert(std::is_same_v<value<b, RE>, long>);
#elif __GNUG__
    static_assert(std::is_same_v<value<b, RE>, int>);
#endif

    run_line:
        Delayed<Assignment<b>>::call<short, decltype([](){})>, 
        Delayed<Delayed<Assignment<b>>>::call<int, decltype([](){})>,
        Delayed<Assignment<b>>::call<long, decltype([](){})>
    {};
    static_assert(std::is_same_v<value<b, RE>, long>);

    struct d {};
    struct e {};
    
    run_line:
        Assign<d, float, RE>,
        Assign<e, value<d, RE>, RE>
    {};

    static_assert(std::is_same_v<value<e, RE>, float>);

    struct g {};
    struct h {};

    run_line:
        Delayed<Assignment<g>>:: template call<float, RE>,
        Assign<h, value<g, RE>, RE>
    {};

    static_assert(std::is_same_v<value<e, RE>, float>);

    struct i {};
    struct j {};

    run_line : Assign<j, short, RE> {};   
    run_line : Assign<i, bool, RE> {};

    template <class T>
    struct misdirection:
        Assign<j, value<i, RE>, RE> {
    };

    run_line : Assign<i, int, RE> {};
    run_line : misdirection<RE> {};

    static_assert(std::is_same_v<value<j, RE>, int>);

    
    struct j1 {};
    struct i1 {};
    run_line : Assign<i1, short, RE> {};
    run_line : Assign<j1, value<i1, RE>, RE> {};

    static_assert(std::is_same_v<value<j1, RE>, short>);

    struct k {};
    struct l {};

    run_line : Assign<k, void, RE>, Assign<l, void, RE> {};
    run_line : 
        Assign<k, int, RE>,
        Assign<l, value<k, RE>, RE>
    {};
    static_assert(std::is_same_v<value<k, RE>, int>);
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
    static_assert(std::is_same_v<value<a, RE>, int>);


    struct b {};
    run_line {
        using _2 = Assign<b, int, RE>;
        using _1 = Delayed<Assignment<b>>:: template call<long, decltype([](){})>;
    };
    static_assert(std::is_same_v<value<b, RE>, int>);

    struct c {};
    struct d {};

    run_line {
        template <class _> using _1 = Assign<c, int, _>;

        using _2 = Assign<d, value<c, RE>, RE>;
        using _3 = _1<RE>;
    };
    static_assert(std::is_same_v<value<d, RE>, ctstd::None>, "the order of the using commands is correct");


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
    static_assert(std::is_same_v<value<e, RE>, long>, "using commands are executed from the inside out");

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
        auto _1() { return Delayed<Assignment<b>>:: template call<float, decltype([](){})>();};
        auto _2() { return Assign<b, long, RE>();};
    };
    static_assert(std::is_same_v<value<a, RE>, long>);

    struct c{};
    run_line {};
    
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
        using _2 = Delayed<Assignment<b>>:: template call<long, decltype([](){})>;

    };
    static_assert(std::is_same_v<value<b, RE>, int>);

    struct c {};
    run_line : Delayed<Assignment<c>>:: template call<long, decltype([](){})> {
        using _1 = Assign<c, int, RE>;

    };
    static_assert(std::is_same_v<value<c, RE>, int>);


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
};

namespace recursion_tests {
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
    struct DoWhileI : SumOfFirstNIntegers<RE>, DoWhileI<
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

    // Here GCC short-circuits the evaluations of value<a> inside the SumOfFirstNIntegers
    // While good boy Clang doesn't
    static_assert(std::is_same_v<value<a, RE>, peano::Four>);
}

int main() {
    // std::cout << typeid(type_var::value<recursion_tests::a, RE>).name() << std::endl;
}