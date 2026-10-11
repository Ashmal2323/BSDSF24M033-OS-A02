/*
* Programming Assignment 02: ls-v1.2.0
* Adds default column display ("down then across") to v1.1.0.
* Usage:
*       $ ./bin/ls
*       $ ./bin/ls -l
*       $ ./bin/ls /home /etc
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

void do_ls(const char *dir, int long_flag);
void display_columns(char **names, int count, int maxlen);
void display_long(const char *dir, char **names, int count);
void print_long(const char *dir, const char *name,
                int w_nlink, int w_user, int w_group, int w_size);
void mode_to_string(mode_t mode, char *str);
int num_width(long n);
int get_terminal_width(void);

int main(int argc, char *argv[])
{
    int long_flag = 0;
    int opt;

    while ((opt = getopt(argc, argv, "l")) != -1)
    {
        switch (opt)
        {
        case 'l':
            long_flag = 1;
            break;
        default:
            fprintf(stderr, "Usage: %s [-l] [dir...]\n", argv[0]);
            exit(1);
        }
    }

    if (optind == argc)
    {
        do_ls(".", long_flag);
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            printf("Directory listing of %s : \n", argv[i]);
            do_ls(argv[i], long_flag);
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

/* Reads all names of directory "dir" into a dynamic array, then displays */
void do_ls(const char *dir, int long_flag)
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

    if (long_flag)
        display_long(dir, names, count);
    else
        display_columns(names, count, maxlen);

    /* free every string, then the array itself */
    for (int i = 0; i < count; i++)
        free(names[i]);
    free(names);
}

/* Prints names in columns, "down then across" */
void display_columns(char **names, int count, int maxlen)
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
            if (c + 1 < cols && next < count)
                printf("%-*s", col_width, names[idx]); /* padded */
            else
                printf("%s", names[idx]); /* last in row: no trailing spaces */
        }
        printf("\n");
    }
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

    printf("%s %*lu %-*s %-*s %*ld %s %s",
           perms,
           w_nlink, (unsigned long)st.st_nlink,
           w_user, pw ? pw->pw_name : "?",
           w_group, gr ? gr->gr_name : "?",
           w_size, (long)st.st_size,
           timebuf,
           name);

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
