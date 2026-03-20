# Makefile for Day09 - Multi-threaded Reactor Echo Server

CXX = g++
CXXFLAGS = -std=c++14 -Wall -Wextra -pedantic

SRC_DIR = src
BUILD_DIR = build
TARGET = server

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
DEPS = $(OBJECTS:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "Linking $@..."
	$(CXX) $(CXXFLAGS) -o $@ $^ -lpthread

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

-include $(DEPS)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

rebuild: clean all

.PHONY: all clean rebuild