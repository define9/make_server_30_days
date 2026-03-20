# Makefile for Socket Client Test
#
# 编译方法：
#   make          # 编译
#   make clean    # 清理
#   make rebuild  # 重新编译
#   make run      # 运行客户端

# 编译器设置
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pedantic

# 目录设置
SRC_DIR = src
BUILD_DIR = build
TARGET_CLIENT = client

# 源文件
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# 编译client
$(TARGET_CLIENT): $(OBJECTS)
	@echo "Linking $@..."
	$(CXX) $(CXXFLAGS) -o $@ $^

# 编译规则：.cpp -> .o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

# 清理
clean:
	rm -rf $(BUILD_DIR) $(TARGET_CLIENT)

# 重新编译
rebuild: clean all

# 运行客户端
# 用法: make run IP=127.0.0.1 PORT=8080
run: $(TARGET_CLIENT)
	@echo "Starting client..."
	@echo "Connect to $(IP):$(PORT), type messages and press Enter to send, Ctrl+C to quit"
	./$(TARGET_CLIENT) $(IP) $(PORT)

# 帮助
help:
	@echo "Available targets:"
	@echo "  make                      - Build the client"
	@echo "  make clean                - Remove build artifacts"
	@echo "  make rebuild              - Clean and rebuild"
	@echo "  make run IP=127.0.0.1 PORT=8080 - Run the client"
	@echo ""
	@echo "Default: IP=127.0.0.1, PORT=8080"

.PHONY: all clean rebuild run help