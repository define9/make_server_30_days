# Makefile for Day11 - Timer Implementation

CXX = g++
CXXFLAGS = -std=c++14 -Wall -Wextra -pedantic -I$(SRC_DIR)

SRC_DIR = src
BUILD_DIR = build
TARGET = server

SOURCES = $(wildcard $(SRC_DIR)/*.cpp) \
          $(wildcard $(SRC_DIR)/net/*.cpp) \
          $(wildcard $(SRC_DIR)/reactor/*.cpp) \
          $(wildcard $(SRC_DIR)/thread/*.cpp) \
          $(wildcard $(SRC_DIR)/timer/*.cpp)

OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
DEPS = $(OBJECTS:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "Linking $@..."
	$(CXX) $(CXXFLAGS) -o $@ $^ -lpthread

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

-include $(DEPS)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

rebuild: clean all

.PHONY: all clean rebuild