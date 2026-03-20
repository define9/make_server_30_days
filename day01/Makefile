# Makefile for Day01 - Blocking I/O Echo Server
#
# 编译方法：
#   make          # 编译
#   make clean    # 清理
#   make rebuild  # 重新编译

# 编译器设置
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pedantic
# -std=c++11: 使用C++11标准
# -Wall: 开启所有警告
# -Wextra: 开启额外警告
# -pedantic: 严格遵循C++标准

# 目录设置
SRC_DIR = src
BUILD_DIR = build
TARGET_SERVER = server

# 源文件
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
# wildcard: 获取匹配的文件列表
# $(SRC_DIR)/*.cpp: src目录下所有.cpp文件

# 对象文件 (在build目录)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# 编译server
$(TARGET_SERVER): $(OBJECTS)
	@echo "Linking $@..."
	$(CXX) $(CXXFLAGS) -o $@ $^

# 编译规则：.cpp -> .o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<
	# -MMD: 生成.d依赖文件
	# -MP: 为每个依赖文件添加伪目标，防止header被删除

# 清理
clean:
	rm -rf $(BUILD_DIR) $(TARGET_SERVER)

# 重新编译
rebuild: clean all

# 运行server (用于测试)
# 用法: make run PORT=8080
run: $(TARGET_SERVER)
	@echo "Starting server on port $(PORT)..."
	./$(TARGET_SERVER) $(PORT)

# 帮助
help:
	@echo "Available targets:"
	@echo "  make              - Build the server"
	@echo "  make server       - Build only the server"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make rebuild      - Clean and rebuild"
	@echo "  make run PORT=8080       - Run the server"

.PHONY: all clean rebuild run help
