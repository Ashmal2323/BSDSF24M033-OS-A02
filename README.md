# 📂 BSDSF24M033-OS-A02: Re-engineering the Unix `ls` Command

![Language](https://img.shields.io/badge/language-C-blue)
![Platform](https://img.shields.io/badge/platform-Linux-green)
![Build](https://img.shields.io/badge/build-make-orange)
![Version](https://img.shields.io/badge/version-1.6.0-purple)

**Operating Systems, Programming Assignment 02**
**Roll No:** BSDSF24M033 | **GitHub:** [Ashmal2323](https://github.com/Ashmal2323)

---

## 📖 About the Project

This project rebuilds the core of the standard Linux `ls` command from scratch in C. It starts from a minimal `ls` (v1.0.0) that only prints filenames, and grows it feature by feature into a working `ls` clone. Each feature was developed on its own Git branch, merged into `main`, tagged, and published as a GitHub Release with the compiled binary attached.

The goal is to practise system programming: file-system system calls, dynamic memory, sorting, terminal I/O, recursion, build automation with `make`, and a disciplined Git workflow.

---

## ✨ Features

| Version | Feature | Description |
|---|---|---|
| v1.0.0 | Base `ls` | Lists non-hidden files, one per line (instructor's starter code) |
| v1.1.0 | `-l` long listing | Permissions, link count, owner, group, size, modified time, name, `total` line, dynamic column alignment, symlink targets |
| v1.2.0 | Column display | Default output in multiple columns, "down then across", adapts to terminal width via `ioctl` |
| v1.3.0 | `-x` horizontal | Left-to-right layout that wraps at the terminal width |
| v1.4.0 | Alphabetical sort | Entries sorted with `qsort()` and `strcmp()` in every display mode |
| v1.5.0 | Colorized output | File names colored by type using ANSI escape codes |
| v1.6.0 | `-R` recursive | Recursive listing with directory headers; combines with `-l` and `-x` |

### 🎨 Color scheme

| File type | Color |
|---|---|
| Directory | Blue |
| Executable file | Green |
| Archive (`.tar`, `.gz`, `.zip`) | Red |
| Symbolic link | Pink (magenta) |
| Special file (device, socket, FIFO) | Reverse video |
| Regular file | Default |

---

## 🗂️ Project Structure

```
BSDSF24M033-OS-A02/
├── src/
│   └── ls-v1.0.0.c     # Complete source code (all features)
├── bin/                # Compiled executable (bin/ls)
├── obj/                # Object files directory
├── man/                # Reserved for the man page (bonus)
├── Makefile            # Build script
├── README.md           # This file
└── REPORT.md           # Answers to the report questions
```

---

## 🔧 Build and Run

**Requirements:** Linux (developed on Kali Linux), `gcc` and `make`.

```bash
# Clone the repository
git clone https://github.com/Ashmal2323/BSDSF24M033-OS-A02.git
cd BSDSF24M033-OS-A02

# Build
make

# Run
./bin/ls

# Remove build output
make clean
```

The Makefile compiles with `-Wall -Wextra -g`, and the build produces no warnings.

---

## 🚀 Usage

```bash
./bin/ls [-l | -x] [-R] [directory ...]
```

| Command | What it does |
|---|---|
| `./bin/ls` | Column display, sorted, colored |
| `./bin/ls -l` | Long listing format |
| `./bin/ls -x` | Horizontal (across) display |
| `./bin/ls -R` | Recursive listing |
| `./bin/ls -lR /tmp` | Recursive long listing |
| `./bin/ls -xR src` | Recursive horizontal listing |
| `./bin/ls /etc /home` | List several directories |

### Example: sorted output

```
$ ./bin/ls /tmp/sorttest
10      2       Apple   apple   banana  cherry  zebra
```

Names are compared with `strcmp()`, so ordering is by ASCII value: digits, then uppercase, then lowercase (the same as `LC_ALL=C ls`).

### Example: recursive long listing

```
$ ./bin/ls -lR /tmp/rtest
/tmp/rtest:
total 0
drwxrwxr-x 3 kali kali 80 Oct 11 20:02 a
drwxrwxr-x 2 kali kali 60 Oct 11 20:02 d
-rw-rw-r-- 1 kali kali  0 Oct 11 20:02 top.txt

/tmp/rtest/a:
total 0
drwxrwxr-x 3 kali kali 80 Oct 11 20:02 b
-rw-rw-r-- 1 kali kali  0 Oct 11 20:02 one.txt
...
```

---

## 🧠 How It Works

1. **Argument parsing:** `getopt()` reads `-l`, `-x` and `-R`. The display mode (`enum display_mode`: default, long, horizontal) is stored in one variable, and `-R` is a separate flag so it can combine with any mode. `optind` marks where the directory names begin.
2. **Reading:** `do_ls()` opens the directory with `opendir()`, reads entries with `readdir()` and stores copies (`strdup`) in a dynamic array that doubles in size with `realloc()`. Hidden files (names starting with `.`) are skipped. The longest name is tracked.
3. **Sorting:** `qsort()` sorts the array using a comparison function that casts its `const void *` arguments to `char **` and calls `strcmp()`.
4. **Display:** one of three functions is chosen from the mode flag:
   - `display_columns()`: `cols = terminal_width / (maxlen + 2)`, `rows = ceil(count / cols)`, and the item at row `r`, column `c` is `names[c * rows + r]`.
   - `display_horizontal()`: keeps a running position counter and starts a new line when the next item would not fit.
   - `display_long()`: first pass finds the widest value in each column and the block total, second pass prints each line with `%*s` dynamic widths.
5. **Colors:** `print_colored()` calls `lstat()`, picks a color from `st_mode`, prints the name inside ANSI escape codes, then prints the padding outside them so column alignment stays correct.
6. **Recursion:** with `-R`, `do_ls()` prints a `dir:` header, and after displaying a directory it builds the full path (`parent/child`) of each entry, checks `lstat()` + `S_ISDIR()`, and calls itself. The base case is a directory with no subdirectories.
7. **Cleanup:** every `strdup`'d string and the array itself are freed.

---

## 🛠️ System Calls and Library Functions Used

| Purpose | Functions |
|---|---|
| Directory access | `opendir()`, `readdir()`, `closedir()` |
| File metadata | `lstat()` (and `stat()` concepts), `st_mode`, `st_nlink`, `st_size`, `st_blocks`, `st_mtime` |
| Name resolution | `getpwuid()`, `getgrgid()` |
| Symbolic links | `readlink()` |
| Time formatting | `ctime()` |
| Terminal size | `ioctl()` with `TIOCGWINSZ` |
| Option parsing | `getopt()` |
| Memory | `realloc()`, `strdup()`, `free()` |
| Sorting | `qsort()`, `strcmp()` |
| Permission bits | `S_IRUSR`, `S_IWUSR`, `S_IXUSR`, `S_ISUID`, `S_ISGID`, `S_ISVTX`, `S_ISDIR()`, `S_ISLNK()`, … |

---

## 🌿 Git Workflow

Every feature follows the same cycle: create a branch from `main`, implement and test, commit, merge into `main`, create an annotated tag, push the branch and tag, then publish a GitHub Release with the compiled binary.

| Feature | Branch | Tag |
|---|---|---|
| Long listing | `feature-long-listing-v1.1.0` | `v1.1.0` |
| Column display | `feature-column-display-v1.2.0` | `v1.2.0` |
| Horizontal display | `feature-horizontal-display-v1.3.0` | `v1.3.0` |
| Alphabetical sort | `feature-alphabetical-sort-v1.4.0` | `v1.4.0` |
| Colorized output | `feature-colorized-output-v1.5.0` | `v1.5.0` |
| Recursive listing | `feature-recursive-listing-v1.6.0` | `v1.6.0` |

📦 **Releases:** https://github.com/Ashmal2323/BSDSF24M033-OS-A02/releases

---

## 📝 Report

The answers to all the report questions (two per feature) are in [REPORT.md](REPORT.md).

---

## ⚠️ Known Limitations

- Hidden files are not shown, because `-a` is not implemented.
- The modification time is always shown as `Mon DD HH:MM`; the real `ls` shows the year for files older than six months.
- Colors are always on, even when output is piped; the real `ls` checks `isatty()` first.
- All columns share one width (longest name + 2), while the real `ls` sizes each column separately.
- Sorting is by ASCII value, not by locale, so uppercase names come before lowercase.

---

## 👤 Author

**Ashmal**, Roll No. **BSDSF24M033**
GitHub: [@Ashmal2323](https://github.com/Ashmal2323)

*Operating Systems, Programming Assignment 02. Instructor: Muhammad Arif Butt, PhD.*
