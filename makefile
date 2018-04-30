NAME= main
WARN= -Wall -Wextra
CPPVER= -std=c++14
SRCS= $(wildcard src/*.cpp)
CC=clang
CXX=clang
DIR=target
DEBUG?=1

ifeq ($(shell echo $(VULKAN_SDK)),)
$(error Vulkan directory not set. Add directory as VULKAN_SDK to enviroment)
endif
ifeq ($(shell echo $(GLFW)),)
$(error GLFW directory not set. Add directory as GLFW to enviroment)
endif

ifeq ($(DEBUG), 1)
	CPPFLAGS = -m64 -g -o3 -march=native -DDEBUG
	LDFLAGS  = -m64 -g -o3 -march=native
else
	CPPFLAGS = -m64 -o3 -march=native
	LDFLAGS  = -m64 -o3 -march=native
endif

CPPFLAGS+= -Xclang -flto-visibility-public-std $(CPPVER) $(WARN)
ifeq ($(OS),Windows_NT)
LDFLAGS+= $(CPPVER) $(WARN) -Wl,-subsystem:windows
LDLIBS= -L$(shell echo $(VULKAN_SDK))/Source/lib -lvulkan-1 -L$(shell echo $(GLFW))/Libs/ -lglfw3dll
TARGET=$(NAME).exe
else
LDFLAGS+= $(CPPVER) $(WARN)
LDLIBS=-ldl -L$(shell echo $(VULKAN_SDK))/lib -lvulkan 
TARGET=$(NAME)
endif
OBJS=$(patsubst src/%,$(DIR)/%,$(patsubst %.cpp,%.o,$(SRCS)))
RM=rm -f
INC= -I$(shell echo $(VULKAN_SDK))/Include  -I$(shell echo $(GLFW))/Include

$(shell mkdir -p $(DIR))

ifeq ($(DEBUG), 1)
all: $(TARGET)
	@true
else
all: $(TARGET)
	@true
	@$(RM) $(OBJS) $(DIR)/$(NAME).ilk $(DIR)/$(NAME).pdb
endif

clean:
	@$(RM) $(OBJS) $(DIR)/$(NAME).ilk $(DIR)/$(NAME).pdb $(DIR)/$(TARGET)

$(TARGET): $(OBJS)
	@echo "Linking the target $@"
	@$(CXX) $(LDFLAGS) -o $(DIR)/$@ $^ $(LDLIBS)

$(DIR)/%.o : src/%.cpp
	@echo "Compiling $<"
	@$(CXX) $(INC) $(CPPFLAGS) -c $< -o $@
