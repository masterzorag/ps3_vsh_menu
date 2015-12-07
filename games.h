#ifndef __GAMES_H__
#define __GAMES_H__


#ifdef GLIBC
#include <stdlib.h>  // malloc, qsort, free
#include <stdio.h>   // sprintf
#include <string.h>
#include <dirent.h>

#define USERLIST_PATH "./"

#else
// include vsh exported functions
#include "inc/vsh_exports.h"  // opendir, readdir, closedir, malloc, memset, free, qsort

#define USERLIST_PATH "/dev_hdd0/GAMES/"

#endif


struct game_entry
{
    char *title;        // array to store real Title
    char *path;         // "/dev_hdd0/GAMES/BLES02161-[FIFA 16]"
};

int getDirListSize(const char *path);
struct game_entry *ReadUserList(int *gmc);

void send_wm_request(char *cmd);
void do_mount(char *path);
void do_umount(void);


#endif // __GAMES_H__