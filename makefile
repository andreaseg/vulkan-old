ifeq ($(OS),Windows_NT)
LDFLAGS=-g -Xclang -flto-visibility-public-std
LDLIBS=  -L$(shell echo $(VULKAN_SDK))/Source/lib -lvulkan-1
TARGET=main.exe
VULKAN_INCLUDE=
else
LDFLAGS=-g
LDLIBS=-ldl -L$(shell echo $(VULKAN_SDK))/lib -lvulkan
TARGET=main
VULKAN_INCLUDE=
endif

SRCS= src/main.cpp 

CC=clang
CXX=clang
OBJS=main.o
RM=rm -f
INCLUDE= -I$(VULKAN_INCLUDE)
CPPFLAGS=-g -std=c++11 -Wall -Wextra 

.cpp.o:
    $(CXX) $(CPPFLAGS) $(INCLUDE) -c $<

all: tool

tool: $(OBJS)
	$(CXX) $(INCLUDE) $(LDFLAGS) -o target/$(TARGET) $(OBJS) $(LDLIBS) 

clean:
	$(RM) $(OBJS)

distclean: clean
	$(RM) 

