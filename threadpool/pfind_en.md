# pfind User Guide

`pfind` is a multithreaded file search tool based on `pthread`. It supports:

* Recursive search in the specified directory and all its subdirectories
* File name matching via **wildcards** (`*`, `?`) or **regular expressions**
* Parallel acceleration using a thread pool
* Output results to the console or a file

---

## Examples

1. **Search for C source files and write results to a log file**

   ```bash
   pfind -p /home -n "*.c" -o c_files.log
   ```

2. **Find all test scripts using a regex and print to console**

   ```bash
   pfind -p ./tests -r ".*_test\.py$"
   ```

3. **On Windows, find specific log files**

   ```bash
    pfind -p C:\logs -n "*.log" -o logs.txt
    ```

---

## Download & Installation

1. Download the precompiled binary (e.g., `pfind-MacOS`) from the GitHub Releases page.
2. Grant execute permission (Linux/macOS):

   ```bash
   chmod +x pfind-MacOS
   ```
3. (Optional) Move it to a directory in your `$PATH`:

   ```bash
   sudo mv pfind-MacOS /usr/local/bin/pfind
   ```

---

## Quick Usage

* Search for `.c` files in the current directory and subdirectories:

  ```bash
  pfind -n "*.c"
  ```

* Use regex to find files in `/home/user/project` starting with `test_` and ending in `.py`:

  ```bash
  pfind -p /home/user/project -r "^test_.*\.py$"
  ```

* If both wildcard and regex are specified, regex takes precedence:

  ```bash
  pfind -p ./src -n "*.js" -r "^app[0-9]+\.js$"
  ```

* Write output to `out.txt` (defaults to current directory if no path is given):

  ```bash
  pfind -n "*.log" -o out.txt
  ```

---

## Command-Line Options

```text
Usage: pfind [options]

  -p, --path <path>       Specify root directory to search (default: current directory ".")
  -n, --name <pattern>    Match file names using wildcards '*' (any characters) and '?' (single character)
  -r, --regex <pattern>   Match file names using POSIX extended regular expressions
  -o, --output <file>     Append matching results to the given output file (default: print to console)
  -h, --help              Show this help message and exit
```

* **Wildcard Matching** (`-n`)

    * `*` matches any number of characters
    * `?` matches a single character
    * Example: `"data_??.csv"` matches `data_01.csv`, `data_A2.csv`, etc.

* **Regex Matching** (`-r`)

    * POSIX extended regex syntax
    * Example: `"^test_[0-9]{3}\.c$"` matches `test_001.c`, `test_123.c` but not `test_12.c`

* **Precedence**

    * If both `-r` and `-n` are specified, regex (`-r`) takes priority and `-n` is ignored.

---

## Exit Codes

* `0`: Search completed successfully
* Non-zero: An error occurred, such as:

    * Missing or invalid arguments
    * Regex compilation failed
    * Failed to open directory or file

---

## Notes

* **Thread Count**: Default max threads is 30, min threads is 3. You can change this by editing `ThreadPoolCreate(30, 3, 100)` in the source code.
* **Queue Capacity**: Default task queue size is 100. If you encounter "queue full" errors, adjust it in the source code and recompile.
* **File Output**: Output is appended to the file. Be cautious of file size and duplicate results.

---