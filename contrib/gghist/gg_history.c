/* Kazik <kazik.cz@interia.pl> */
/* License : GPL */

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <glib.h>

#include "gg_history.h"

char *getitem(int fd, int offset, char schr, char echr, int item, int cut)
{
    int	i	= 0,
	found	= 0,
	start	= -1-item,
	end	= -1-item;
    char *tmp	= calloc(2, sizeof(char));
    
    if(schr == '\0') start = 0;
    if(echr == '\n') end   = -1;
    
    lseek(fd, offset, 0);
    
    while(!found)
    {
	read(fd, tmp, 1);
	
	if(tmp[0] == schr)
	{
	    if(start == -1) start = i;
	    else if(start < -1) start++;
	}
	
	if(tmp[0] == echr)
	{
	    if((end == -1)&&(start != i)) end = i;
	    else if(end < -1) end++;
	}
	
	if((start > -1)&&(end > -1)) found++;
	i++;
    }
    
    free(tmp);
    if(!found) return NULL;
    
    tmp = calloc(end-start+1, sizeof(char));
    lseek(fd, offset+start+cut, 0);
    read(fd, tmp, end-start-cut);
    
    return tmp;
}

int lines_count(int fd)
{
    char *tmp = calloc(2, sizeof(char));
    int lines = 0;
    
    lseek(fd, 0, 0);
    while(1)
    {
	if(read(fd, tmp, 1) != 1) break;
	if(tmp[0] == '\n') lines++;
    }
    free(tmp);

    return lines++;
}

int get_lines(int fd, int *list)
{
    char *tmp = calloc(2, sizeof(char));

    int  i = 0,
	 pos = 0;
	

    list[0] = 0;
    lseek(fd, 0, 0);
    while(1)
    {
	if(read(fd, tmp, 1) != 1) break;
	if(tmp[0] == '\n')
	    list[1+i++] = pos+1;
	pos++;
    }
    free(tmp);
    
    return i;
}

struct gg_hist_line *formatline(int fd, int offset)
{
    struct gg_hist_line *hist_line = calloc(1, sizeof(struct gg_hist_line));
    int msg_type = 0;
    
    char *type = getitem(fd, offset, '\0', ',', 0, 0);
    
    if(strcmp(type, "chatsend") == 0)		msg_type = GG_HIST_SEND;
    else if(strcmp(type, "chatrcv") == 0)	msg_type = GG_HIST_RCV;
    else if(strcmp(type, "status") == 0)	msg_type = GG_HIST_STATUS;
    free(type);
    hist_line->type = msg_type;
    
    switch(msg_type)
    {
	case GG_HIST_SEND:
	{
	    char *tmp = NULL;
	    
	    tmp = getitem(fd, offset, ',', ',', 0, 1);
	    hist_line->id = tmp; // ID
	    tmp = getitem(fd, offset, ',', ',', 1, 1);
	    hist_line->nick = tmp; // NICK
	    tmp = getitem(fd, offset, ',', ',', 2, 1);
	    hist_line->timestamp1 = tmp; // TIMESTAMP1
	    tmp = getitem(fd, offset, ',', '\n', 3, 1);
	    hist_line->data = tmp; // DATA

	    break;
	}
	case GG_HIST_RCV:
	{
	    char *tmp = NULL;

	    tmp = getitem(fd, offset, ',', ',', 0, 1);
	    hist_line->id = tmp; // ID
	    tmp = getitem(fd, offset, ',', ',', 1, 1);
	    hist_line->nick = tmp; // NICK
	    tmp = getitem(fd, offset, ',', ',', 2, 1);
	    hist_line->timestamp1 = tmp; // TIMESTAMP1
	    tmp = getitem(fd, offset, ',', ',', 3, 1);
	    hist_line->timestamp2 = tmp; // TIMESTAMP2
	    tmp = getitem(fd, offset, ',', '\n', 4, 1);
	    hist_line->data = tmp; // DATA

	    break;
	}
	case GG_HIST_STATUS:
	{
	    char *tmp = NULL;
	    
	    tmp =  getitem(fd, offset, ',', ',', 0, 1);
	    hist_line->id = tmp; // ID
	    tmp = getitem(fd, offset, ',', ',', 1, 1);
	    hist_line->nick = tmp; // NICK
	    tmp = getitem(fd, offset, ',', ',', 2, 1);
	    hist_line->ip = tmp; // IP
	    tmp = getitem(fd, offset, ',', ',', 3, 1);
	    hist_line->timestamp1 = tmp; // TIMESTAMP1
	    tmp = getitem(fd, offset, ',', '\n', 4, 1);
	    hist_line->data = tmp; // DATA
	    
	    break;
	}
    }
    
    return hist_line;
}

gchar *gg_convert_utf8(char *data)
{
    gchar *tmp = g_convert(data, strlen(data), "UTF-8", "ISO8859-2", NULL, NULL, NULL);
    
    if(g_utf8_validate(tmp, strlen(tmp), NULL) == FALSE)
    {
	g_free(tmp);
	tmp = g_convert(data, strlen(data), "UTF-8", "CP1250", NULL, NULL, NULL);
    }
    
    if(g_utf8_validate(tmp, strlen(tmp), NULL) == FALSE)
	g_warning("Could not convert to UTF-8!\n");
    
    return tmp;
}

gchar *gg_hist_time(int timestamp)
{
    gchar   gtmp[32],
	    *gtmp2;
    
    struct tm *ptm = (struct tm *)localtime((time_t *)&timestamp);
    strftime(gtmp, sizeof(gtmp), "%H:%M:%S (%Y-%m-%d)", ptm);
    free(ptm);
    
    gtmp2 = g_strdup_printf("%s", gtmp);
    
    return gtmp2;
}
