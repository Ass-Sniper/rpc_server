CERT_DIR=/home/kay/codebase/test/rpc_server/cert
TOOLS_DIR=/home/kay/codebase/test/rpc_server/tools
mkdir -p $CERT_DIR

# 删除旧证书文件
rm -f $CERT_DIR/*.key $CERT_DIR/*.crt $CERT_DIR/*.csr $CERT_DIR/*.srl

# 定义颜色变量
GREEN="\033[32m"
RED="\033[31m"
RESET="\033[0m"

# 添加verbose参数处理
verbose=1
while getopts "v" opt; do
    case $opt in
        v)
            verbose=1
            ;;
        \?)
            echo "Invalid option: -$OPTARG" >&2
            exit 1
            ;;
    esac
done

# 修改openssl命令以支持verbose
openssl_command() {
    if [ $verbose -eq 1 ]; then
        openssl "$@"
    else
        openssl "$@" 2>/dev/null
    fi
}

# 1. 生成CA证书
openssl_command genrsa -out $CERT_DIR/ca.key 4096
# 1.1 创建CA证书
openssl_command req -new -x509 -days 3650 -key $CERT_DIR/ca.key -out $CERT_DIR/ca.crt -subj "/CN=MyCA"
if [ $verbose -eq 1 ]; then
    cp -fv $CERT_DIR/ca.crt $CERT_DIR/cert.pem
else
    cp -f $CERT_DIR/ca.crt $CERT_DIR/cert.pem >/dev/null 2>&1
fi

# 2. 生成服务器证书
openssl_command genrsa -out $CERT_DIR/server.key 4096
# 2.1 生成带SAN的CSR
openssl_command req -new -key $CERT_DIR/server.key -config $TOOLS_DIR/san.cnf \
    -out $CERT_DIR/server.csr -subj "/CN=localhost"
    # -new: 指示 OpenSSL 创建一个新的证书签名请求。
    # -key $CERT_DIR/server.key: 指定用于生成 CSR 的私钥文件路径。这里使用之前生成的服务器私钥文件
    # -config $TOOLS_DIR/san.cnf: 使用指定的配置文件，该配置文件包含扩展信息，如 SAN（Subject Alternative Name）字段。SAN 字段允许为同一证书指定多个域名或 IP 地址，这对于支持多个主机名的服务非常重要。
    # -out $CERT_DIR/server.csr: 输出生成的CSR文件路径
    # -subj "/CN=localhost": 指定证书的主题名称（Subject），其中 /CN=localhost 表示通用名称（Common Name）为 localhost。这通常用于标识证书所属的实体，在这里是服务器的主机名。
# 2.2 签发证书
openssl_command x509 -req -days 3650 -in $CERT_DIR/server.csr \
    -CA $CERT_DIR/ca.crt -CAkey $CERT_DIR/ca.key \
    -CAcreateserial \
    -extfile $TOOLS_DIR/san.cnf -extensions v3_req \
    -out $CERT_DIR/server.crt
    # -req: 指示 OpenSSL 处理证书签名请求（CSR）。
    # -days 3650: 设置证书的有效期为 3650 天。
    # -in $CERT_DIR/server.csr: 指定输入的 CSR 文件路径。
    # -CA $CERT_DIR/ca.crt: 指定用于签发证书的 CA 证书文件路径。
    # -CAkey $CERT_DIR/ca.key: 指定用于签发证书的 CA 私钥文件路径。
    # -CAcreateserial: 创建一个序列号文件，用于跟踪签发的证书数量。
    # -extfile $TOOLS_DIR/san.cnf: 使用指定的配置文件，该配置文件包含扩展信息，如 SAN（Subject Alternative Name）字段。
    # -extensions v3_req: 指定使用配置文件中的 v3_req 扩展部分。
    # -out $CERT_DIR/server.crt: 指定输出的证书文件路径。

# 3.验证服务器证书（server.crt）与私钥（server.key）匹配性
server_cert_modulus=$(openssl x509 -noout -modulus -in $CERT_DIR/server.crt | openssl sha256)
server_key_modulus=$(openssl rsa -noout -modulus -in $CERT_DIR/server.key | openssl sha256)

if [ "$server_cert_modulus" == "$server_key_modulus" ]; then
    echo -e "${GREEN}Server certificate and private key match${RESET}"
else
    echo -e "${RED}Server certificate and private key do not match${RESET}"
fi

# 4. 生成客户端证书（双向认证时使用）
# 4.1 生成客户端私钥
openssl_command genrsa -out $CERT_DIR/client.key 4096
# 4.2 生成客户端证书请求
openssl_command req -new -key $CERT_DIR/client.key -out $CERT_DIR/client.csr -subj "/CN=Test Client"
# 4.3 签发客户端证书
openssl_command x509 -req -days 365 -in $CERT_DIR/client.csr \
    -CA $CERT_DIR/ca.crt -CAkey $CERT_DIR/ca.key -set_serial 02 -out $CERT_DIR/client.crt
    # -req: 指示 OpenSSL 处理证书签名请求（CSR）。
    # -days 365: 设置证书的有效期为 365 天。
    # -in $CERT_DIR/client.csr: 指定输入的 CSR 文件路径。
    # -CA $CERT_DIR/ca.crt: 指定用于签发证书的 CA 证书文件路径。
    # -CAkey $CERT_DIR/ca.key: 指定用于签发证书的 CA 私钥文件路径。
    # -set_serial 02: 设置证书的序列号为 02。
    # -out $CERT_DIR/client.crt: 指定输出的证书文件路径。

# 5.验证客户端证书（client.crt）与CA私钥（ca.key）匹配性
# 5.1 提取客户端证书的签名信息
client_cert_signer=$(openssl x509 -noout -issuer -in $CERT_DIR/client.crt)
ca_subject=$(openssl x509 -noout -subject -in $CERT_DIR/ca.crt)

if [ "$client_cert_signer" == "$ca_subject" ]; then
    echo -e "${GREEN}Client certificate is signed by the CA${RESET}"
else
    echo -e "${RED}Client certificate is NOT signed by the CA${RESET}"
fi

# 5.2 验证CA私钥是否正确签名了客户端证书
openssl_command verify -CAfile $CERT_DIR/ca.crt $CERT_DIR/client.crt
if [ $? -eq 0 ]; then
    echo -e "${GREEN}Client certificate is correctly signed by the CA private key${RESET}"
else
    echo -e "${RED}Client certificate is NOT correctly signed by the CA private key${RESET}"
fi