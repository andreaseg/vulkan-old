#ifndef CONSOLE_COLOR_H
#define CONSOLE_COLOR_H

#ifdef __linux__

#elif _WIN32
#include <windows.h>        
#else
#error "Unknown or unsupported OS"
#endif


namespace console_color {

    struct color {
        #ifdef __linux__
        
        #elif _WIN32
            WORD value;

            color(WORD value) : value(value) {}

        #else
        #error "Unknown or unsupported OS"
        #endif
    };

    #ifdef __linux__
        
    #elif _WIN32

    static color BLACK(0);
    static color DARK_BLUE(1);
    static color DARK_GREEN(2);
    static color DARK_CYAN(3);
    static color DARK_RED(4);
    static color DARK_MAGENTA(5);
    static color DARK_YELLOW(6);
    static color DARK_GREY(7);
    static color GREY(8);
    static color BLUE(9);
    static color GREEN(10);
    static color CYAN(11);
    static color RED(12);
    static color MAGENTA(13);
    static color YELLOW(14);
    static color WHITE(15);

    #else
    #error "Unknown or unsupported OS"
    #endif

    static color get_color() {
        #ifdef __linux__
        
        #elif _WIN32

        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        _CONSOLE_SCREEN_BUFFER_INFO screen_buffer_info;
        GetConsoleScreenBufferInfo(hConsole, &screen_buffer_info);
        return color(screen_buffer_info.wAttributes);
        #else
        #error "Unknown or unsupported OS"
        #endif
    }
    

    static color set_color(color c) {
        #ifdef __linux__
        
        #elif _WIN32

        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        _CONSOLE_SCREEN_BUFFER_INFO screen_buffer_info;
        GetConsoleScreenBufferInfo(hConsole, &screen_buffer_info);
        color old_color = screen_buffer_info.wAttributes;

        SetConsoleTextAttribute(hConsole, 
        (c.value & 0xf) | (old_color.value & (~0xf))
        );
        return old_color;
        #else
        #error "Unknown or unsupported OS"
        #endif
        
    }

    
}

#endif // CONSOLE_COLOR_H