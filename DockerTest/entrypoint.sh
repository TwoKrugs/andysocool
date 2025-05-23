#!/bin/bash

# 啟動 server 在背景
./server &
SERVER_PID=$!

# 等待一下 server 起來（模擬等待網路）
sleep 1

# 啟動 client（前景）
./client

# 等待 server 結束
wait $SERVER_PID