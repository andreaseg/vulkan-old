NAME= main
WARN= -Wall -Wextra
CPPVER= -std=c++14
SRCS= $(wildcard src/*.cpp)
CC=clang
CXX=clang

ifeq ($(OS),Windows_NT)
LDFLAGS=-g $(CPPVER) $(WARN)
LDLIBS= -L$(shell echo $(VULKAN_SDK))/Source/lib -lvulkan-1
TARGET=$(NAME).exe
else
LDFLAGS=-g $(CPPVER) $(WARN)
LDLIBS=-ldl -L$(shell echo $(VULKAN_SDK))/lib -lvulkan
TARGET=$(NAME)
endif

OBJS=$(patsubst src/%,target/%,$(patsubst %.cpp,%.o,$(SRCS)))
RM=rm -f
INC= -I$(shell echo $(VULKAN_SDK))/Include 
CPPFLAGS=-g -Xclang -flto-visibility-public-std $(CPPVER) $(WARN)

DIRS=target
$(shell mkdir -p $(DIRS))

all: $(TARGET)
	@true

clean:
	@$(RM) $(TARGET) $(OBJS)

$(TARGET): $(OBJS)
	@echo "Linking the target $@"
	@$(CXX) $(LDFLAGS) -o target/$@ $^ $(LDLIBS)

target/%.o : src/%.cpp
	@echo "Compiling $<"
	@$(CXX) $(INC) $(CPPFLAGS) -c $< -o $@
