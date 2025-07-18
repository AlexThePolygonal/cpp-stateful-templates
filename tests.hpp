#include "type_var.hpp"
#include "cexpr_control.hpp"
#include "peano.hpp"
#include "ctstd.hpp"
#include <iostream>
#include <array>


#define run_line template <> struct __run_line<decltype([](){})>
#define RE decltype([](){})

namespace prereqs {
    template <class _ = decltype([](){})>
    struct T {};

    static_assert(!std::is_same_v<T<>, T<>>, "test");

    struct a {};
    struct b : a {};
    struct c : b {};
    struct d : a {};
    template <class T, class U>
    struct is_base_of__boolean {
        static constexpr bool value = std::is_same_v<ctstd::is_base_of<T, U>, ctstd::True>;
    };
    static_assert(is_base_of__boolean<a, b>::value, "b is derived from a");
    static_assert(is_base_of__boolean<b, c>::value, "c is derived from b");
    static_assert(is_base_of__boolean<a, d>::value, "d is derived from a");
    static_assert(is_base_of__boolean<b, b>::value, "b is derived from b");
    static_assert(!is_base_of__boolean<c, b>::value, "b isn't derived from c");
    static_assert(!is_base_of__boolean<d, b>::value, "b isn't derived from d");
};

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
    run_line : Assign<a, peano::_0, RE> {};

    run_line :
        Assign<a, peano::Succ<value<a, RE>>, RE>,
        Assign<a, peano::Succ<value<a, RE>>, RE>
    {};

    static_assert(std::is_same_v<value<a, RE>, peano::_2>, "Succ is a simple template, so they're instantiated in the order of occurrence");

    run_line : Assign<a, peano::_1, RE> {};

    run_line :
        Assign<a, peano::add<value<a, RE>, peano::_2>, RE>,
        Assign<a, peano::Succ<value<a, RE>>, RE>
    {};
    static_assert(std::is_same_v<value<a, RE>, peano::_4>);
    
    run_line : Assign<a, peano::_1, RE> {};
    run_line :
        Assign<a, peano::mult<value<a, RE>, peano::_2>, RE>,
        Assign<a, peano::Succ<value<a, RE>>, RE>
    {};
    static_assert(std::is_same_v<value<a, RE>, peano::_3>);

    struct b{};
    struct c{};
    run_line : 
        Assign<a, peano::_1, RE>, 
        Assign<b, peano::_1, RE>,
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
                peano::leq<value<a, RE>, peano::_1>, RE
            >
    {};
    static_assert(std::is_same_v<value<a, RE>, peano::_2>);
    static_assert(std::is_same_v<value<b, RE>, peano::_3>);
    static_assert(std::is_same_v<value<c, RE>, ctstd::False>);

    run_line : 
        Assign<a, peano::_1, RE>, 
        Assign<b, peano::_1, RE>,
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
                peano::leq<value<a, RE>, peano::_1>, RE
            >
    {};

    run_line : folded<RE> {};

    static_assert(std::is_same_v<value<a, RE>, peano::_2>);
    static_assert(std::is_same_v<value<b, RE>, peano::_3>);
    static_assert(std::is_same_v<value<c, RE>, ctstd::False>);

    run_line : 
        Assign<a, peano::_1, RE>, 
        Assign<b, peano::_1, RE>,
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
                peano::leq<value<a, RE>, peano::_1>, RE
            >
        {};
    };

    run_line : 
        lambda_folded:: template __call__<RE>
    {};

    static_assert(std::is_same_v<value<a, RE>, peano::_2>);
    static_assert(std::is_same_v<value<b, RE>, peano::_3>);
    static_assert(std::is_same_v<value<c, RE>, ctstd::False>);
};

namespace ctstd_adv_test {
    using namespace type_var;
    using namespace cexpr_control;
    using namespace ctstd;

    template <class> struct __run_line {};

    // Test variables for all arithmetic and comparison operations
    struct a {};
    struct b {};
    struct c {};
    struct d {};

    // Initialize test variables with different values
    run_line : Assign<a, _2, RE> {};
    run_line : Assign<b, _3, RE> {};
    run_line : Assign<c, _5, RE> {};
    run_line : Assign<d, _0, RE> {};

    // Test mult function
    static_assert(std::is_same_v<mult<a, _2, RE>, _4>, "multiplying variable by constant works correctly");
    static_assert(std::is_same_v<mult<_2, a, RE>, _4>, "multiplying constant by variable works correctly");
    static_assert(std::is_same_v<mult<_2, _3, RE>, _6>, "multiplying constants works correctly");
    static_assert(std::is_same_v<mult<a, b, RE>, _6>, "multiplying two variables works correctly");
    static_assert(std::is_same_v<mult<a, d, RE>, _0>, "multiplying by zero works correctly");
    static_assert(std::is_same_v<mult<d, a, RE>, _0>, "zero times variable works correctly");

    // Test divid function
    static_assert(std::is_same_v<divide<c, a, RE>, _2>, "dividing variable by variable works correctly");
    static_assert(std::is_same_v<divide<c, _2, RE>, _2>, "dividing variable by constant works correctly");
    static_assert(std::is_same_v<divide<_6, a, RE>, _3>, "dividing constant by variable works correctly");
    static_assert(std::is_same_v<divide<_6, _3, RE>, _2>, "dividing constants works correctly");
    static_assert(std::is_same_v<divide<a, a, RE>, _1>, "dividing variable by itself works correctly");
    static_assert(std::is_same_v<divide<d, _1, RE>, _0>, "dividing zero by constant works correctly");

    // Test remainder function
    static_assert(std::is_same_v<remainder<c, a, RE>, _1>, "remainder of variable by variable works correctly");
    static_assert(std::is_same_v<remainder<c, _2, RE>, _1>, "remainder of variable by constant works correctly");
    static_assert(std::is_same_v<remainder<_7, a, RE>, _1>, "remainder of constant by variable works correctly");
    static_assert(std::is_same_v<remainder<_7, _3, RE>, _1>, "remainder of constants works correctly");
    static_assert(std::is_same_v<remainder<a, a, RE>, _0>, "remainder of variable by itself is zero");
    static_assert(std::is_same_v<remainder<d, _1, RE>, _0>, "remainder of zero by constant is zero");

    // Test minus function
    static_assert(std::is_same_v<minus<c, a, RE>, _3>, "subtracting variable from variable works correctly");
    static_assert(std::is_same_v<minus<c, _2, RE>, _3>, "subtracting constant from variable works correctly");
    static_assert(std::is_same_v<minus<_5, a, RE>, _3>, "subtracting variable from constant works correctly");
    static_assert(std::is_same_v<minus<_5, _2, RE>, _3>, "subtracting constants works correctly");
    static_assert(std::is_same_v<minus<a, a, RE>, _0>, "subtracting variable from itself gives zero");
    static_assert(std::is_same_v<minus<a, d, RE>, _2>, "subtracting zero from variable gives the variable");

    // Test leq (less than or equal) function
    static_assert(std::is_same_v<leq<a, b, RE>, True>, "leq of smaller variable to larger variable is True");
    static_assert(std::is_same_v<leq<b, a, RE>, False>, "leq of larger variable to smaller variable is False");
    static_assert(std::is_same_v<leq<a, _2, RE>, True>, "leq of variable to equal constant is True");
    static_assert(std::is_same_v<leq<a, _1, RE>, False>, "leq of variable to smaller constant is False");
    static_assert(std::is_same_v<leq<_2, a, RE>, True>, "leq of constant to equal variable is True");
    static_assert(std::is_same_v<leq<_3, a, RE>, False>, "leq of larger constant to variable is False");
    static_assert(std::is_same_v<leq<_2, _3, RE>, True>, "leq of smaller constant to larger constant is True");
    static_assert(std::is_same_v<leq<_3, _2, RE>, False>, "leq of larger constant to smaller constant is False");
    static_assert(std::is_same_v<leq<_2, _2, RE>, True>, "leq of constant to itself is True");
    static_assert(std::is_same_v<leq<a, a, RE>, True>, "leq of variable to itself is True");
    static_assert(std::is_same_v<leq<d, a, RE>, True>, "leq of zero to positive variable is True");
    static_assert(std::is_same_v<leq<a, d, RE>, False>, "leq of positive variable to zero is False");

    // Test eq (equality) function
    static_assert(std::is_same_v<eq<a, a, RE>, True>, "eq of variable to itself is True");
    static_assert(std::is_same_v<eq<a, b, RE>, False>, "eq of different variables is False");
    static_assert(std::is_same_v<eq<a, _2, RE>, True>, "eq of variable to equal constant is True");
    static_assert(std::is_same_v<eq<a, _3, RE>, False>, "eq of variable to different constant is False");
    static_assert(std::is_same_v<eq<_2, a, RE>, True>, "eq of constant to equal variable is True");
    static_assert(std::is_same_v<eq<_3, a, RE>, False>, "eq of constant to different variable is False");
    static_assert(std::is_same_v<eq<_2, _2, RE>, True>, "eq of constant to itself is True");
    static_assert(std::is_same_v<eq<_2, _3, RE>, False>, "eq of different constants is False");
    static_assert(std::is_same_v<eq<b, c, RE>, False>, "eq of different variables with different values is False");
    static_assert(std::is_same_v<eq<d, _0, RE>, True>, "eq of zero variable to zero constant is True");
    static_assert(std::is_same_v<eq<_0, d, RE>, True>, "eq of zero constant to zero variable is True");
    static_assert(std::is_same_v<eq<d, d, RE>, True>, "eq of zero variable to itself is True");

    // Test is_same function (from ctstd_base.hpp)
    static_assert(std::is_same_v<is_same<a, a>, True>, "is_same of variable to itself is True");
    static_assert(std::is_same_v<is_same<a, b>, False>, "is_same of different variables is False");
    static_assert(std::is_same_v<is_same<_2, _2>, True>, "is_same of constant to itself is True");
    static_assert(std::is_same_v<is_same<_2, _3>, False>, "is_same of different constants is False");
    static_assert(std::is_same_v<is_same<True, True>, True>, "is_same of True to True is True");
    static_assert(std::is_same_v<is_same<True, False>, False>, "is_same of True to False is False");

    // Test logical operations (Not, And, Or, Xor)
    // Test Not function
    static_assert(std::is_same_v<Not<True, RE>, False>, "Not of True is False");
    static_assert(std::is_same_v<Not<False, RE>, True>, "Not of False is True");

    // Test And function  
    static_assert(std::is_same_v<And<True, True, RE>, True>, "And of True and True is True");
    static_assert(std::is_same_v<And<True, False, RE>, False>, "And of True and False is False");
    static_assert(std::is_same_v<And<False, True, RE>, False>, "And of False and True is False");
    static_assert(std::is_same_v<And<False, False, RE>, False>, "And of False and False is False");

    // Test Or function
    static_assert(std::is_same_v<Or<True, True, RE>, True>, "Or of True and True is True");
    static_assert(std::is_same_v<Or<True, False, RE>, True>, "Or of True and False is True");
    static_assert(std::is_same_v<Or<False, True, RE>, True>, "Or of False and True is True");
    static_assert(std::is_same_v<Or<False, False, RE>, False>, "Or of False and False is False");

    // Test Xor function
    static_assert(std::is_same_v<Xor<True, True, RE>, False>, "Xor of True and True is False");
    static_assert(std::is_same_v<Xor<True, False, RE>, True>, "Xor of True and False is True");
    static_assert(std::is_same_v<Xor<False, True, RE>, True>, "Xor of False and True is True");
    static_assert(std::is_same_v<Xor<False, False, RE>, False>, "Xor of False and False is False");

    // Test logical operations with type variables (analogous to arithmetic tests)
    struct bool_var1 {};
    struct bool_var2 {};
    run_line : Assign<bool_var1, True, RE> {};
    run_line : Assign<bool_var2, False, RE> {};

    // Test Not with variables
    static_assert(std::is_same_v<Not<bool_var1, RE>, False>, "Not of True variable is False");
    static_assert(std::is_same_v<Not<bool_var2, RE>, True>, "Not of False variable is True");

    // Test And with variables and constants
    static_assert(std::is_same_v<And<bool_var1, True, RE>, True>, "And of True variable with True constant is True");
    static_assert(std::is_same_v<And<bool_var1, False, RE>, False>, "And of True variable with False constant is False");
    static_assert(std::is_same_v<And<True, bool_var1, RE>, True>, "And of True constant with True variable is True");
    static_assert(std::is_same_v<And<False, bool_var1, RE>, False>, "And of False constant with True variable is False");
    static_assert(std::is_same_v<And<bool_var1, bool_var2, RE>, False>, "And of True variable with False variable is False");

    // Test Or with variables and constants
    static_assert(std::is_same_v<Or<bool_var1, True, RE>, True>, "Or of True variable with True constant is True");
    static_assert(std::is_same_v<Or<bool_var1, False, RE>, True>, "Or of True variable with False constant is True");
    static_assert(std::is_same_v<Or<True, bool_var2, RE>, True>, "Or of True constant with False variable is True");
    static_assert(std::is_same_v<Or<False, bool_var2, RE>, False>, "Or of False constant with False variable is False");
    static_assert(std::is_same_v<Or<bool_var1, bool_var2, RE>, True>, "Or of True variable with False variable is True");

    // Test Xor with variables and constants
    static_assert(std::is_same_v<Xor<bool_var1, True, RE>, False>, "Xor of True variable with True constant is False");
    static_assert(std::is_same_v<Xor<bool_var1, False, RE>, True>, "Xor of True variable with False constant is True");
    static_assert(std::is_same_v<Xor<False, bool_var1, RE>, True>, "Xor of False constant with True variable is True");
    static_assert(std::is_same_v<Xor<True, bool_var2, RE>, True>, "Xor of True constant with False variable is True");
    static_assert(std::is_same_v<Xor<bool_var1, bool_var2, RE>, True>, "Xor of True variable with False variable is True");


    // Test is_base_of function (from ctstd_base.hpp)
    struct base_test {};
    struct derived_test : base_test {};
    struct unrelated_test {};

    static_assert(std::is_same_v<is_base_of<base_test, derived_test>, True>, "is_base_of works for derived class");
    static_assert(std::is_same_v<is_base_of<base_test, base_test>, True>, "is_base_of works for same class");
    static_assert(std::is_same_v<is_base_of<base_test, unrelated_test>, False>, "is_base_of works for unrelated class");
    static_assert(std::is_same_v<is_base_of<derived_test, base_test>, False>, "is_base_of is not symmetric");

    // Test complex expressions combining multiple operations
    struct e {};
    struct f {};
    run_line : Assign<e, add<mult<a, b, RE>, _1, RE>, RE> {};  // e = (a * b) + 1 = (2 * 3) + 1 = 7
    run_line : Assign<f, minus<c, divide<e, a, RE>, RE>, RE> {}; // f = c - (e / a) = 5 - (7 / 2) = 5 - 3 = 2

    static_assert(std::is_same_v<value<e, RE>, _7>, "complex arithmetic expression works correctly");
    static_assert(std::is_same_v<value<f, RE>, _2>, "nested arithmetic operations work correctly");

    // Test edge cases and boundary conditions
    static_assert(std::is_same_v<add<_0, _0, RE>, _0>, "adding zero to zero gives zero");
    static_assert(std::is_same_v<mult<_1, _5, RE>, _5>, "multiplying by one gives original value");
    static_assert(std::is_same_v<divide<_5, _1, RE>, _5>, "dividing by one gives original value");
    static_assert(std::is_same_v<remainder<_3, _5, RE>, _3>, "remainder when dividend is smaller than divisor");
    static_assert(std::is_same_v<minus<_3, _0, RE>, _3>, "subtracting zero gives original value");
    static_assert(std::is_same_v<leq<_0, _0, RE>, True>, "zero is less than or equal to zero");
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

    run_line : Assign<a, peano::_0, RE> {};
    run_line : Assign<b, peano::_0, RE> {};
    run_line : Assign<c, ctstd::True, RE> {};
    run_line : Assign<loop_cnt, peano::_0, RE> {};


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
                peano::leq<value<a, RE>, peano::_3>, RE
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
    static_assert(std::is_same_v<value<a, RE>, peano::_4>);
#elif __GNUG__
    static_assert(std::is_same_v<value<a, RE>, peano::_5>);
#endif
}

namespace recursion_test_2 {
    using namespace type_var;
    using namespace cexpr_control;

    template <class> struct __run_line {};

    struct a {};
    struct b {};
    struct c {};
    run_line : Assign<a, peano::_0, RE> {};
    run_line : Assign<b, peano::_0, RE> {};
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
                peano::leq<value<a, RE>, peano::_5>, RE
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

namespace recursion_test_2_1 {
    using namespace type_var;
    using namespace ctstd;
    using namespace cexpr_control;

    template <class> struct __run_line {};

    struct a {};
    struct b {};
    struct c {};
    run_line : Assign<a, _0, RE> {};
    run_line : Assign<b, _0, RE> {};
    run_line : Assign<c, True, RE> {};


    struct SumOfFirstIntegers {
        template <class _>
        struct __call__ : 
            Assign<a, add<a, _1, RE>, RE>,
            Assign<b, add<a, b, RE>, RE>,
            Assign<c, leq<a, _5, RE>, RE>
        {};
    };

    run_line : ::cexpr_control::DoWhile<SumOfFirstIntegers, c, RE> {};

#ifdef __clang__
    static_assert(to_bool<eq<b, Integer<21>, RE>>);
#elif __GNUG__
    static_assert(to_bool<eq<b, Integer<28>, RE>>);
#endif
}

namespace recursion_test_delayed_1 {
    using namespace type_var;
    using namespace ctstd;
    using namespace cexpr_control;

    template <class> struct __run_line {};

    struct a {};
    struct b {};
    struct c {};
    struct d {};
    run_line : Assign<a, _0, RE> {};
    run_line : Assign<b, _0, RE> {};
    run_line : Assign<c, True, RE> {};

    struct SumOfFirstIntegers {
        template <class _>
        struct __call__ : 
            Assign<a, add<a, _1, RE>, RE>,
            Assign<b, add<a, b, RE>, RE>,
            Assign<c, leq<a, _5, RE>, RE>
        {};
    };

    run_line : ::cexpr_control::DoWhile<Delayed<Delayed<SumOfFirstIntegers>>, c, RE> {};

#ifdef __clang__
    static_assert(to_bool<eq<b, Integer<21>, RE>>);
#elif __GNUG__
    static_assert(to_bool<eq<b, Integer<28>, RE>>);
#endif
}

namespace big_recursion_test_1 {
    using namespace type_var;
    using namespace ctstd;
    using namespace cexpr_control;

    template <class> struct __run_line {};

    struct a {};
    struct b {};
    struct c {};

    run_line: Assign<a, _7, RE> {};
    // 5 > 16 > 8 > 4 > 2 > 1
    // 7 > 22 > 11 > 34 > 17 > 52 > 26 > 13 > 40 > 20 > 10 > 5 > 16 > 8 > 4 > 2 > 1
    run_line: Assign<b, _0, RE> {};
    run_line: Assign<c, True, RE> {};

    struct CollatzStep {
        template <class _>
        struct __call__ :
            if_else<
                eq<remainder<a, _2, RE>, _0, RE>,
                Assignment<a>,
                divide<a, _2, RE>,
                add<mult<a, _3, RE>, _1, RE>,
                RE
            >,
            Assign<b, add<b, _1, RE>, RE>,
            Assign<c, Not<eq<a, _1, RE>, RE>, RE>
        {};
    };

    run_line: ::cexpr_control::DoWhile<CollatzStep, c, RE> {};

#ifdef __clang__
    static_assert(to_bool<eq<a, Integer<1>, RE>>);
    static_assert(to_bool<eq<b, Integer<16>, RE>>);
#elif __GNUG__
    static_assert(to_bool<eq<a, Integer<2>, RE>>);
    static_assert(to_bool<eq<b, Integer<18>, RE>>);
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