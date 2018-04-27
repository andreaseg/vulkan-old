ifeq ($(OS),Windows_NT)
LDFLAGS=-g -Xclang -flto-visibility-public-std
LDLIBS=  -L$(shell echo $(VULKAN_SDK))/Source/lib -lvulkan-1
TARGET=main.exe
else
LDFLAGS=-g
LDLIBS=-ldl -L$(shell echo $(VULKAN_SDK))/lib -lvulkan
TARGET=main
endif

SRCS= src/main.cpp 

CC=clang
CXX=clang
OBJS=main.o
RM=rm -f
INC= -I$(shell echo $(VULKAN_SDK))/Include
CPPFLAGS=-g -std=c++11 -Wall -Wextra 

all: tool

tool: $(OBJS)
	$(CXX) $(INC) $(LDFLAGS) -o target/$(TARGET) $(OBJS) $(LDLIBS) 

depend: .depend

.depend: $(SRCS)
	$(RM) ./.depend
	$(CXX) $(INC) $(CPPFLAGS) -MM $^>>./.depend;

clean:
	$(RM) $(OBJS)

distclean: clean
	$(RM) *~ .depend

include .depend
