#include "cexpr_lib.hpp"
#include "peano.hpp"
#include <iostream>
#include <array>

#define run_line template <> struct __run_line<decltype([](){})>
#define VALUE(T) type_var::value<T, decltype([](){})>

// here we are going to test whether the assignment mechanism works well
namespace basic_tests {
    using namespace type_var;
    template <class> struct __run_line {};

    struct a {};
    static_assert(std::is_same_v<value<a>, ctstd::None>, "the value of initialized variable must be None");

    run_line : Assign<a, a> {};
    static_assert(std::is_same_v<value<value<value<a>>>, a>, "the value of a variable can be the variable name itself");

    run_line : Assign<a, void> {};
    static_assert(std::is_same_v<value<a>, void>, "the value of a variable can be void");
    
    run_line : Assign<a, const int> {};
    static_assert(std::is_same_v<value<a>, const int>, "the value of a variable can be const int");

    run_line : Assign<a, int&> {};
    static_assert(std::is_same_v<value<a>, int&>);

    run_line : Assign<a, Assign<a, unsigned>> {};
    static_assert(!std::is_same_v<value<a>, unsigned>);

    run_line : Assign<a, int> {};
    run_line : Assign<a, float> {};
    run_line : Assign<a, int> {};

    static_assert(std::is_same_v<value<a>, int>, "");

    struct b {};
    static_assert(std::is_same_v<value<b>, ctstd::None>);

    struct c {};
    run_line : Assign<c, void> {};
};

namespace cexpr_control_tests {
    using namespace ctstd;
    using namespace type_var;
    using namespace cexpr_control;

    template <class> struct __run_line {};

    struct a {};

    run_line : if_<True, Assignment<a>, float> {};
    run_line : if_<True, Assignment<a>, double> {};
    run_line : if_<True, Assignment<a>, float> {};

    static_assert(std::is_same_v<value<a>, float>);

    struct b {};

    run_line : if_else<True, Assignment<b>, float, bool> {};
    run_line : if_else<True, Assignment<b>, double, bool> {};
    run_line : if_else<True, Assignment<b>, float, bool> {};

    static_assert(std::is_same_v<value<b>, float>);

    struct c {};
    struct d {};

};

namespace inheritance_order_tests {
    using namespace ctstd;
    using namespace type_var;
    using namespace cexpr_control;

    template <class> struct __run_line {};

    struct a {};
    run_line : Assign<a, unsigned>, Assign<a, int> {};
    static_assert(std::is_same_v<value<a>, int>);

    run_line: 
        Assign<a, short>, 
        if_<True, Assignment<a>, int>, 
        Assign<a, long> 
    {};
#ifdef __clang__
    static_assert(std::is_same_v<value<a>, long>, "Clang instantiates the templates depth-first");
#elif __GNUG__
    static_assert(std::is_same_v<value<a>, int>, "GCC short-circuits the instantiation of non-folded templates");
#endif

    struct b{};
    run_line: 
        if_<True, Assignment<b>, short>, 
        if_<True, Delayed<Assignment<b>>, int>, 
        Assign<b, long>
    {};
#ifdef __clang__
    static_assert(std::is_same_v<value<b>, long>);
#elif __GNUG__
    static_assert(std::is_same_v<value<b>, int>);
#endif

    run_line:
        Delayed<Assignment<b>>::call<short, decltype([](){})>, 
        Delayed<Delayed<Assignment<b>>>::call<int, decltype([](){})>,
        Delayed<Assignment<b>>::call<long, decltype([](){})>
    {};
    static_assert(std::is_same_v<value<b>, long>);

    struct d {};
    struct e {};
    
    run_line:
        Assign<d, float>,
        Assign<e, value<d>>
    {};

    static_assert(std::is_same_v<value<e>, float>);

    struct g {};
    struct h {};

    run_line:
        Delayed<Assignment<g>>:: template call<float, decltype([](){})>,
        Assign<h, value<g>>
    {};

    static_assert(std::is_same_v<value<e>, float>);

    struct i {};
    struct j {};

    run_line : Assign<j, short> {};   
    run_line : Assign<i, bool> {};

    template <class T>
    struct misdirection:
        Assign<j, value<i, decltype([](){})>, decltype([](){})> {
    };

    run_line : Assign<i, int> {};
    run_line : misdirection<decltype([](){})> {};

    static_assert(std::is_same_v<value<j>, int>);

    
    struct j1 {};
    struct i1 {};
    run_line : Assign<i1, short> {};
    run_line : Assign<j1, value<i1>> {};

    static_assert(std::is_same_v<value<j1>, short>);

    struct k {};
    struct l {};

    run_line : Assign<k, void>, Assign<l, void> {};
    run_line : 
        Assign<k, int>,
        Assign<l, value<k>>
    {};
    static_assert(std::is_same_v<value<k>, int>);
};

namespace using_order_test {
    using namespace type_var;
    using namespace cexpr_control;

    template <class> struct __run_line {};


    struct a {};
    run_line {
        using _1 = Assign<a, long>;
        using _2 = Assign<a, int>;
    };
    static_assert(std::is_same_v<value<a>, int>);


    struct b {};
    run_line {
        using _2 = Assign<b, int>;
        using _1 = Delayed<Assignment<b>>:: template call<long, decltype([](){})>;
    };
    static_assert(std::is_same_v<value<b>, int>);

    struct c {};
    struct d {};

    run_line {
        template <class T> using _1 = Assign<c, int, T>;

        using _2 = Assign<d, value<c>>;
        using _3 = _1<decltype([](){})>;
    };
    static_assert(std::is_same_v<value<d>, ctstd::None>, "the order of the using commands is correct");


    struct e{};

    template <class T>
    struct InnerLoop {
        using _1 = Assign<T, int>;
    };

    template <class T>
    struct OuterLoop : InnerLoop<T> {
        using _1 = Assign<T, long>;
    };
    run_line : OuterLoop<e> {};
    static_assert(std::is_same_v<value<e>, long>, "using commands are executed from the inside out");

};

namespace memberfunction_order_test {
    using namespace type_var;
    using namespace cexpr_control;
    template <class> struct __run_line {};


    struct a {};
    run_line {
        auto _1() { return Assign<a, int>();};
        auto _2() { return Assign<a, long>();};
    };
    static_assert(std::is_same_v<value<a>, long>);

    struct b {};
    run_line {
        auto _1() { return Delayed<Assignment<b>>:: template call<float, decltype([](){})>();};
        auto _2() { return Assign<b, long>();};
    };
    static_assert(std::is_same_v<value<a>, long>);

    struct c{};
    run_line {};
    
};

namespace mixed_order_test {
    using namespace type_var;
    using namespace cexpr_control;

    template <class> struct __run_line {};

    struct a {};
    run_line : Assign<a, float> {
        using _1 = Assign<a, bool>;
    };
    static_assert(std::is_same_v<value<a>, bool>);

    struct b {};
    run_line : Assign<b, short> {
        using _1 = Assign<b, int>;
        using _2 = Delayed<Assignment<b>>:: template call<long, decltype([](){})>;

    };
    static_assert(std::is_same_v<value<b>, int>);

    struct c {};
    run_line : Delayed<Assignment<c>>:: template call<long, decltype([](){})> {
        using _1 = Assign<c, int>;

    };
    static_assert(std::is_same_v<value<c>, int>);


    struct d{};
    struct e{};

    run_line : Assign<d, void>, Assign<e, void> {};

    template <class T, class U>
    struct recurse : Assign<T, int> {
        using _1 = Assign<T, float>;
    };

    run_line : recurse<d, decltype([](){})> {
        using _2 = Assign<e, value<d>>;
    };

    static_assert(std::is_same_v<value<e>, float>);

};

namespace recursion_tests {
    using namespace type_var;
    using namespace cexpr_control;

    template <class> struct __run_line {};

    struct a {};
    struct b {};
    struct c {};
    struct loop_cnt {};

    run_line : Assign<a, peano::Zero> {};
    run_line : Assign<b, peano::Zero> {};
    run_line : Assign<c, ctstd::True> {};
    run_line : Assign<loop_cnt, peano::Zero> {};


    template <int N>
    struct Wrap {
        static constexpr int val = N;
    };


    template <class _ = decltype([](){})>
    struct SumOfFirstNIntegers :
            Assign<a, 
                peano::Succ<value<a, decltype([](){})>>, _
            >,
            Assign<c,
                peano::leq<value<a, _>, peano::One>, _
            >
    {
        using tp = std::array<int, _::val>;
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
    struct DoWhileI : SumOfFirstNIntegers<Wrap<N>>::tp, DoWhileI<
                    type_var::value<c, decltype([](){})>, 
                    _, N+1, value<a, decltype([](){})>
                >, Assign<loop_cnt, decltype([](){}), _>
        {    
        };
    
    template <
        class _, unsigned N, class print
    >
    struct DoWhileI<ctstd::False, _, N, print> {
        using next_iteration = ctstd::None;
    };

    run_line: DoWhileI<> {};


    static_assert(std::is_same_v<value<a>, peano::Four>);
};

int main() {
    // std::cout << typeid(type_var::value<recursion_tests::b>).name() << std::endl;
}