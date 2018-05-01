#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include <algorithm>

#include "graphics_engine.hpp"

class App : public Graphics {
    void loop() {}
};

int main() {

    #ifdef DEBUG
    #else
    std::ofstream cout("log.txt");
    std::cout.rdbuf(cout.rdbuf());
    std::ofstream cerr("error.txt");
    std::cerr.rdbuf(cerr.rdbuf());
    #endif
    

    App gfx;
    gfx.start();

    return 0;
    
}

// Windows entry-point
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;
    return main();
}