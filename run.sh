#!/bin/bash

# 1. 编译 链接 生成可执行文件
g++ -I inc/ -o bin/stat-server src/*.cc
# 2. 执行
./bin/stat-server 