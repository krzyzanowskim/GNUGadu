/*
   Author  : Kazik <kazik.cz@interia.pl> 
	     GNU Gadu Team (c) 2004
   License : GPL 
*/

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
	int i = 0;
	int found = 0;
	int start = -1 - item;
	int end = -1 - item;
	char *tmp = g_new0(char, 2);

	if (schr == '\0')
		start = 0;
	if (echr == '\n')
		end = -1;

	lseek(fd, offset, 0);

	while (!found)
	{
		read(fd, tmp, 1);

		if (tmp[0] == schr)
		{
			if (start == -1)
				start = i;
			else if (start < -1)
				start++;
		}

		if (tmp[0] == echr)
		{
			if ((end == -1) && (start != i))
				end = i;
			else if (end < -1)
				end++;
		}

		if ((start > -1) && (end > -1))
			found++;
		i++;
	}

	g_free(tmp);
	if (!found)
		return NULL;

	tmp = g_new0(char, end - start + 1);
	lseek(fd, offset + start + cut, 0);
	read(fd, tmp, end - start - cut);

	return tmp;
}

int lines_count(int fd)
{
	char *tmp = g_new0(char, 2);
	int lines = 0;

	lseek(fd, 0, 0);
	while (1)
	{
		if (read(fd, tmp, 1) != 1)
			break;
		if (tmp[0] == '\n')
			lines++;
	}
	g_free(tmp);

	return lines++;
}

int get_lines(int fd, int *list)
{
	char *tmp = g_new0(char, 2);

	int i = 0;
	int pos = 0;

	list[0] = 0;
	lseek(fd, 0, 0);
	
	while (1)
	{
		if (read(fd, tmp, 1) != 1)
			break;
		if (tmp[0] == '\n')
			list[1 + i++] = pos + 1;
		pos++;
	}
	
	g_free(tmp);

	return i;
}

struct gg_hist_line *formatline(int fd, int offset)
{
	int msg_type = 0;
	struct gg_hist_line *hist_line = g_new0(struct gg_hist_line, 1);
	char *type = getitem(fd, offset, '\0', ',', 0, 0);

	if (!strcmp(type, "chatsend"))
		msg_type = GG_HIST_SEND;
	else if (!strcmp(type, "chatrcv"))
		msg_type = GG_HIST_RCV;
	else if (!strcmp(type, "status"))
		msg_type = GG_HIST_STATUS;

	g_free(type);
	hist_line->type = msg_type;

	switch (msg_type)
	{
	case GG_HIST_SEND:
		{
			hist_line->id = getitem(fd, offset, ',', ',', 0, 1);	// ID
			hist_line->nick = getitem(fd, offset, ',', ',', 1, 1);	// NICK
			hist_line->timestamp1 = getitem(fd, offset, ',', ',', 2, 1);	// TIMESTAMP1
			hist_line->data = getitem(fd, offset, ',', '\n', 3, 1);	// DATA
			break;
		}
	case GG_HIST_RCV:
		{
			hist_line->id = getitem(fd, offset, ',', ',', 0, 1);	// ID
			hist_line->nick = getitem(fd, offset, ',', ',', 1, 1);	// NICK
			hist_line->timestamp1 = getitem(fd, offset, ',', ',', 2, 1);	// TIMESTAMP1
			hist_line->timestamp2 = getitem(fd, offset, ',', ',', 3, 1);	// TIMESTAMP2
			hist_line->data = getitem(fd, offset, ',', '\n', 4, 1);	// DATA
			break;
		}
	case GG_HIST_STATUS:
		{
			hist_line->id = getitem(fd, offset, ',', ',', 0, 1);	// ID
			hist_line->nick = getitem(fd, offset, ',', ',', 1, 1);	// NICK
			hist_line->ip = getitem(fd, offset, ',', ',', 2, 1);	// IP
			hist_line->timestamp1 = getitem(fd, offset, ',', ',', 3, 1);	// TIMESTAMP1
			hist_line->data = getitem(fd, offset, ',', '\n', 4, 1);	// DATA
			break;
		}
	}

	return hist_line;
}


gchar *gg_hist_time(int timestamp)
{
	gchar gtmp[32];
	gchar *gtmp2 = NULL;

	struct tm *ptm = (struct tm *) localtime((time_t *) & timestamp);
	strftime(gtmp, sizeof (gtmp), "%H:%M:%S (%Y-%m-%d)", ptm);

	gtmp2 = g_strdup_printf("%s", gtmp);

	return gtmp2;
}
