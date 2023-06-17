# C-Plus
![alt text](https://github.com/ColleagueRiley/c-plus/blob/main/logo.png?raw=true)

C-Plus is an middle ground between c and c++. 

It attemps to merge the cleanness low-levelness, and over all efficiency of c with higher level abstraction.

Although C-Plus supports objects [via structures with functions] it does not support OOP directly and does not plan to ever support OOP directly.

# Build statuses
![cplus workflow](https://github.com/ColleagueRiley/c-plus/actions/workflows/linux.yml/badge.svg)
![cplus workflow windows](https://github.com/ColleagueRiley/c-plus/actions/workflows/windows.yml/badge.svg)
![cplus workflow windows](https://github.com/ColleagueRiley/c-plus/actions/workflows/macos.yml/badge.svg)

# building
    to build the cplus compiler, simply run `make`

    you can also very easily compile cplus by hand by running
    
    `[your c compiler (ex. gcc)] main.c -o cplus`

# installing
    to install cplus, simply move cplus into your bin directory, on unix you can do this just by running `sudo make install` or `sudo mv cplus /usr/bin`

    to install cplus on windows however, you must move cplus into the same directory that your c-compiler exectuable file is in

# Supported Features
- structs can be used normally without typedef
- structs can be used as a class, i.e. the usr can define functions inside structs

externally defined functions use the same synstax as c++

ex.
```cpp
void type::func(){

}
```

- namespace, namespace syntax is slightly different than c++'s in that it uses `.` [optionally] instead of `::`

ex.

```cpp
namespace ns {
    int a;
}

int main() {
    ns.a = 5;
    ns::a = 5; /* this still works but it's ugly :)*/
}
```

# Future Features
- void*/casting object functions
- load object using data (eg. char*)
- opperator overloading

# Sili
The C-Plus compiler uses the [Sili Toolkit](https::/github.com/EimaMei/Sili-Toolkit)] for handling strings, arrays and file I/O

Sili helps to make C feel more modern and high level. Unlike many other higher level Toolkit/Standard Libraries [such as the c++ standard library]. 
Sili also makes sure the library doesn't just feel nice to use, it also runs efficiently.

Not does Sili modernize C while staying efficient, it is also very lightweight and is all contained in one small single-header-file.

This is why C-Plus uses sili for copiling and why C-Plus ships with sili and uses it as the offical C-Plus Standard Library.

# Copyright
The language design of c-plus (conversion to c, basic design, ect) is 100% public domain.

This means you can make your own c-plus compiler if you wish to. Infact, I encourage that you do if you're interested. 

However,

The source code for this project (main.c) is copyrighted under the libpng license

this pretty much means you can use the code as you please so long as you don't pretend you wrote the original code, state any changes you made and keep all the copyright info attached