/*
* Programming Assignment 02: ls-v1.5.0
* Adds colorized output based on file type to v1.4.0.
* Usage:
*       $ ./bin/ls
*       $ ./bin/ls -l
*       $ ./bin/ls -x
*       $ ./bin/ls -x /home /etc
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

extern int errno;

#define COL_SPACING 2

/* ANSI escape codes for colors */
#define COLOR_RESET   "\033[0m"
#define COLOR_BLUE    "\033[0;34m"
#define COLOR_GREEN   "\033[0;32m"
#define COLOR_RED     "\033[0;31m"
#define COLOR_PINK    "\033[0;35m"
#define COLOR_REVERSE "\033[7m"

/* The three display modes. Only one is active at a time. */
enum display_mode
{
    MODE_DEFAULT = 0,    /* down then across */
    MODE_LONG = 1,       /* -l */
    MODE_HORIZONTAL = 2  /* -x */
};

void do_ls(const char *dir, int mode);
int compare_names(const void *a, const void *b);
void display_columns(const char *dir, char **names, int count, int maxlen);
void display_horizontal(const char *dir, char **names, int count, int maxlen);
void display_long(const char *dir, char **names, int count);
void print_long(const char *dir, const char *name,
                int w_nlink, int w_user, int w_group, int w_size);
void print_colored(const char *dir, const char *name, int padding);
void mode_to_string(mode_t mode, char *str);
int num_width(long n);
int get_terminal_width(void);
int is_archive(const char *name);

int main(int argc, char *argv[])
{
    int mode = MODE_DEFAULT;
    int opt;

    while ((opt = getopt(argc, argv, "lx")) != -1)
    {
        switch (opt)
        {
        case 'l':
            mode = MODE_LONG;
            break;
        case 'x':
            mode = MODE_HORIZONTAL;
            break;
        default:
            fprintf(stderr, "Usage: %s [-l | -x] [dir...]\n", argv[0]);
            exit(1);
        }
    }

    if (optind == argc)
    {
        do_ls(".", mode);
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            printf("Directory listing of %s : \n", argv[i]);
            do_ls(argv[i], mode);
            puts("");
        }
    }
    return 0;
}

/* Returns the terminal width in characters; falls back to 80 */
int get_terminal_width(void)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
        return 80;
    return ws.ws_col;
}

/* Comparison function for qsort(): each argument is really a char ** */
int compare_names(const void *a, const void *b)
{
    const char *s1 = *(const char *const *)a;
    const char *s2 = *(const char *const *)b;
    return strcmp(s1, s2);
}

/* Returns 1 if the filename ends with .tar, .gz or .zip */
int is_archive(const char *name)
{
    size_t len = strlen(name);
    const char *exts[] = {".tar", ".gz", ".zip"};
    for (int i = 0; i < 3; i++)
    {
        size_t elen = strlen(exts[i]);
        if (len > elen && strcmp(name + len - elen, exts[i]) == 0)
            return 1;
    }
    return 0;
}

/*
 * Prints a filename in the color matching its type, then pads it with
 * "padding" spaces. Padding is printed OUTSIDE the color codes, because
 * escape codes take no screen space but would break %-*s width counting.
 */
void print_colored(const char *dir, const char *name, int padding)
{
    char path[4096];
    snprintf(path, sizeof(path), "%s/%s", dir, name);

    const char *color = "";
    struct stat st;
    if (lstat(path, &st) == 0)
    {
        mode_t m = st.st_mode;
        if (S_ISLNK(m))
            color = COLOR_PINK;
        else if (S_ISDIR(m))
            color = COLOR_BLUE;
        else if (S_ISCHR(m) || S_ISBLK(m) || S_ISSOCK(m) || S_ISFIFO(m))
            color = COLOR_REVERSE;
        else if (is_archive(name))
            color = COLOR_RED;
        else if (m & (S_IXUSR | S_IXGRP | S_IXOTH))
            color = COLOR_GREEN;
    }

    if (color[0] != '\0')
        printf("%s%s%s", color, name, COLOR_RESET);
    else
        printf("%s", name);

    for (int i = 0; i < padding; i++)
        putchar(' ');
}

/* Reads all names of directory "dir" into a dynamic array, sorts, displays */
void do_ls(const char *dir, int mode)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }

    char **names = NULL;
    int count = 0;
    int capacity = 0;
    int maxlen = 0;

    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        /* grow the array when it is full (doubling strategy) */
        if (count == capacity)
        {
            capacity = (capacity == 0) ? 16 : capacity * 2;
            char **tmp = realloc(names, capacity * sizeof(char *));
            if (tmp == NULL)
            {
                perror("realloc failed");
                break;
            }
            names = tmp;
        }

        names[count] = strdup(entry->d_name);
        if (names[count] == NULL)
        {
            perror("strdup failed");
            break;
        }

        int len = (int)strlen(entry->d_name);
        if (len > maxlen)
            maxlen = len;
        count++;
    }

    if (errno != 0)
    {
        perror("readdir failed");
    }
    closedir(dp);

    /* Sort alphabetically, after reading everything and before displaying */
    if (count > 1)
        qsort(names, count, sizeof(char *), compare_names);

    /* Choose the display function based on the mode flag */
    if (mode == MODE_LONG)
        display_long(dir, names, count);
    else if (mode == MODE_HORIZONTAL)
        display_horizontal(dir, names, count, maxlen);
    else
        display_columns(dir, names, count, maxlen);

    /* free every string, then the array itself */
    for (int i = 0; i < count; i++)
        free(names[i]);
    free(names);
}

/* Prints names in columns, "down then across" */
void display_columns(const char *dir, char **names, int count, int maxlen)
{
    if (count == 0)
        return;

    int term_width = get_terminal_width();
    int col_width = maxlen + COL_SPACING;

    int cols = term_width / col_width;
    if (cols < 1)
        cols = 1;
    if (cols > count)
        cols = count;

    int rows = (count + cols - 1) / cols; /* ceiling of count / cols */

    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            int idx = c * rows + r; /* down the column first */
            if (idx >= count)
                break;

            int next = (c + 1) * rows + r; /* is there an item after this one? */
            int len = (int)strlen(names[idx]);
            if (c + 1 < cols && next < count)
                print_colored(dir, names[idx], col_width - len); /* padded */
            else
                print_colored(dir, names[idx], 0); /* last in row: no padding */
        }
        printf("\n");
    }
}

/* Prints names left to right, wrapping when the line is full ("-x") */
void display_horizontal(const char *dir, char **names, int count, int maxlen)
{
    if (count == 0)
        return;

    int term_width = get_terminal_width();
    int col_width = maxlen + COL_SPACING;
    int pos = 0; /* how many characters are already on the current line */

    for (int i = 0; i < count; i++)
    {
        /* if this item would not fit on the current line, start a new one */
        if (pos > 0 && pos + col_width > term_width)
        {
            printf("\n");
            pos = 0;
        }
        int len = (int)strlen(names[i]);
        print_colored(dir, names[i], col_width - len);
        pos += col_width;
    }
    printf("\n");
}

/* Long listing: pass 1 measures widths and total, pass 2 prints */
void display_long(const char *dir, char **names, int count)
{
    int w_nlink = 0, w_user = 0, w_group = 0, w_size = 0;
    long total = 0;

    for (int i = 0; i < count; i++)
    {
        char path[4096];
        snprintf(path, sizeof(path), "%s/%s", dir, names[i]);

        struct stat st;
        if (lstat(path, &st) == -1)
            continue;

        total += st.st_blocks;

        int w = num_width((long)st.st_nlink);
        if (w > w_nlink)
            w_nlink = w;

        struct passwd *pw = getpwuid(st.st_uid);
        int lu = pw ? (int)strlen(pw->pw_name) : 1;
        if (lu > w_user)
            w_user = lu;

        struct group *gr = getgrgid(st.st_gid);
        int lg = gr ? (int)strlen(gr->gr_name) : 1;
        if (lg > w_group)
            w_group = lg;

        w = num_width((long)st.st_size);
        if (w > w_size)
            w_size = w;
    }

    /* st_blocks counts 512-byte units; ls shows 1K units */
    printf("total %ld\n", total / 2);

    for (int i = 0; i < count; i++)
        print_long(dir, names[i], w_nlink, w_user, w_group, w_size);
}

/* Returns how many digits a number has */
int num_width(long n)
{
    int w = 1;
    while (n >= 10)
    {
        n /= 10;
        w++;
    }
    return w;
}

/* Converts st_mode into a string like "drwxr-xr-x" */
void mode_to_string(mode_t mode, char *str)
{
    /* 1st character: file type */
    if (S_ISREG(mode))       str[0] = '-';
    else if (S_ISDIR(mode))  str[0] = 'd';
    else if (S_ISLNK(mode))  str[0] = 'l';
    else if (S_ISCHR(mode))  str[0] = 'c';
    else if (S_ISBLK(mode))  str[0] = 'b';
    else if (S_ISFIFO(mode)) str[0] = 'p';
    else if (S_ISSOCK(mode)) str[0] = 's';
    else                     str[0] = '?';

    /* owner permissions */
    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    str[3] = (mode & S_IXUSR) ? 'x' : '-';
    /* group permissions */
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';
    /* others permissions */
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';

    /* special bits: setuid, setgid, sticky */
    if (mode & S_ISUID) str[3] = (mode & S_IXUSR) ? 's' : 'S';
    if (mode & S_ISGID) str[6] = (mode & S_IXGRP) ? 's' : 'S';
    if (mode & S_ISVTX) str[9] = (mode & S_IXOTH) ? 't' : 'T';

    str[10] = '\0';
}

/* Prints one line of long listing for file "name" inside directory "dir" */
void print_long(const char *dir, const char *name,
                int w_nlink, int w_user, int w_group, int w_size)
{
    char path[4096];
    snprintf(path, sizeof(path), "%s/%s", dir, name);

    struct stat st;
    if (lstat(path, &st) == -1)
    {
        perror(path);
        return;
    }

    char perms[11];
    mode_to_string(st.st_mode, perms);

    struct passwd *pw = getpwuid(st.st_uid);
    struct group *gr = getgrgid(st.st_gid);

    /* ctime() gives "Sun Oct 11 10:30:45 2026\n"; we want "Oct 11 10:30" */
    char *t = ctime(&st.st_mtime);
    char timebuf[13];
    strncpy(timebuf, t + 4, 12);
    timebuf[12] = '\0';

    printf("%s %*lu %-*s %-*s %*ld %s ",
           perms,
           w_nlink, (unsigned long)st.st_nlink,
           w_user, pw ? pw->pw_name : "?",
           w_group, gr ? gr->gr_name : "?",
           w_size, (long)st.st_size,
           timebuf);

    /* the filename is colored by file type */
    print_colored(dir, name, 0);

    /* for symbolic links, also show where they point */
    if (S_ISLNK(st.st_mode))
    {
        char target[4096];
        ssize_t len = readlink(path, target, sizeof(target) - 1);
        if (len != -1)
        {
            target[len] = '\0';
            printf(" -> %s", target);
        }
    }
    printf("\n");
}
