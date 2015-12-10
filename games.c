/*
    games.c, dealing with GAMES folder on HDD0

    Main data model taken from Artemis-PS3 project source
*/

#include "games.h"
#include "misc.h"


// taken from cobra source
static int parse_param_sfo(char *file, const char *field, char *title_name)
{
    FILE *fp = fopen(file, "rb");

    if(fp)
    {
      unsigned len, pos, str;
      unsigned char *mem = NULL;
      fseek(fp, 0, SEEK_END);
      len = ftell(fp);
      mem = (unsigned char *) malloc(len + 16);
      if(!mem)
      {
          fclose(fp);
          return -2;
      }

      memset(mem, 0, len + 16);
      fseek(fp, 0, SEEK_SET);
      fread((void *)mem, len, 1, fp);
      fclose(fp);

      str = (mem[8] + (mem[9] << 8));
      pos = (mem[0xc] + (mem[0xd] << 8));
      int indx = 0;
      while(str < len)
      {
          if(mem[str] == 0) break;

          if(!strcmp((char *) &mem[str], field))
          {
              strncpy(title_name, (char *) &mem[pos], 63);
              free(mem);
              return 0;
          }
          while (mem[str]) str++;

          str++;
          pos += (mem[0x1c + indx] + (mem[0x1d + indx] << 8));
          indx += 16;
      }
      if(mem) free(mem);
    }
    return -1;
}


/* qsort struct comparison function (C-string field) */
static int struct_cmp_by_title(const void *a, const void *b)
{
    struct game_entry *ia = (struct game_entry *)a;
    struct game_entry *ib = (struct game_entry *)b;
    return strcmp(ia->title, ib->title);
/* strcmp functions works exactly as expected from comparison function */
}


int getDirListSize(const char *path)
{
    struct dirent *dir;
    DIR *d = opendir(path);

    if(d)
    {
        int count = 0;
        while((dir = readdir(d)) != NULL)
        {
            if(strcmp(dir->d_name, "." ) != 0 
            && strcmp(dir->d_name, "..") != 0) count++;
        }
        closedir(d);

        return count;
    }
    else
        return -1;
}


struct game_entry *ReadUserList(short *gmc)
{
    char *userPath = (char*)USERLIST_PATH;

    int game_count = getDirListSize(userPath);

    *gmc = game_count;

    if(game_count < 1) return NULL; // error on USERLIST_PATH

    // build structs list
    struct game_entry *ret = (struct game_entry *)malloc(sizeof(struct game_entry) * game_count);
    memset(ret, 0, sizeof(struct game_entry) * game_count);

    DIR *d = opendir(userPath);

    if(d)
    {
        struct dirent *dir;
        int cur_count = 0, err = 0;
        char fullPath[256], title[128];

        while((dir = readdir(d)) != NULL)
        {
            if(strcmp(dir->d_name, ".")  != 0
            && strcmp(dir->d_name, "..") != 0)
            {
                // build full game path
                sprintf(fullPath, "%s%s", userPath, dir->d_name);

                ret[cur_count].path = (char *)malloc(strlen(dir->d_name) + 1);
                strcpy(ret[cur_count].path, dir->d_name);

                // parse SFO(fullPath/PS3_GAME/PARAM.SFO) for real title
                strcat(fullPath, "/PS3_GAME/PARAM.SFO");
                memset(title, 0, 128);
                char *p = NULL;
                err = parse_param_sfo(fullPath, "TITLE", (char *)&title);
                if(!err)
                    p = (char *)&title; // store real title
                else
                    p = dir->d_name;    // store folder name

                ret[cur_count].title = (char *)malloc(strlen(p) + 1);
                strcpy(ret[cur_count].title, p);
                cur_count++;
            }
        }
        closedir(d);

        /* resort using custom comparision function */
        qsort(ret, cur_count, sizeof(struct game_entry), struct_cmp_by_title);

        return ret;
    }
    else
        return NULL;
}


/***********************************************************************
* webMAN integration
*
* webMAN MOD now supports http requests
* Example: GET /mount.ps3/dev_hdd0/PS3ISO/MyGame.iso
***********************************************************************/
static int connect_to_webman(void)
{
    struct sockaddr_in sin;
    int s;

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = 0x7F000001; // 127.0.0.1 (localhost)
    sin.sin_port = htons(80);         // http port (80)
    s = socket(AF_INET, SOCK_STREAM, 0);

    if(s < 0) return -1;
    if(connect(s, (struct sockaddr*)&sin, sizeof(sin)) < 0) return -1;

    return s;
}

/* send command */
void send_wm_request(char *cmd)
{
    int conn_s = -1;
    conn_s = connect_to_webman();

    if(conn_s >= 0)
    {
        int res = send(conn_s, cmd, strlen(cmd), 0);
        if(res < 0) return;
    }
    buzzer(1);  // feedback
}

/***********************************************************************
* wrapper to mount path
***********************************************************************/
void do_mount(char *path)
{
    char request[256];
    strcpy((char*)&request, "GET /mount.ps3");

    // build full game path
    strcat((char*)&request, USERLIST_PATH);
    strcat((char*)&request, path);

    send_wm_request((char*)&request);
}

/***********************************************************************
* wrapper to unmount
***********************************************************************/
void do_umount(void)
{
    send_wm_request((char*)"GET /mount_ps3/unmount");
}

