/*
    disk_man.c  - eZ80L92 SD/FAT Disk Manager for CP/M 50 MkII
*/
/*-
 * Copyright (c) 2018 Jesse Marroquin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
#include <stdio.h>
#include <string.h>
#include "ff.h"

#define NUM_DRIVES 6

#define CTRL_C  0x03
#define CTRL_H  0x08
#define DELETE  0x7F

#define CLI_BUF_LEN 120

#define MAX_CLI_ARGS 4

extern int getch(void);
extern int putch(int);

typedef struct {
    char fname[40];
    FIL fp;
    // disk image details
    unsigned char spt;
    unsigned char sectlen;
    unsigned char resv;
} drive_t;

typedef struct {
    char *name;
    void (*func)(unsigned char, char**);
    char *help;
} cmd_t;

extern FATFS fs;
extern drive_t drive[];

static char clibuf[CLI_BUF_LEN];
static char *clibuflim = clibuf + CLI_BUF_LEN - 1;
static unsigned char done = 0;

void dskman_dir(unsigned char, char **);
void dskman_mount(unsigned char, char **);
void dskman_umount(unsigned char, char **);
void dskman_cd(unsigned char, char **);
void dskman_mkfs(unsigned char, char **);
void dskman_label(unsigned char, char **);
void dskman_exit(unsigned char, char **);
void dskman_help(unsigned char, char **);

const cmd_t cmds[] = {
    { "dir",    dskman_dir,    "[<path>][/][<pattern>]" },
    { "mount",  dskman_mount,  "[<path> <device>]" },
    { "umount", dskman_umount, "<device>" },
    { "cd",     dskman_cd,     "<path>" },
    { "mkfs",   dskman_mkfs,   "<path>" },
    { "label",  dskman_label,  ""},
    { "exit",   dskman_exit,   ""},
    { "help",   dskman_help,   ""},
    { "",       NULL,          ""}
};

unsigned char get_drvnum(char *str)
{
    unsigned char result;

     if (*(str + 1)) {
        return -1;
    }

    *str &= ~0x20;

    if (*str <= 'D') {
        result = *str - 'A';
    } else if ((*str == 'I') || (*str == 'J')) {
        result = *str - 'E';
    } else {
        result = -1;
    }

    return result;
}

void show_fileinfo(FILINFO *fno)
{
    printf("%02d/%02d/%4d",
           (fno->fdate >> 5) & 0xF,
           (fno->fdate & 0x1F),
           ((fno->fdate >> 9) & 0x7F) + 1980);

    printf("  %02d:%02d:%02d",
           (fno->ftime >> 11) & 0x1F,
           (fno->ftime >> 5) & 0x3F,
           fno->ftime & 0x1F);

    if (fno->fattrib & AM_DIR) {
        printf("           ");
        printf(" %s/\n", fno->fname);
    } else {
        printf("%11d", fno->fsize);
        printf(" %s\n", fno->fname);
    }
}

void dskman_dir(unsigned char argc, char **argv)
{
    FRESULT result;
    TCHAR *path = "";
    TCHAR *patt = NULL;
    DIR dir;
    FILINFO fno;

    if (argc > 2) {
        printf("too many args\n");
        return;
    }

    if (argc == 2) {
        path = argv[1];

        if ((patt = strrchr(argv[1], '/')) != NULL) {
            if (strcmp("/.", patt) && strcmp("/..", patt)) {
                *patt = '\0';

                if (*(patt + 1) != '\0') {
                    patt++;
                } else {
                    patt = NULL;
                }
            } else {
                patt = NULL;
            }
        } else if (strcmp(".", argv[1]) && strcmp("..", argv[1])) {
            patt = argv[1];
            path = "";
        }
    }

    if ((patt == NULL) && (result = f_opendir(&dir, path)) != FR_OK) {
        return;
    }

    result = (patt == NULL) ? f_readdir(&dir, &fno) : f_findfirst(&dir, &fno, path, patt);

    while ((result == FR_OK) && (*fno.fname != '\0')) {
        show_fileinfo(&fno);
        result = (patt == NULL) ? f_readdir(&dir, &fno) : f_findnext(&dir, &fno);
    }

    f_closedir(&dir);
}

void dskman_mount(unsigned char argc, char **argv)
{
    unsigned char i;
    unsigned char drvnum;

    if (argc == 1) {
        for (i = 0; i < NUM_DRIVES; i++) {
            if (drive[i].fp.obj.fs != 0) {
                printf("%s on %c\n", drive[i].fname, "ABCDIJ"[i]);
            }
        }
    } else if (argc == 3) {
        if ((drvnum = get_drvnum(argv[2])) < NUM_DRIVES) {
            if (drive[drvnum].fp.obj.fs != 0) {
                printf("drive busy\n");
                return;
            }

            if (f_open(&drive[drvnum].fp, argv[1], FA_READ | FA_WRITE)) {
                printf("failed to open disk image\n");
                *drive[drvnum].fname = '\0';
                return;
            }

            strncpy(drive[drvnum].fname, argv[1], 40);
        } else {
            printf("unknown device\n");
        }
    } else {
        printf("invalid args\n");
    }
}

void dskman_umount(unsigned char argc, char **argv)
{
    unsigned char drvnum;

    if (argc == 2) {
        if ((drvnum = get_drvnum(argv[1])) < NUM_DRIVES) {
            f_close(&drive[drvnum].fp);
        } else {
            printf("unknown device\n");
        }
    } else {
        printf("invalid args\n");
    }
}

void dskman_cd(unsigned char argc, char **argv)
{
    FRESULT result;

    if (argc == 2) {
        result = f_chdir(argv[1]);
    } else {
        printf("invalid args\n");
    }
}

void dskman_mkfs(unsigned char argc, char **argv)
{
    FRESULT result;
    FIL fp;
    char buf[128];
    unsigned char i;
    UINT bw;

    if (argc != 2) {
        printf("invalid or missing args\n");
        return;
    }

    if ((result = f_open(&fp, argv[1], FA_CREATE_NEW | FA_WRITE)) != FR_OK) {
        if (result == FR_EXIST) {
            printf("file exists; will not overwrite\n");
        } else {
            printf("could not create file\n");
        }
        return;
    }

    if ((result = f_expand(&fp, 77 * 26 * 128, 1)) != FR_OK) {
        printf("error expanding file\n");
        goto close_f;
    }

    if ((f_lseek(&fp, 2 * 26 * 128) != FR_OK) || (f_tell(&fp) != 2 * 26 * 128)) {
        printf("lseek failed\n");
        goto close_f;
    }

    memset(buf, 0xE5, sizeof(buf));

    // Only 16 sectors are allocated for directory entries.
    // Formatting the entire track should not break anything.
    for (i = 0; i < 26; i++) {
        if ((result = f_write(&fp, buf, sizeof(buf), &bw)) != FR_OK) {
            if (bw != sizeof(buf)) {
                printf("write error; disk full\n");
            } else {
                printf("write error\n");
            }
            break;
        }
    }

close_f:
    f_close(&fp);
}

void dskman_label(unsigned char argc, char **argv)
{
    FRESULT result;
    TCHAR lbl[12];
    DWORD vsn;

    if (argc == 1) {
        if ((result = f_getlabel("", lbl, &vsn)) == FR_OK) {
            if (*lbl) {
                printf("Volume is %s\n", lbl);
            } else {
                printf("Volume has no label.\n");
            }

            printf("Volume Serial Number is %04X-%04X\n",
                   (unsigned short)(vsn >> 16), (unsigned short)(vsn & 0xFFFF));
        }
    } else {
        printf("too many args\n");
    }
}

void dskman_exit(unsigned char argc, char **argv)
{
    done = 1;
}

void dskman_help(unsigned char argc, char **argv)
{
    unsigned char i;

    for (i = 0; *cmds[i].name; i++) {
        printf("%s %s\n", cmds[i].name, cmds[i].help);
    }
}

char *skip_space(char *str)
{
    while (*str && (*str == ' ')) {
        str++;
    }

    return str;
}

char *find_space(char *str)
{
    while (*str && (*str != ' ')) {
        str++;
    }

    return str;
}

void dskman_exec(cmd_t *cmds, char *cmdline)
{
    unsigned char argc;
    char *argv[MAX_CLI_ARGS];

    for (argc = 0; (argc < MAX_CLI_ARGS) && *cmdline; argc++) {
        cmdline = skip_space(cmdline);
        argv[argc] = cmdline;
        cmdline = find_space(cmdline);

        if (*cmdline == ' ') {
            *cmdline++ = '\0';
        }
    }

    while (*cmds->name) {
        if (!strcmp(cmds->name, argv[0])) {
            cmds->func(argc, argv);
            break;
        }

        cmds++;
    }

    if (!(*cmds->name)) {
        printf("%s: command not found\n", argv[0]);
    }
}

void dskman(void (*func)())
{
    char c;
    char *ptr;
    unsigned char ready;

    done = 0;

    while (!done) {
        f_getcwd(clibuf, sizeof(clibuf));
        printf("dskman:%s$ ", clibuf);

        ptr = clibuf;
        *ptr = '\0';
        ready = 0;

        while (!ready) {
            c = getch();
            switch (c) {
            case CTRL_C:
                *clibuf = '\0';
                // fall-through
            case '\n': // minicom
            case '\r': // picocom, windows? maybe both
                printf("\r\n");
                ready = 1;
                break;

            case DELETE:
            case CTRL_H:
                if (ptr > clibuf) {
                    *--ptr = '\0';
                    printf("\x08 \x08");
                }
                break;

            case '\t':
                c = ' ';
                // fall-through
            default:
                if ((ptr < clibuflim) && (c >= ' ') && (c <= '\x7E')) {
                    *ptr++ = c;
                    *ptr = '\0';
                    putch(c);
                }
                break;
            }
        }

        if (*clibuf) {
            dskman_exec(cmds, clibuf);
        }
    }
}
