/* $Id: remote_plugin.c,v 1.13 2004/01/17 00:45:02 shaster Exp $ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <regex.h>

#include "gg-types.h"
#include "unified-types.h"
#include "plugins.h"
#include "ggadu_conf.h"
#include "signals.h"
#include "menu.h"
#include "support.h"
#include "dialog.h"
#include "perl_embed.h"

#include "remote_plugin.h"

GGaduPlugin *handler;

gchar *this_configdir = NULL;

const char *pth_unix = ".gg2_remote";
gchar *sock_path;
int sck = -1;
GIOChannel *sck_ch;
guint sck_watch;

GGaduMenu *menu_remotemenu;

regex_t regex;

gboolean var_same_uid = 1;
gboolean var_same_gid = 1;

struct sockaddr_un csun;
int csunlen;

int remote_send(char *text);
int remote_send_data(char *data, int len);

int remote_sig_gui_show(char *arg);
int remote_sig_gui_warn(char *arg);
int remote_sig_gui_msg(char *arg);
int remote_sig_gui_notify(char *arg);
int remote_sig_quit(char *arg);
int remote_sig_xosd_show(char *arg);

int remote_sig_remote_get_icon(char *arg);
int remote_sig_remote_get_icon_path(char *arg);

int remote_sig_core_load_plugin(char *arg);
int remote_sig_core_unload_plugin(char *arg);

int remote_sig_perl_load(char *arg);
int remote_sig_perl_unload(char *arg);

/* Scacheowane dane dla klientów remote */
gchar *remote_data_icon_directory;
gchar *remote_data_icon_filename;
gchar *remote_data_icon_tooltip;

/* W poni¿szej strukturze znajduj± siê komendy oraz funkcje wykonywane
 * dla poszczególnych komend.
 *
 * Wybór funkcji jest dokonywany na podstawie nazwy komendy (command)
 * oraz podkomendy (arg1), je¶li ta nie jest NULL.
 * Je¶li arg1 nie jest NULL znaczy to, ¿e danymi (np. tekst do wy¶wietlenia
 * przez xosd) dla podanej komendy jest argument drugi.
 * Je¶li arg1 jest NULL, to dane siê znajduj± w argumencie pierwszym.
 *
 * Je¶li signal_data jest równe 0, to dana komenda nie przyjmuje ¿adnych
 * danych. Je¶li signal_data jest równe 1, komenda przyjmuje dane, które
 * s± pobierane z argumentów w sposób opisany powy¿ej.
 *
 * returns okre¶la, czy wywo³ywana funkcja zwraca jakie¶ dane nadawcy.
 * Je¶li tak (1), to w przypadku braku adresu zwrotnego pakietu (co jest
 * mo¿liwe w przypadku gniazd uniksowych) pakiet jest odrzucany.
 * Funkcja, w przypadku zwracania danych, wysy³a je na adres podany
 * w zmiennej globalnej csun o rozmiarze csunlen.
 */

const struct
{
    char *command;
    char *arg1;
    char signal_data;
    char returns;
    int (*func) (char *arg);
} remote_signals[] =
{
    {
    "gui", "show", 0, 0, remote_sig_gui_show},
    {
    "gui", "warn", 1, 0, remote_sig_gui_warn},
    {
    "gui", "msg", 1, 0, remote_sig_gui_msg},
    {
    "gui", "notify", 1, 0, remote_sig_gui_notify},
    {
    "quit", NULL, 0, 0, remote_sig_quit},
    {
    "xosd", "show", 1, 0, remote_sig_xosd_show},
    {
    "remote", "get_icon", 1, 1, remote_sig_remote_get_icon},
    {
    "remote", "get_icon_path", 1, 1, remote_sig_remote_get_icon_path},
    {
    "core", "load_plugin", 1, 0, remote_sig_core_load_plugin},
    {
    "core", "unload_plugin", 1, 0, remote_sig_core_unload_plugin},
    {
    "perl", "load", 1, 0, remote_sig_perl_load},
    {
    "perl", "unload", 1, 0, remote_sig_perl_unload},
    {
    NULL, NULL, 0, 0, NULL}
};

GGadu_PLUGIN_INIT("remote", GGADU_PLUGIN_TYPE_MISC);

void read_remote_config(void)
{
    var_same_uid = (gboolean) ggadu_config_var_get(handler, "same_uid");
    var_same_gid = (gboolean) ggadu_config_var_get(handler, "same_gid");
}

int remote_send(char *text)
{
    return remote_send_data(text, strlen(text) + 1);
}

int remote_send_data(char *data, int len)
{
    struct msghdr msg;
    struct cmsghdr *cmsg;
    int text_len = len;
    char buf[CMSG_SPACE(sizeof (struct ucred))];
    struct ucred *pcred;
    struct iovec iov;

    msg.msg_control = buf;
    msg.msg_controllen = sizeof (buf);

    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_CREDENTIALS;
    cmsg->cmsg_len = CMSG_LEN(sizeof (struct ucred));

    pcred = (struct ucred *) CMSG_DATA(cmsg);
    pcred->pid = getpid();
    pcred->uid = getuid();
    pcred->gid = getgid();

    msg.msg_name = &csun;
    msg.msg_namelen = csunlen;

    iov.iov_base = data;
    iov.iov_len = text_len;

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    return sendmsg(sck, &msg, 0);
}

int remote_sig_gui_show(char *arg)
{
    signal_emit_full(GGadu_PLUGIN_NAME, "gui show invisible chats", NULL, "main-gui", NULL);
    return 0;
}

int remote_sig_gui_warn(char *arg)
{
    signal_emit_full(GGadu_PLUGIN_NAME, "gui show warning", g_strdup(arg == NULL ? "<null>" : arg), "main-gui", NULL);
    return 0;
}

int remote_sig_gui_msg(char *arg)
{
    signal_emit_full(GGadu_PLUGIN_NAME, "gui show message", g_strdup(arg == NULL ? "<null>" : arg), "main-gui", NULL);
    return 0;
}

int remote_sig_gui_notify(char *arg)
{
    signal_emit(GGadu_PLUGIN_NAME, "gui show notify", g_strdup(arg == NULL ? "<null>" : arg), "main-gui");
    return 0;
}

int remote_sig_quit(char *arg)
{
    signal_emit_full(GGadu_PLUGIN_NAME, "exit", NULL, NULL, NULL);
    g_main_loop_quit(config->main_loop);
    return 0;
}

int remote_sig_xosd_show(char *arg)
{
    signal_emit_full(GGadu_PLUGIN_NAME, "xosd show message", arg, NULL, NULL);
    return 0;
}

int remote_sig_remote_get_icon(char *arg)
{
    gchar *text;
    int dirlen;

    if (!remote_data_icon_directory || !remote_data_icon_filename || !remote_data_icon_tooltip)
    {
	print_debug("%s: cache empty\n", GGadu_PLUGIN_NAME);
	remote_send("err Cache empty");
	return -1;
    }

    dirlen = strlen(remote_data_icon_directory);
    text = g_new0(gchar, 3 + dirlen + 1 + strlen(remote_data_icon_filename) + 1);

    strcpy(text, "ok ");
    strcpy(text + 3, remote_data_icon_directory);
    strcpy(text + 4 + dirlen, remote_data_icon_filename);
    text[3 + dirlen] = '/';

    print_debug("%s: file: %s\n", GGadu_PLUGIN_NAME, text);
    remote_send(text);
    g_free(text);

    return 0;
}

int remote_sig_remote_get_icon_path(char *arg)
{
    gchar *found_filename = NULL;
    GSList *dir = NULL;
    gchar *iconsdir = NULL;
    gchar *text;

    if (!remote_data_icon_directory || !remote_data_icon_filename || !remote_data_icon_tooltip)
    {
	print_debug("%s: cache empty\n", GGadu_PLUGIN_NAME);
	remote_send("err Cache empty");
	return -1;
    }

    /* We first try any pixmaps directories set by the application. */
    dir = g_slist_prepend(dir, PACKAGE_DATA_DIR "/pixmaps");
    dir = g_slist_prepend(dir, PACKAGE_DATA_DIR "/pixmaps/emoticons");
#ifdef GGADU_DEBUG
    dir = g_slist_prepend(dir, PACKAGE_SOURCE_DIR "/pixmaps");
    dir = g_slist_prepend(dir, PACKAGE_SOURCE_DIR "/pixmaps/emoticons");
#endif
    iconsdir = g_build_filename(PACKAGE_DATA_DIR, "pixmaps", "icons", remote_data_icon_directory, NULL);
    dir = g_slist_prepend(dir, iconsdir);

    while (dir)
    {
	found_filename = check_file_exists((gchar *) dir->data, remote_data_icon_filename);

	if (found_filename)
	    break;

	dir = dir->next;
    }

    /* If we haven't found the pixmap, try the source directory. */
    if (!found_filename)
	found_filename = check_file_exists("../pixmaps", remote_data_icon_filename);

    if (!found_filename)
    {
	g_warning(_("Couldn't find pixmap file: %s"), remote_data_icon_filename);
	remote_send("err Cache empty");
	return -1;
    }

    print_debug("%s: FILE: %s\n", GGadu_PLUGIN_NAME, found_filename);
    text = g_new0(gchar, 3 + strlen(found_filename) + 1);
    strcpy(text, "ok ");
    strcpy(text + 3, found_filename);

    remote_send(text);

    g_free(text);
    g_slist_free(dir);
    g_free(iconsdir);

    return 0;
}

int remote_sig_core_load_plugin(char *arg)
{
    print_debug("%s: attempting to load %s plugin\n", GGadu_PLUGIN_NAME, arg);
    load_plugin(arg);
    return 0;
}

int remote_sig_core_unload_plugin(char *arg)
{
    print_debug("%s: attempting to unload %s plugin\n", GGadu_PLUGIN_NAME, arg);
    unload_plugin(arg);
    return 0;
}

int remote_sig_perl_load(char *arg)
{
    print_debug("%s: attempting to load %s script\n", GGadu_PLUGIN_NAME, arg);
    perl_load_script(arg);
    return 0;
}

int remote_sig_perl_unload(char *arg)
{
    print_debug("%s: attempting to unload %s script\n", GGadu_PLUGIN_NAME, arg);
    perl_unload_script(arg);
    return 0;
}

void signal_recv(gpointer name, gpointer signal_ptr)
{
    GGaduSignal *signal = (GGaduSignal *) signal_ptr;

    print_debug("%s : receive signal %d\n", GGadu_PLUGIN_NAME, signal->name);

    if (signal->name == g_quark_from_static_string("update config"))
    {
	GGaduDialog *d = signal->data;
	GSList *tmplist = d->optlist;
	if (d->response == GGADU_OK)
	{
	    while (tmplist)
	    {
		GGaduKeyValue *kv = (GGaduKeyValue *) tmplist->data;
		switch (kv->key)
		{
		case REMOTE_CONFIG_SAME_UID:
		    print_debug("change same_uid to %d\n", kv->value);
		    ggadu_config_var_set(handler, "same_uid", kv->value);
		    break;
		case REMOTE_CONFIG_SAME_GID:
		    print_debug("change same_gid to %d\n", kv->value);
		    ggadu_config_var_set(handler, "same_gid", kv->value);
		    break;
		}
		tmplist = tmplist->next;
	    }
	    ggadu_config_save(handler);
	    read_remote_config();
	}
	GGaduDialog_free(d);

	return;
    }

    if ((signal->name == g_quark_from_static_string("docklet set icon")) || (signal->name == g_quark_from_static_string("docklet set default icon")))
    {
	GSList *sigdata = (GSList *) signal->data;
	gchar *directory = g_slist_nth_data(sigdata, 0);
	gchar *filename = g_slist_nth_data(sigdata, 1);
	gchar *tooltip = g_slist_nth_data(sigdata, 2);

	print_debug("%s: %p %p %p %s %s %s\n", GGadu_PLUGIN_NAME, directory, filename, tooltip, directory ? directory : "<null>",
		    filename ? filename : "<null>", tooltip ? tooltip : "<null>");
	if (!filename)
	    return;

	remote_data_icon_directory = g_strdup(directory);
	remote_data_icon_filename = g_strdup(filename);
	remote_data_icon_tooltip = g_strdup(tooltip ? tooltip : "GNU Gadu 2");

	print_debug("%s: FILE: %s%s, %s\n", GGadu_PLUGIN_NAME, remote_data_icon_directory, remote_data_icon_filename, remote_data_icon_tooltip);
    }

    return;
}

void remote_destroy(void)
{
    g_source_remove(sck_watch);
    if (sck != -1)
	close(sck);
    sck = -1;
}

char *get_regerror(int errcode, regex_t * compiled)
{
    size_t length = regerror(errcode, compiled, NULL, 0);
    char *buffer = malloc(length);
    if (!buffer)
	abort();
    (void) regerror(errcode, compiled, buffer, length);
    return buffer;
}


void remote_process(char *buf)
{
    regmatch_t matches[6];
    char *command;
    char *arg1 = NULL;
    char *arg2 = NULL;
    int e;

    if ((e = regexec(&regex, buf, 6, matches, 0)) != 0)
    {
	char *err = get_regerror(e, &regex);
	printf("%s: regexec() failed: %s\n", GGadu_PLUGIN_NAME, err);
	free(err);
	return;
    }

    command = buf + matches[1].rm_so;
    buf[matches[1].rm_eo] = '\0';
    arg1 = matches[3].rm_so == -1 ? NULL : buf + matches[3].rm_so;
    if (arg1)
    {
	buf[matches[3].rm_eo] = '\0';
	arg2 = matches[5].rm_so == -1 ? NULL : buf + matches[5].rm_so;
	if (arg2)
	    buf[matches[5].rm_eo] = '\0';
    }

    print_debug("%s: %s (%s, %s)\n", GGadu_PLUGIN_NAME, command, arg1 == NULL ? "<void>" : arg1, arg2 == NULL ? "<void>" : arg2);


    for (e = 0; remote_signals[e].command != NULL; e++)
    {
	if (strcasecmp(command, remote_signals[e].command) == 0)
	{
	    if (remote_signals[e].arg1 != NULL)
	    {
		if (strcasecmp(arg1, remote_signals[e].arg1) == 0)
		{
		    remote_signals[e].func(arg2);
		}
	    }
	    else
	    {
		remote_signals[e].func(arg1);
	    }
	}
    }
}

gboolean remote_recv(GIOChannel * source, GIOCondition condition, gpointer data)
{
    if ((condition & G_IO_ERR) || (condition & G_IO_HUP))
    {
	remote_destroy();
	return FALSE;
    }

    if (condition & G_IO_IN)
    {
	char buf[512];
	int len;

	struct msghdr msg;
	struct cmsghdr *cmsg = NULL;
	struct ucred *pcred;
	struct iovec iov[1];
	char mbuf[CMSG_SPACE(sizeof (struct ucred))];

	memset(&msg, 0, sizeof msg);
	memset(mbuf, 0, sizeof mbuf);

	msg.msg_name = &csun;
	msg.msg_namelen = sizeof csun;

	iov[0].iov_base = buf;
	iov[0].iov_len = 512;

	msg.msg_iov = iov;
	msg.msg_iovlen = 1;

	msg.msg_control = mbuf;
	msg.msg_controllen = sizeof mbuf;

	if ((len = recvmsg(sck, &msg, 0)) == -1)
	{
	    print_debug("%s: recvmsg: %s\n", GGadu_PLUGIN_NAME, strerror(errno));
	    return TRUE;
	}

	/* szukamy w danych dodatkowych przes³anych uwierzytelnieñ */
	for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg))
	{
	    if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_CREDENTIALS)
	    {
		pcred = (struct ucred *) CMSG_DATA(cmsg);
		break;
	    }
	}

	if (cmsg == NULL)
	{
	    /* nie znale¼li¶my ¿adnych uwierzytelnieñ, wiêc ignorujemy pakiet */
	    print_debug("%s: no credentials, discarding\n", GGadu_PLUGIN_NAME);
	    return TRUE;
	}

	/* sprawdzanie uwierzytelnieñ
	 * w zale¿no¶ci od opcji sprawdzane jest:
	 *  - uid
	 *  - gid
	 */

	print_debug("%s: pid=%d, uid=%d, gid=%d\n", GGadu_PLUGIN_NAME, pcred->pid, pcred->uid, pcred->gid);

	if (var_same_uid && pcred->uid != getuid())
	{
	    print_debug("%s: z³y uid (%d != %d)\n", GGadu_PLUGIN_NAME, pcred->uid, getuid());
	    return TRUE;
	}
	if (var_same_gid && pcred->gid != getgid())
	{
	    print_debug("%s: z³y gid (%d != %d)\n", GGadu_PLUGIN_NAME, pcred->gid, getgid());
	    return TRUE;
	}

	/* sprawdzenie poprawno¶ci danych */
	if (len == 0)		/* nie mo¿e byæ */
	{
	    print_debug("%s: recvmsg() = 0, a nie powinno\n", GGadu_PLUGIN_NAME);
	    return TRUE;
	}

	if (buf[len - 1] != '\0')
	{
	    print_debug("%s: b³êdny pakiet\n", GGadu_PLUGIN_NAME);
	    return TRUE;
	}

/*
	csun = msg.msg_name;
*/
	csunlen = msg.msg_namelen;

	/* przes³anie danych do przetworzenia */
	remote_process(buf);

    }

    return TRUE;
}

int remote_init(void)
{
    struct sockaddr_un sun;
    int yes = 1;

    sock_path = g_build_filename(this_configdir, pth_unix, NULL);

    /* poni¿szy kod nie podoba mi siê za bardzo 
     * jako¶ tak niezbyt ³adnie wygl±da na moim monitorze
     * mo¿e kiedy¶ si±dê i pozmieniam poni¿szy kod, ¿eby ³adnie siê
     * prezentowa³
     */

    unlink(sock_path);
    sck = socket(PF_UNIX, SOCK_DGRAM, 0);
    if (sck == -1)
	goto abort;

    sun.sun_family = AF_UNIX;
    bzero(sun.sun_path, sizeof (sun.sun_path));
    strcpy(sun.sun_path, sock_path);

    if (bind(sck, (struct sockaddr *) &sun, SUN_LEN(&sun)) == -1)
	goto abort;

    if (setsockopt(sck, SOL_SOCKET, SO_PASSCRED, &yes, sizeof (int)) == -1)
	goto abort;

    sck_ch = g_io_channel_unix_new(sck);
    sck_watch = g_io_add_watch(sck_ch, G_IO_IN | G_IO_ERR | G_IO_HUP, remote_recv, NULL);

    return 0;

  abort:
    print_debug("%s: %s\n", GGadu_PLUGIN_NAME, strerror(errno));
    print_debug("%s: plugin disabled\n", GGadu_PLUGIN_NAME);
    if (sck != -1)
    {
	close(sck);
	sck = -1;
    }
    return -1;
}

gpointer remote_menu_preferences(gpointer user_data)
{
    GGaduDialog *d;

    print_debug("%s : Preferences\n", GGadu_PLUGIN_NAME);
    d = ggadu_dialog_new();
    ggadu_dialog_set_title(d, _("Remote Preferences"));
    ggadu_dialog_callback_signal(d, "update config");
    ggadu_dialog_set_type(d, GGADU_DIALOG_CONFIG);

    ggadu_dialog_add_entry(&(d->optlist), REMOTE_CONFIG_SAME_UID, _("Ignore commands with different uid"), VAR_BOOL,
			   (gpointer) ggadu_config_var_get(handler, "same_uid"), VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&(d->optlist), REMOTE_CONFIG_SAME_GID, _("Ignore commands with different gid"), VAR_BOOL,
			   (gpointer) ggadu_config_var_get(handler, "same_gid"), VAR_FLAG_NONE);

    signal_emit(GGadu_PLUGIN_NAME, "gui show dialog", d, "main-gui");

    return NULL;
}

GGaduMenu *remote_menu(void)
{
    GGaduMenu *root = ggadu_menu_create();
    GGaduMenu *item = ggadu_menu_add_item(root, _("Remote"), NULL, NULL);

    ggadu_menu_add_submenu(item, ggadu_menu_new_item(_("Preferences"), remote_menu_preferences, NULL));

    return root;
}

void start_plugin()
{
    int e;
    if ((e = regcomp(&regex, "^([^ ]+)( ([^ ]+)( (.*))?)?$", REG_EXTENDED)) != 0)
    {
	char *err = get_regerror(e, &regex);
	print_debug("%s: regcomp() failed: %s\n", GGadu_PLUGIN_NAME, err);
	print_debug("%s: plugin disabled\n", GGadu_PLUGIN_NAME);
	free(err);
	return;
    }

    if (remote_init() == -1)
	return;

    register_signal(handler, "update config");

    /* perfidnie podpinamy siê pod sygna³ dockleta */
    register_signal(handler, "docklet set icon");
    register_signal(handler, "docklet set default icon");
    menu_remotemenu = remote_menu();
    signal_emit(GGadu_PLUGIN_NAME, "gui register menu", menu_remotemenu, "main-gui");
}


GGaduPlugin *initialize_plugin(gpointer conf_ptr)
{
    print_debug("%s : initialize\n", GGadu_PLUGIN_NAME);

    GGadu_PLUGIN_ACTIVATE(conf_ptr);	/* wazne zeby to bylo tutaj */

    handler = (GGaduPlugin *) register_plugin(GGadu_PLUGIN_NAME, _("Remote control"));


    if (g_getenv("CONFIG_DIR") || g_getenv("HOME_ETC"))
	this_configdir = g_build_filename(g_get_home_dir(), g_getenv("CONFIG_DIR") ? g_getenv("CONFIG_DIR") : g_getenv("HOME_ETC"), "gg2", NULL);
    else
	this_configdir = g_build_filename(g_get_home_dir(), ".gg2", NULL);

    ggadu_config_set_filename((GGaduPlugin *) handler, g_build_filename(this_configdir, "remote", NULL));

    ggadu_config_var_add(handler, "same_uid", VAR_BOOL);
    ggadu_config_var_add(handler, "same_gid", VAR_BOOL);

    if (!ggadu_config_read(handler))
	g_warning(_("Unable to read config file for plugin remote"));

    read_remote_config();

    register_signal_receiver((GGaduPlugin *) handler, (signal_func_ptr) signal_recv);

    return handler;
}


void destroy_plugin()
{
    print_debug("destroy_plugin %s\n", GGadu_PLUGIN_NAME);
    if (menu_remotemenu)
    {
	signal_emit(GGadu_PLUGIN_NAME, "gui unregister menu", menu_remotemenu, "main-gui");
	ggadu_menu_free(menu_remotemenu);
    }
    g_free(this_configdir);
    unlink(sock_path);
    g_free(sock_path);
    regfree(&regex);
}
