/* $Id: remote_client.h,v 1.2 2004/01/17 00:44:57 shaster Exp $ */
#ifndef REMOTE_CLIENT_H
#define REMOTE_CLIENT_H

int remote_init(void);
int remote_send(char *text);
int remote_send_data(char *data, int len);
int remote_close(void);
int remote_fd(void);

int remote_sig_quit(void);
int remote_sig_gui_show(void);
int remote_sig_gui_warn(char *text);
int remote_sig_gui_msg(char *text);
int remote_sig_gui_notify(char *text);
int remote_sig_xosd_show(char *text);
int remote_sig_remote_get_icon(char **filepath);
int remote_sig_remote_get_icon_path(char **filepath);

#endif
