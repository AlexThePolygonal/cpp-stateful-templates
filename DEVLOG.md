# HELLWORLD ULTRA TORMENT NIGHMARE DEVLOG

WARNING: THIS IS A STATEFUL METAPROGRAMMING PROJECT USING STATEFUL TEMPLATES. 

Corrupt insanity, monstrous and vile, cruelly twisted beyond all human notions of good and evil. Welll, I did decide to break the compiler myself, so who am I to complain.

### Default arguments are cached

*Obviously*, in
```cpp
template <class _ = decltype([](){})>
```
the default argument might decide to get itself cached and stay the same in different instantiations.
Normally, you'll never see it, but in some conditions, you will 
So we always have to write `decltype([](){})` explicitly to force reinstantiations.

#### Premature instantiation of templates with dummy parameters  

One of the reasons why default arguments don't work is this:

I always pass the unique tags `class _ = decltype([](){})` into the nested call structure. I have to do this for two reasons.

First, it saves a lot of time for the compiler, who has to instantiate far less stuff. Testing on GCC, it cuts down the compilation time of my test suite by an order of magnitude.

Second, the compilate always tries to save effort, and if it sees that a template «function» like
```c++
template <class T>
struct Func : Assign<Name, value<Other>> {};
```
which you try to effectfully instantiate at a subsequent location, it will see that _akshully_ you're not using T. Then if you use the default argument `_` of `Assign`, then it consider `Assign<Name, value<Other>>` to always be the same.
Then, the compiler says to itself that its free to instantiate `Func` once and ignore the dependency on `T`. 


So it goes.

#### Immediate instantiation of simple templates

You would expect that the compiler processes the template parent classes sequentially, from left to right. However, GCC is smarter. It processes them in two passes, and if it sees a simple template, that is, a template class with all of its arguments being non-template classes, it will instantiate it immediately. Otherwise, it defers the instantiation to the second pass. 

For example

So it goes.