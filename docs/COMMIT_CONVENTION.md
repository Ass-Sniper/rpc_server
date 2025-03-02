# Git 提交规范指南

## 提交类型
### **完整类型对照表**
| 类型          | 作用域示例        | 语义范围                           | 版本号影响 |
|---------------|------------------|-----------------------------------|-----------|
| `fix`         | auth, logging    | 缺陷修复                           | PATCH+1   |
| `feat`        | api, ui          | 新功能                            | MINOR+1   |
| `refactor`    | core, utils      | 代码结构优化                       | -         |
| `chore`       | deps, config     | 维护性任务                         | -         |
| `docs`        | readme, javadoc  | 文档更新                          | -         |
| `test`        | unit, e2e        | 测试用例添加/修改                  | -         |
| `perf`        | database, cache  | 性能优化                          | PATCH+1   |
| `ci`          | github, jenkins  | CI 配置变更                       | -         |

## 作用域示例
```text
build    - 构建系统或依赖管理
rpc      - 远程过程调用模块
config   - 配置文件相关
docs     - 文档更新
...
```

## 完整示例
```text
feat(rpc): implement bidirectional streaming support

- Add ServerStreamingRPCHandler class
- Update protocol buffer definitions
- Add streaming unit tests
```
---

## **为什么需要规范注释？**
1. **自动生成 CHANGELOG**  
   ```bash
   conventional-changelog -p angular -i CHANGELOG.md -s
   ```
2. **语义化版本控制**  
   ![版本控制示意图](https://semver.org/semver.png)
3. **提高代码审查效率**  
   ```bash
   # 快速过滤特定类型提交
   git log --grep "^feat(rpc)"
   ```