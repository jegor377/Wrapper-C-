# Wrapper-CPP
Little wrapper for C++ language. It links all hpp files inside a directory to single header file for C++.

It basically loads all files from the directory that you pass over to it and in the specific init file, it replaces "@import <import file name>" to the insides of the imported file. This helps me to create standalone header files for C++ and avoid compilation errors. :/

It's using Min-GW but I think it's going to work on linux equivalent also.

Compiling:
g++ -c main.cpp -std=c++11
g++ -o wcpp main.o
