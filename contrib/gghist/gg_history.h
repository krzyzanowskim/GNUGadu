/* Kazik <kazik.cz@interia.pl> */
/* License : GPL */

#ifndef GG_HISTORY_SUPPORT
#define GG_HISTORY_SUPPORT

#include <sys/types.h>
#include <unistd.h>
#include <glib.h>

#define GG_HIST_SEND 1
#define GG_HIST_RCV 2 
#define GG_HIST_STATUS 3

#define SHOW_STATUS

struct gg_hist_line
{
    char *id;
    char *nick;
    char *ip;
    char *timestamp1;
    char *timestamp2;
    char *data;
    int type;
};

char *getitem(int fd, int offset, char schr, char echr, int item, int cut);
int lines_count(int fd);
int get_lines(int fd, int *list);
struct gg_hist_line *formatline(int fd, int offset);
gchar *gg_convert_utf8(char *data);
gchar *gg_hist_time(int timestamp);

#endif
