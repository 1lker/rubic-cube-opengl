# Makefile for Bouncing Ball Program

# Default compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -I. -Iinclude -I/opt/homebrew/include

# Project name
TARGET = homework2

# Source files
SOURCES = main.cpp InitShader.cpp

# Default target (using CMake)
all: cmake_build

# CMake build
cmake_build:
	mkdir -p build
	cd build && cmake .. && make

# Direct build for macOS
macos:
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -L/opt/homebrew/lib -lglfw

# Direct build for Linux
linux:
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) -lGL -lGLEW -lglfw

# Clean build files
clean:
	rm -rf build $(TARGET)
	rm -f *.o

# Run the program
run: all
	./build/$(TARGET)

# Help message
help:
	@echo "Makefile targets:"
	@echo "  all        - Build using CMake (default)"
	@echo "  macos      - Build directly on macOS"
	@echo "  linux      - Build directly on Linux"
	@echo "  clean      - Remove build files"
	@echo "  run        - Build and run the program"
	@echo "  help       - Show this help message"

.PHONY: all cmake_build macos linux clean run help