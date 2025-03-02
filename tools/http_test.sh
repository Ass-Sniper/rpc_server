#!/bin/bash
# https_test.sh
SERVER="localhost"
PORT=8443
CERT_FILE="../cert/cert.pem"
TEST_DATA='{"jsonrpc":"2.0","method":"mathService.add","params":{"a":5,"b":3}}'

echo "=== SSL/TLS 握手测试 ==="
openssl -d s_client -connect $SERVER:$PORT -showcerts -servername $SERVER < /dev/null 2>&1 | grep -E "SSL-Session|Protocol|Cipher"

echo -e "\n=== 证书验证测试 ==="
curl -v --cacert $CERT_FILE -v https://$SERVER:$PORT 2>&1 | grep -E "SSL connection|HTTP"

echo -e "\n=== 基本功能测试 ==="
echo "GET 测试:"
curl -v --cacert $CERT_FILE -s -o /dev/null -w "HTTP状态码: %{http_code}\n" https://$SERVER:$PORT

echo -e "\nPOST 测试:"
curl -v --cacert $CERT_FILE -X POST -H "Content-Type: application/json" \
-d "$TEST_DATA" -s -o response.json -w "HTTP状态码: %{http_code}\n" \
https://$SERVER:$PORT/api

echo -e "\n响应内容:"
jq . response.json

echo -e "\n=== 异常情况测试 ==="
echo "无效证书测试:"
curl -v --cacert bad_cert.pem -v https://$SERVER:$PORT 2>&1 | grep "SSL certificate problem"

echo -e "\n错误方法测试:"
curl -v --cacert $CERT_FILE -X PUT -d "{}" -s -o /dev/null -w "HTTP状态码: %{http_code}\n" \
https://$SERVER:$PORT/api

echo -e "\n=== 性能测试 ==="
echo "简单压力测试 (10个并发, 100请求):"
wrk -t 2 -c 10 -d 10s --timeout 2s -s post.lua https://$SERVER:$PORT/api

cat <<EOF > post.lua
wrk.method = "POST"
wrk.headers["Content-Type"] = "application/json"
wrk.body = '{"jsonrpc":"2.0","method":"mathService.add","params":{"a":1,"b":2}}'
EOF

echo -e "\n=== Python 验证脚本 ==="
cat <<EOF > https_test.py
import requests
import json

url = "https://$SERVER:$PORT/api"
cert = ("client.crt", "client.key")  # 双向认证时使用

tests = [
    {
        "name": "正常请求",
        "data": {"jsonrpc": "2.0", "method": "mathService.add", "params": {"a": 3, "b": 4}},
        "expected": 200
    },
    {
        "name": "无效参数",
        "data": {"jsonrpc": "2.0", "method": "mathService.add", "params": {"a": "text"}},
        "expected": 400
    }
]

for test in tests:
    try:
        r = requests.post(
            url,
            json=test['data'],
            verify='$CERT_FILE',
            # cert=cert  # 启用客户端证书认证
        )
        assert r.status_code == test['expected'], f"状态码错误: {r.status_code}"
        print(f"✓ {test['name']} 通过")
    except Exception as e:
        print(f"✗ {test['name']} 失败: {str(e)}")
EOF

echo -e "\n执行 Python 测试:"
python3 https_test.py