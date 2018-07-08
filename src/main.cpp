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
    


    // Graphics

    App app;

    app.addKeyCallback(GLFW_KEY_A, GLFW_PRESS, 0, [](){
        std::cout << "A" << std::endl;
        });

    app.addKeyCallback(GLFW_KEY_ESCAPE, GLFW_PRESS, 0, [&app](){
        app.close();
    });

    app.addMouseCallback(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, 0, [](double x, double y){
        std::cout << "Click: " << x << ", " << y << std::endl;
    });

    app.addWindowFocusCallback(true, [](){
        std::cout << "Focus gained" << std::endl;
    });

    app.addWindowFocusCallback(false, [](){
        std::cout << "Focus lost" << std::endl;
    });

    app.start();

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