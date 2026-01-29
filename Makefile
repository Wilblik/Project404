OBJ_DIR = obj
BIN_DIR = bin

TARGET = $(BIN_DIR)/Project404
OS ?= linux

SRCS = main.cpp
OBJS = $(SRCS:%.cpp=$(OBJ_DIR)/%.o)

CXX = g++
PKG_CONFIG = pkg-config
CXXFLAGS = -Wall -Wextra -std=c++23 `$(PKG_CONFIG) --cflags sdl3`
LDFLAGS  = `$(PKG_CONFIG) --libs sdl3`

ifeq ($(OS), win)
    CXX = x86_64-w64-mingw32-g++
    PKG_CONFIG = x86_64-w64-mingw32-pkg-config
    LDFLAGS = -static `$(PKG_CONFIG) --static --libs sdl3`
    TARGET := $(TARGET).exe
endif

all: $(TARGET)

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: all
	./$(TARGET)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean
