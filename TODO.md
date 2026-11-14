- handle multi-line pre-processors
- compiling from multiple sources
- allow class functions to directly edit the values of a struct w/o `->` eg. `a = 5;` instead of `this->a = 5;`
- void casting objects
```c
void* a;
((class*)a)->func()
*((class)a).func()
```
- typedef support for classes [as alt name for classes]
- embeded structs [ in another struct ]
- embeded namespace [ in another namespace ]
- operator overloading
- Update single header files (C lexer and sili)

(maybe)
- string switches
- multi argument system 
- allow c errors to extend to the header 
