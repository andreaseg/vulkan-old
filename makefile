NAME= main
WARN= -Wall -Wextra 

ifeq ($(OS),Windows_NT)
LDFLAGS=-g -Xclang -flto-visibility-public-std $(WARN)
LDLIBS= -L$(shell echo $(VULKAN_SDK))/Source/lib -lvulkan-1
TARGET=$(NAME).exe
else
LDFLAGS=-g
LDLIBS=-ldl -L$(shell echo $(VULKAN_SDK))/lib -lvulkan
TARGET=$(NAME)
endif

SRCS= $(wildcard src/*.cpp)

CC=clang
CXX=clang
OBJS=$(subst .cc,.o,$(SRCS))
RM=rm -f
INC= -I$(shell echo $(VULKAN_SDK))/Include
CPPFLAGS=-g -std=c++11 $(WARN)

DIRS=target
$(shell mkdir -p $(DIRS))


all: $(NAME)

$(NAME): $(OBJS)
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
