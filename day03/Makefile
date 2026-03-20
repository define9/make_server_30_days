# Makefile for Day03 - Poll Multiplexing Echo Server
#
# 编译方法：
#   make          # 编译
#   make clean    # 清理

CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pedantic

SRC_DIR = src
BUILD_DIR = build
TARGET = server

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
DEPS = $(OBJECTS:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "Linking $@..."
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

-include $(DEPS)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

rebuild: clean all

help:
	@echo "Targets:"
	@echo "  make          - Build server"
	@echo "  make clean    - Clean build"
	@echo "  make rebuild  - Rebuild"
	@echo "  make run PORT=8080 - Run server"

.PHONY: all clean rebuild help
