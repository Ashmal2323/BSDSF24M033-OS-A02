# OS Programming Assignment 02 - Report
**Roll No:** BSDSF24M033

## Feature 2: Long Listing Format (v1.1.0)

**Q1. What is the crucial difference between stat() and lstat()? When is lstat() more appropriate in ls?**

`stat()` follows symbolic links: if the path is a symlink, it returns information about the file the link points to. `lstat()` does not follow links: it returns information about the link itself (its type is S_IFLNK and its size is the length of the target path). In `ls`, `lstat()` is more appropriate because `ls -l` must show a symlink as a link (type `l`, with `name -> target`) and not as the file it points to. `stat()` would also fail on broken links, while `lstat()` still works. It also prevents infinite loops in recursive listing, since links to directories are not followed.

**Q2. How can bitwise operators and macros extract the file type and permissions from st_mode?**

`st_mode` packs the file type and the permission bits into one integer. To test a permission bit, AND the value with a mask: `mode & S_IRUSR` is non-zero if the owner-read bit is set. The same works for S_IWUSR, S_IXUSR, S_IRGRP, S_IWGRP, S_IXGRP, S_IROTH, S_IWOTH and S_IXOTH. For the file type, the type bits are isolated with the mask `S_IFMT`: `(mode & S_IFMT) == S_IFDIR` means the file is a directory. The macros `S_ISDIR(mode)`, `S_ISREG(mode)` and `S_ISLNK(mode)` perform this comparison for us. The special bits S_ISUID, S_ISGID and S_ISVTX are tested the same way and displayed as `s`/`S` and `t`/`T`.

## Feature 3: Column Display, Down Then Across (v1.2.0)

**Q1. Explain the general logic for printing items in a "down then across" format. Why is a single loop through the list insufficient?**

First all filenames are stored in an array and the longest name is found. The column width is `maxlen + 2`. The number of columns is `cols = terminal_width / column_width`, and the number of rows is `rows = ceil(count / cols)`. Then we loop over rows, and for each row we loop over columns, printing `names[c * rows + r]`, so column 0 holds items 0 to rows-1, column 1 holds the next `rows` items, and so on. A single loop over the array is insufficient because the screen is printed row by row, but the items of one screen row are not neighbours in the array; they are `rows` positions apart. A single loop would print the items left to right (across), not down each column first.

**Q2. What is the purpose of ioctl here? What are the limitations of a fixed 80-column fallback?**

`ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws)` asks the terminal driver for the current window size and fills a `struct winsize`; `ws.ws_col` gives the number of columns. This lets the program fit as many columns as the terminal can show and adapt when the window is resized. With only a fixed 80-column width, the program would waste space on wide terminals (fewer columns than possible) and on narrow terminals the lines would be longer than the window, so they wrap and break the column alignment. It would also not adapt to resizing.

## Feature 4: Horizontal Display -x (v1.3.0)

**Q1. Compare the complexity of the vertical ("down then across") and horizontal ("across") logic. Which needs more pre-calculation and why?**

The vertical logic needs more pre-calculation. It must first compute the number of columns and the number of rows (with a ceiling division), and then each item's array index depends on both row and column (`c * rows + r`), with extra checks for incomplete last columns. The horizontal logic only needs the column width and a running position counter: print each name padded, add the width to the counter, and when the next name would exceed the terminal width, print a newline and reset the counter. It walks the array in order with no row/column math.

**Q2. How did you manage the different display modes (-l, -x, default)?**

I used an `enum display_mode` with the values MODE_DEFAULT, MODE_LONG and MODE_HORIZONTAL, stored in a single variable `mode`. The `getopt()` loop sets it when it sees `-l` or `-x`. Because the modes are mutually exclusive, one variable is enough (if both are given, the last one wins). After `do_ls` has read and sorted the filenames, an if / else if / else chain checks `mode` and calls `display_long()`, `display_horizontal()` or `display_columns()`. The recursive option `-R` is a separate flag because it can be combined with any display mode.

## Feature 5: Alphabetical Sort (v1.4.0)

**Q1. Why must all entries be read into memory before sorting? What are the drawbacks for directories with millions of files?**

`readdir()` returns entries one at a time in an arbitrary order determined by the filesystem. A sort has to compare any entry with any other, so the last entry could belong first; therefore every name must be in memory before the sorted order is known. For a directory with millions of files this uses a large amount of memory (one string plus one pointer per file), sorting costs O(n log n) time, and no output appears until everything is read and sorted, so the program feels slow and might run out of memory. Unsorted streaming (like `ls -f`) avoids this.

**Q2. Explain the purpose and signature of the comparison function required by qsort(). Why const void * arguments?**

`qsort()` is a generic function that can sort any array, so it needs a function that tells it how to order two elements. The signature is `int compare(const void *a, const void *b)`. It must return a negative number if the first element should come first, zero if they are equal, and a positive number if the first should come after. `qsort` does not know the element type, so it passes untyped `const void *` pointers (const because the comparison must not modify the data). Each pointer points to an element of the array. Our elements are `char *`, so each argument is really a `char **`; we cast it and dereference once to get the string, then return `strcmp(s1, s2)`.

## Feature 6: Colorized Output (v1.5.0)

**Q1. How do ANSI escape codes produce color? Show the sequence for green text.**

ANSI escape codes are special character sequences starting with the ESC character (octal `\033`, value 27) followed by `[`, some parameters and a final letter `m`. The terminal does not print them; it interprets them as formatting commands that stay active for all following text until they are reset. To print green text: `printf("\033[0;32m" "text" "\033[0m");`. Here `\033[0;32m` switches the foreground colour to green and `\033[0m` resets everything back to the default.

**Q2. Which bits of st_mode determine whether a file is executable by owner, group or others?**

The execute bits: `S_IXUSR` (0100) for the owner, `S_IXGRP` (0010) for the group and `S_IXOTH` (0001) for others. A file is executable by the owner if `(st_mode & S_IXUSR)` is non-zero, by the group if `(st_mode & S_IXGRP)` is non-zero, and by others if `(st_mode & S_IXOTH)` is non-zero. My code tests `st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)`, which is true if any of the three is set, and then prints the name in green.

## Feature 7: Recursive Listing -R (v1.6.0)

**Q1. What is a base case? What is the base case in your recursive ls?**

A base case is the condition in a recursive function under which it stops calling itself and simply returns; without it the recursion never ends. In my `do_ls`, the recursion happens in a loop over the entries of the current directory, and a recursive call is made only for entries that are directories (checked with `lstat` and `S_ISDIR`, excluding `.` and `..`). When a directory contains no subdirectories, the loop makes no recursive call and the function returns: that is the base case. Also, if `opendir()` fails, `do_ls` prints an error and returns immediately. Because directory trees are finite, and symlinks are not followed (lstat) and `.`/`..` are skipped, the recursion always terminates.

**Q2. Why must a full path be constructed before the recursive call? What would happen with do_ls("subdir")?**

Directory and file names returned by `readdir()` are only bare names, not paths. System calls such as `opendir()` and `lstat()` resolve a relative name against the program's current working directory, not against the directory being listed. If `do_ls("subdir")` were called from inside `do_ls("parent_dir")`, `opendir("subdir")` would look for `subdir` in the current working directory, where it usually does not exist, so it would fail with "Cannot open directory"; or worse, if an unrelated folder with the same name existed there, it would list the wrong directory. Building `parent_dir/subdir` gives the correct location every time, no matter where the program is run from.
