# bazel-deps-checker / Bazel依赖检查器

**正在缓慢的开发中**
**It is currently in the process of being developed slowly.**

**一个用于分析Bazel依赖关系的工具，检查不必要的依赖和循环依赖。目前支持C++的target。在Ubuntu 22.04环境下开发与测试。**
**A tool to analyze Bazel dependencies, checking for unnecessary and circular dependencies. Currently supports C++ targets. Developed and tested on Ubuntu 22.04.**  

---

## 功能亮点 / Key Features

- **检查未使用的依赖** - 识别声明但未实际使用的依赖  
  **Check unused dependencies** - Identify declared but unused dependencies
- **检测循环依赖** - 发现可能导致构建失败的循环依赖链  
  **Detect circular dependencies** - Find dependency cycles that may break builds
- **多格式报告输出** - 支持控制台、Markdown、JSON和HTML格式  
  **Multi-format reports** - Console, Markdown, JSON and HTML outputs
- **测试目标分析** - 可选包含测试目标进行依赖检查  
  **Test target analysis** - Optionally include test targets in dependency checks
- **Ubuntu 22.04支持** - 在Ubuntu 22.04环境开发和测试  
  **Ubuntu 22.04 support** - Developed and tested on Ubuntu 22.04

---

## 使用示例 / Usage Examples

```bash
# 基础用法：分析工作区依赖
bazel-deps-analyzer -w /path/to/workspace

# 生成JSON格式报告
bazel-deps-analyzer -w . -f json

# 输出Markdown报告并包含测试目标
bazel-deps-analyzer -w . -o report.md -f markdown -t
