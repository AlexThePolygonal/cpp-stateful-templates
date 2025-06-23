# GCC Template Instantiation Order Analysis by Claude

## Overview

This report analyzes the template instantiation behavior differences between Clang and GCC when using stateful metaprogramming techniques, based on comprehensive testing in `tests.hpp`. The key finding is that GCC exhibits a unique template classification and ordering strategy that significantly differs from Clang's straightforward depth-first approach.

## Key Behavioral Differences

### Clang Behavior (Baseline)
- **Consistent depth-first instantiation**: Templates are instantiated in the exact order they appear, recursively
- **Sequential parent processing**: Parent classes are processed left-to-right, completely before moving to the next
- **Predictable execution order**: `run_line: A, B, C` always executes A→B→C

### GCC Behavior (Complex)
- **Template classification system**: GCC appears to categorize templates into different "complexity levels"
- **Multi-phase instantiation**: Different template types are processed in separate phases
- **Early argument evaluation**: Template arguments are often evaluated before the templates themselves

## Evidence from Tests

### 1. Basic Inheritance Order (`inheritance_order_tests`)

**Test Case:**
```cpp
run_line: 
    Assign<a, short, RE>, 
    if_<True, Assignment<a>, int, RE>, 
    Assign<a, long, RE>
```

**Results:**
- **Clang**: `a = long` (depth-first: short→int→long)
- **GCC**: `a = int` (evaluates non-recursive templates first, then recursive ones)

**Analysis:** GCC classifies `Assign` as "simple" and `if_` as "complex", processing all simple templates before complex ones.

### 2. Mixed Template Complexity

**Test Case:**
```cpp
run_line: 
    if_<True, Delayed<Assignment<b>>, int, RE>, 
    if_<True, Assignment<b>, short, RE>, 
    Assign<b, value<a, RE>, RE>
```

**Results:**
- **Clang**: `b = long` (depth-first order)
- **GCC**: `b = short` (processes non-recursive templates first)

**Analysis:** GCC processes `Assignment<b>` (simple) before `Delayed<Assignment<b>>` (complex).

### 3. Early Argument Evaluation

**Test Case:**
```cpp
run_line:
    Delayed<Assignment<g>>::__call__<float, RE>,
    Assign<h, value<g, RE>, RE>
```

**Results:**
- **Clang**: `h = float` (second template evaluated after first completes)
- **GCC**: `h = int` (second template arguments evaluated before first template)

**Analysis:** GCC evaluates `value<g, RE>` (the argument to the second template) before executing the first template.

### 4. The DoWhile Problem (Critical Evidence)

**Test Case:**
```cpp
struct SumOfFirstIntegers {
    template <class _>
    struct __call__ : 
        Assign<a, peano::Succ<value<a, RE>>, RE>,
        Assign<b, peano::add<value<b, RE>, value<a, RE>>, RE>,
        Assign<c, peano::leq<value<a, RE>, peano::Five>, RE>
    {};
};
```

**Results:**
- **Clang**: `b = 21` (correct sum: 1+2+3+4+5+6)
- **GCC**: `b = 28` (incorrect due to premature evaluation)

**Analysis:** GCC evaluates ALL `value<a, RE>` and `value<b, RE>` calls BEFORE any `Assign` operations execute. This causes all the addition operations to use the same initial values instead of the incrementally updated values.

### 5. Template Wrapping Effects

**Test Case:**
```cpp
template <class T>
struct simplified : Delayed<Assignment<n>>::__call__<float, RE> {};
run_line : simplified<RE>, Assign<n, int, RE>
```

**Results:**
- **Clang**: `n = int` (processes templates in order)
- **GCC**: `n = float` (wrapped template gets processed earlier)

**Analysis:** Wrapping templates in additional template layers can change their classification in GCC's processing order.

## Template Classification Hypothesis

Based on the evidence, GCC appears to use a multi-tier classification system:

### Tier 1: Simple Argument Evaluation
- **Templates**: `value<>`, basic type operations, template parameter resolution
- **Behavior**: Evaluated immediately when encountered, regardless of context
- **Impact**: Can cause premature evaluation in complex template chains

### Tier 2: Simple Templates  
- **Templates**: Direct `Assign`, basic inheritance, simple template instantiations
- **Behavior**: Processed in declaration order, but before complex templates
- **Impact**: Execute reliably in order when not mixed with complex templates

### Tier 3: Complex Templates
- **Templates**: Control flow (`if_`, `if_else`), `DoWhile`, `Delayed` wrappers, recursive templates
- **Behavior**: Processed after simpler templates, may have different ordering rules
- **Impact**: Can be delayed until simpler templates complete

### Tier 4: Nested Complex Templates
- **Templates**: Multiple layers of wrapping (`Delayed<Delayed<...>>`), deeply nested structures
- **Behavior**: May follow different rules entirely, possibly processed in separate phases
- **Impact**: Can sometimes execute earlier than expected due to unwrapping

## Specific GCC Patterns Observed

### 1. Argument Pre-evaluation
```cpp
// GCC evaluates value<g, RE> before Delayed<Assignment<g>>::__call__ executes
Delayed<Assignment<g>>::__call__<float, RE>, Assign<h, value<g, RE>, RE>
```

### 2. Simple-before-Complex Ordering
```cpp
// GCC processes Assign before if_, regardless of declaration order
Assign<a, short, RE>, if_<True, Assignment<a>, int, RE>, Assign<a, long, RE>
```

### 3. Mass Argument Evaluation in Loops
```cpp
// In DoWhile, ALL value<> calls are evaluated before ANY Assign operations
Assign<a, peano::Succ<value<a, RE>>, RE>,  // value<a, RE> evaluated early
Assign<b, peano::add<value<b, RE>, value<a, RE>>, RE>  // both values evaluated early
```

### 4. Template Wrapping Effects
```cpp
// Wrapping can promote templates to earlier processing phases
simplified<RE>  // May execute before simpler templates that come after it
```

## Implications for Stateful Metaprogramming

### 1. Loop Correctness
- **Problem**: DoWhile loops produce incorrect results on GCC due to early argument evaluation
- **Workaround**: Avoid complex expressions in loop bodies, use intermediate assignments

### 2. Execution Order Reliability
- **Problem**: Mixed simple/complex template inheritance produces different orders on GCC vs Clang
- **Workaround**: Separate complex operations into distinct inheritance levels

### 3. State Dependencies
- **Problem**: Templates depending on previously modified state may see stale values
- **Workaround**: Use explicit sequencing with template barriers

## Conclusion

GCC's template instantiation strategy represents a significant departure from the intuitive depth-first approach used by Clang. The multi-tier classification system, while potentially more efficient for standard template usage, creates substantial challenges for stateful metaprogramming where execution order is semantically significant.

The core issue is that GCC attempts to optimize template instantiation by grouping similar operations, but this optimization breaks the sequential execution semantics required for imperative-style metaprogramming. The early evaluation of `value<>` expressions is particularly problematic, as it violates the fundamental assumption that state reads occur at the point of template instantiation.

This analysis suggests that portable stateful metaprogramming requires careful consideration of GCC's classification behavior and may necessitate design patterns that explicitly work within GCC's multi-phase instantiation model.
