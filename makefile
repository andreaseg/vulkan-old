ifeq ($(OS),Windows_NT)
CC=clang
CXX=clang
RM=rm -f
CPPFLAGS=-g -std=c++11  -I$(shell echo $(VULKAN_SDK))include/vulkan -Wall -Wextra 
LDFLAGS=-g -Xclang -flto-visibility-public-std
LDLIBS=  -L$(shell echo $(VULKAN_SDK))/Source/lib -lvulkan-1
TARGET=main.exe
else
CC=gcc
CXX=g++
RM=rm -f
CPPFLAGS=-g -std=c++11 -I$(shell echo $(VULKAN_SDK))include/vulkan -Wall -Wextra
LDFLAGS=-g
LDLIBS=-ldl -L$(shell echo $(VULKAN_SDK))/lib -lvulkan
TARGET=main
endif

SRCS=src/VulkanFunctions.cpp src/vulkanLoader.cpp src/main.cpp 
OBJS=$(subst .cc,.o,$(SRCS))

all: tool

tool: $(OBJS)
	$(CXX) $(LDFLAGS) -o target/$(TARGET) $(OBJS) $(LDLIBS) 

depend: .depend

.depend: $(SRCS)
	$(RM) ./.depend
	$(CXX) $(CPPFLAGS) -MM $^>>./.depend;

clean:
	$(RM) $(OBJS)

distclean: clean
	$(RM) *~ .depend

include .depend
