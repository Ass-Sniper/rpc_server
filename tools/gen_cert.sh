
CERT_DIR=/home/kay/codebase/test/rpc_server/cert
TOOLS_DIR=/home/kay/codebase/test/rpc_server/tools
mkdir -p $CERT_DIR

# 1. 生成CA证书
openssl genrsa -out $CERT_DIR/ca.key 4096
# 1.1 创建CA证书
openssl req -new -x509 -days 3650 -key $CERT_DIR/ca.key -out $CERT_DIR/ca.crt -subj "/CN=MyCA"
cp -fv $CERT_DIR/ca.crt $CERT_DIR/cert.pem

# 2. 生成服务器证书
openssl genrsa -out $CERT_DIR/server.key 4096
# 2.1 生成带SAN的CSR
openssl req -new -key $CERT_DIR/server.key -config $TOOLS_DIR/san.cnf \
    -out $CERT_DIR/server.csr -subj "/CN=localhost"
# 2.2 签发证书
openssl x509 -req -days 3650 -in $CERT_DIR/server.csr \
    -CA $CERT_DIR/ca.crt -CAkey $CERT_DIR/ca.key \
    -CAcreateserial \
    -extfile $TOOLS_DIR/san.cnf -extensions v3_req \
    -out $CERT_DIR/server.crt


# 3.验证服务器证书（server.crt）与私钥（server.key）匹配性
openssl x509 -noout -modulus -in $CERT_DIR/server.crt | openssl sha256
openssl rsa -noout -modulus -in $CERT_DIR/server.key | openssl sha256


# 4. 生成客户端证书（双向认证时使用）
# 4.1 生成客户端私钥
openssl genrsa -out $CERT_DIR/client.key 4096
# 4.2 生成客户端证书请求
openssl req -new -key $CERT_DIR/client.key -out $CERT_DIR/client.csr -subj "/CN=Test Client"
# 4.3 签发客户端证书
openssl x509 -req -days 365 -in $CERT_DIR/client.csr \
    -CA $CERT_DIR/ca.crt -CAkey $CERT_DIR/ca.key -set_serial 02 -out $CERT_DIR/client.crt
