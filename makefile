NAME= main
WARN= -Wall -Wextra
CPPVER= -std=c++14
SRCS= $(wildcard src/*.cpp)
CC=clang
CXX=clang

DEBUG?= 1

ifeq ($(DEBUG), 1)
	CPPFLAGS += -g
	LDFLAGS += -g
	DIR=debug
else
	CPPFLAGS += -o2
	LDFLAGS += -o2
	DIR=release
endif

ifeq ($(OS),Windows_NT)
LDFLAGS+= $(CPPVER) $(WARN)
LDLIBS= -L$(shell echo $(VULKAN_SDK))/Source/lib -lvulkan-1
TARGET=$(NAME).exe
else
LDFLAGS+= $(CPPVER) $(WARN)
LDLIBS=-ldl -L$(shell echo $(VULKAN_SDK))/lib -lvulkan
TARGET=$(NAME)
endif
OBJS=$(patsubst src/%,$(DIR)/%,$(patsubst %.cpp,%.o,$(SRCS)))
RM=rm -f
INC= -I$(shell echo $(VULKAN_SDK))/Include 
CPPFLAGS+= -Xclang -flto-visibility-public-std $(CPPVER) $(WARN)

$(shell mkdir -p $(DIR))

all: $(TARGET)
	@true

clean:
	@$(RM) $(DIR)/$(TARGET) $(OBJS)

$(TARGET): $(OBJS)
	@echo "Linking the target $@"
	@$(CXX) $(LDFLAGS) -o $(DIR)/$@ $^ $(LDLIBS)

$(DIR)/%.o : src/%.cpp
	@echo "Compiling $<"
	@$(CXX) $(INC) $(CPPFLAGS) -c $< -o $@
