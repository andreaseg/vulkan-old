NAME= main
WARN= -Wall -Wextra
CPPVER= -std=c++17
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
	LDFLAGS  = -m64 -g -o3 -march=native -Wl,-subsystem:console
else
	CPPFLAGS = -m64 -o3 -march=native
	LDFLAGS  = -m64 -o3 -march=native -Wl,-subsystem:windows
endif

ifeq ($(OS),Windows_NT)
LDFLAGS+= $(CPPVER) $(WARN)
LDLIBS= -L$(shell echo $(VULKAN_SDK))/Source/lib -lvulkan-1 -L$(shell echo $(GLFW))/Libs/ -lglfw3dll 
TARGET=$(NAME).exe
CPPFLAGS+= -DWINDOWS
else
LDFLAGS+= $(CPPVER) $(WARN)
LDLIBS=-ldl -L$(shell echo $(VULKAN_SDK))/lib -lvulkan 
TARGET=$(NAME)
endif
CPPFLAGS+= -Xclang -flto-visibility-public-std $(CPPVER) $(WARN)
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

# Shader compilation

$(shell mkdir -p $(DIR)/shaders)

SHDSRCS = $(wildcard src/shaders/*)
SHDTAR = $(patsubst src/shaders/%,$(DIR)/shaders/%.spv,$(SHDSRCS))

ifeq ($(OS),Windows_NT)
	VAL = $(VULKAN_SDK)/Bin32/glslangValidator.exe
else
	VAL = $(VULKAN_SDK)/x86_64/bin/glslangValidator
endif

shaders: $(SHDTAR)

$(DIR)/shaders/%.spv: src/shaders/%
	@echo "Compiling shader $@"
	@$(VAL) -V $< -o $@

clean-shaders: $(SHDTAR)
	@$(RM) $(SHDTAR)
