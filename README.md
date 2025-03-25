### DEVLOG HELLWORLD

Блять, какой же пиздец нахуй. Ну я сам выбрал ломать компилятор.

#### Premature instantiation of templates with dummy parameters  

One thing you would notice is that I always pass the unique tags `class _ = decltype([](){})` into the nested call structure. I have to do this for two reasons:

First, it saves a lot of time for the compiler, who has to instantiate far less stuff. Testing on GCC, it cuts down the compilation time of my test suite by an order of magnitude.

Second, the compilate always tries to save effort, and if it sees that a template «function» like
```c++
template <class T>
struct Func : Assign<Name, value<Other>> {};
```
which you try to effectfully instantiate at a subsequent location, it will see that _akshully_ you're not using T. Then to save on computation, it will eliminate the T, instantiate `Func` immediately and simply substitute it at each instantiation-site instead of reinstantiating it each time. You will see that the side effects appear only at declaration site.

You can try to call it like this:
```c++
template <class T>
struct Func : Assign<Name, value<Other>, T> {};
```
This is better, however the elimination analysis also applies to `Assign<...>` itself. If the unique tag-type isn't used, GCC will mark is as eliminable immediately, and this eliminable mark means that the parameter of `Func` can also be eliminated, with the same result.

Therefore, we have to pass this parameter as deep as possible, so it taints everything. Then because of the SFINAE complications, it _probably_ gives up, allowing as to actually define functions.

So it goes.

#### Immediate instantiation of simple templates

You would expect that the compiler processes the template parent classes sequentially, from left to right. However, GCC is smarter. It processes them in two passes, and if it sees a simple template, that is, a template class with all of its arguments being non-template classes, it will instantiate it immediately. Otherwise, it defers the instantiation to the second pass. 

So it goes.

### Default arguments are cached

*Obviously*, in
```cpp
template <class _ = decltype([](){})>
```
the default argument stays the same in different instantiations
So we always have to write `decltype([](){})` explicitly to force reinstantiations.