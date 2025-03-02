# 定义可执行文件名称
EXECUTABLE = rpc_server

# 源文件配置
FRAMEWORK_SRC = $(wildcard src/framework/*.cpp)
SERVICES_SRC =  $(wildcard src/services/*.cpp)
MAIN_SRC = src/main.cpp

# 目标文件生成路径
LIB_OBJS = $(patsubst src/framework/%.cpp, build/framework/%.o, $(FRAMEWORK_SRC))
SERVICE_OBJS = $(patsubst src/services/%.cpp, build/services/%.o, $(SERVICES_SRC))
MAIN_OBJ = build/main.o

# 编译参数
CXX = g++
CXXFLAGS = -Wall -O2
CXXFLAGS += -std=c++11 
CXXFLAGS += -DCPP11_SUPPORTED=1
CXXFLAGS += -I./include -I/usr/include/event2 -I/usr/include/nlohmann 
#CXXFLAGS += -Iextern/json
LDFLAGS = -L./lib \
		-lframework \
		-lservices \
		-levent_openssl \
		-levent \
		-lpthread \
		-lssl \
		-lcrypto

# 构建目标
all: prepare libframework.a libservices.a $(EXECUTABLE)

# 主程序构建
$(EXECUTABLE): $(MAIN_OBJ) libframework.a libservices.a
	@echo "链接主程序:   $@"
	@$(CXX)   $< $(LDFLAGS) -o   $@

# 主程序编译规则
build/main.o: src/main.cpp
	@echo "编译主程序: $<"
	@mkdir -p   $(@D)
	@$(CXX)   $(CXXFLAGS) -c $< -o   $@

# 静态库构建规则
libframework.a: $(LIB_OBJS)
	@echo "构建框架库:   $@"
	@ar rcs lib/$@   $^

libservices.a: $(SERVICE_OBJS) 
	@echo "构建服务库:   $@"
	@ar rcs lib/$@   $^

# 通用编译规则
build/framework/%.o: src/framework/%.cpp
	@echo "编译框架组件: $<"
	@mkdir -p   $(@D)
	@$(CXX)   $(CXXFLAGS) -c $< -o   $@

build/services/%.o: src/services/%.cpp
	@echo "编译服务组件: $<"
	@mkdir -p   $(@D)
	@$(CXX)   $(CXXFLAGS) -c $< -o   $@

# 准备构建目录
prepare:
	@mkdir -p build/framework
	@mkdir -p build/services
	@mkdir -p lib

cert:
	@echo "Generating HTTPS certificates"
	@mkdir -p cert
	@./tools/gen_cert.sh

# 清理构建产物
clean:
	@rm -rf build
	@rm -rf lib
	@rm -f $(EXECUTABLE)

# 打印编译参数和链接参数
print-flags:
	@echo "----------------------------------------"
	@echo "编译参数 (CXXFLAGS):"
	@echo "----------------------------------------"
	@echo $(CXXFLAGS)
	@echo "----------------------------------------"
	@echo "链接参数 (LDFLAGS):"
	@echo "----------------------------------------"
	@echo $(LDFLAGS)
	@echo "----------------------------------------"

.PHONY: all prepare cert clean print-flags