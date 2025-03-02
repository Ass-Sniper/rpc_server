#!/bin/bash

# 创建工程目录结构
mkdir -p \
  include/framework \
  include/services \
  src/framework \
  src/services \
  lib \
  cert

# 创建框架头文件
touch include/framework/{ioc_container,rpc_server}.h

# 创建业务接口头文件 
touch include/services/{rpc_service,math_service}.h

# 创建框架实现文件
touch src/framework/{ioc_container,rpc_server}.cpp

# 创建业务实现文件
touch src/services/{rpc_service,math_service}.cpp

# 创建主文件
cat << EOF > main.cpp
#include "framework/rpc_server.h"
#include "services/math_service.h"

int main() {
    // 服务初始化代码
    return 0;
}
EOF

# 创建Makefile模板
cat << 'EOF' > Makefile
CC = g++
CXXFLAGS = -std=c++11 -I./include -Wall -O2
LDFLAGS = -L./lib -lframework -lservices -levent_openssl -lssl -lcrypto

# 静态库编译目标
FRAMEWORK_SRC = \$(wildcard src/framework/*.cpp)
SERVICES_SRC = \$(wildcard src/services/*.cpp)

LIB_OBJS = \\
    \$(patsubst src/framework/%.cpp, build/framework/%.o, \$(FRAMEWORK_SRC)) \\
    \$(patsubst src/services/%.cpp, build/services/%.o, \$(SERVICES_SRC))

# 目录创建
MKDIR = mkdir -p

all: prepare libs server

prepare:
	\$(MKDIR) build/framework
	\$(MKDIR) build/services
	\$(MKDIR) lib

# 框架库编译
build/framework/%.o: src/framework/%..cpp
	\$(CC) \$(CXXFLAGS) -c \$< -o \$@

libframework.a: \$(patsubst src/framework/%.cpp, build/framework/%.o, \$(FRAMEWORK_SRC))
	ar rcs lib/\$@ \$^

# 业务库编译 
build/services/%.o: src/services/%..cpp
	\$(CC) \$(CXXFLAGS) -c \$< -o \$@

libservices.a: \$(patsubst src/services/%.cpp, build/services/%.o, \$(SERVICES_SRC))
	ar rcs lib/\$@ \$^

# 主程序
server: main.cpp libframework.a libservices.a
	\$(CC) \$(CXXFLAGS) \$< -o \$@ \$(LDFLAGS)

clean:
	rm -rf build lib server

.PHONY: all prepare libs server clean
EOF

# 设置可执行权限
chmod +x Makefile

echo "工程目录创建完成！"
echo "请执行以下命令生成证书："
echo "  mkdir -p cert && openssl req -x509 -newkey rsa:4096 -nodes \\"
echo "      -keyout cert/server.key -out cert/server.crt -days 365 \\"
echo "      -subj '/CN=localhost'"
