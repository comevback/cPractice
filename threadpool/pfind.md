# pfind 使用说明

`pfind` 是一个基于 pthread 的并行文件搜索工具，支持：  
- 在指定目录及其子目录递归查找  
- 对文件名做正则表达式或通配符（`*`、`?`）匹配  
- 多线程并发加速  
- 可将结果输出到屏幕或文件  

---

## 示例

1. **搜索 C 源文件并写日志**

   ```bash
   pfind -p /home -n "*.c" -o c_files.log
   ```

2. **查找所有测试脚本（正则）并打印在控制台**

   ```bash
   pfind -p ./tests -r ".*_test\.py$"
   ```
   
3. **在Windows系统下查找特定格式的日志文件**

   ```bash
    pfind -p C:\logs -n "*.log" -o logs.txt
    ```
   
---

## 下载与安装

1. 从 GitHub Releases 页面下载预编译二进制（例如 `pfind-MacOS`）。  
2. 赋可执行权限（Linux/macOS）：

   ```bash
   chmod +x pfind-MacOS
   ```
3. （可选）将其复制到你的 `$PATH` 目录：

   ```bash
   sudo mv pfind-MacOS /usr/local/bin/pfind
   ```

---

## 用法简介

* 在当前目录（和子目录）查找所有后缀为 `.c` 的文件：

  ```bash
  pfind -n "*.c"
  ```
* 在 `/home/user/project`（和子目录） 中，对文件名做正则匹配，查找以 `test_` 开头且以 `.py` 结尾的文件：

  ```bash
  pfind -p /home/user/project -r "^test_.*\.py$"
  ```
* 同时指定通配符和正则时，以正则优先：

  ```bash
  pfind -p ./src -n "*.js" -r "^app[0-9]+\.js$"
  ```
* 将结果写入 `out.txt`（未定义路径则默认当前路径）：

  ```bash
  pfind -n "*.log" -o out.txt
  ```

---

## 命令行选项

```text
Usage: pfind [options]

  -p, --path <path>       指定要搜索的根目录，默认为当前目录（"."）
  -n, --name <pattern>    按通配符匹配文件名，支持 '*'（任意长度）和 '?'（单字符）
  -r, --regex <pattern>   按 POSIX 扩展正则表达式匹配文件名
  -o, --output <file>     将匹配结果写入指定文件（追加模式），默认输出到标准输出
  -h, --help              显示本帮助信息并退出
```

* **通配符模式** (`-n`)

    * `*` 匹配任意长度的任意字符
    * `?` 匹配任意单个字符
    * 例如：`"data_??.csv"` 会匹配 `data_01.csv`、`data_A2.csv` 等

* **正则表达式** (`-r`)

    * 使用 POSIX 扩展语法
    * 例如：`"^test_[0-9]{3}\.c$"` 会匹配 `test_001.c`、`test_123.c`，但不匹配 `test_12.c`

* **优先级**

    * 如果同时指定了 `-r`，则忽略 `-n`，仅使用正则匹配。

---

## 退出状态

* `0`：成功完成搜索
* 非 `0`：发生错误，例如：

    * 参数缺失或无效
    * 正则编译失败
    * 打开目录或文件失败

---

## 注意事项

* **线程数** 最大线程数固定为 30，最小线程数固定为3，你也可以在源码中修改 `ThreadPoolCreate(30,3,100)` 里的 `30` 来调整。
* 默认队列容量为 100，如果遇到“队列已满”错误，可修改源码或重编译时调整。
* 输出到文件时会以追加模式打开，请留意文件大小及重复匹配。

---