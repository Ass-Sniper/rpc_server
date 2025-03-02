import requests
import json

url = "https://localhost:8443/api"
cert = ("client.crt", "client.key")  # 双向认证时使用

tests = [
    {
        "name": "正常请求",
        "data": {"jsonrpc": "2.0", "method": "MathService.add", "params": {"a": 3, "b": 4}},
        "expected": 200
    },
    {
        "name": "无效参数",
        "data": {"jsonrpc": "2.0", "method": "MathService.add", "params": {"a": "text"}},
        "expected": 400
    }
]

for test in tests:
    try:
        r = requests.post(
            url,
            json=test['data'],
            verify='../cert/cert.pem',
            # cert=cert  # 启用客户端证书认证
        )
        assert r.status_code == test['expected'], f"状态码错误: {r.status_code}"
        print(f"✓ {test['name']} 通过")
    except Exception as e:
        print(f"✗ {test['name']} 失败: {str(e)}")
