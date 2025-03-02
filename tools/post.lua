wrk.method = "POST"
wrk.headers["Content-Type"] = "application/json"
wrk.body = '{"jsonrpc":"2.0","method":"mathService.add","params":{"a":1,"b":2}}'
