/* $Id: signals.c,v 1.14 2004/01/17 00:44:58 shaster Exp $ */
#include <glib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gg-types.h"
#include "plugins.h"
#include "support.h"
#include "signals.h"

extern GGaduConfig *config;

static GMutex *thread_signal_mutex = NULL;
static GAsyncQueue *thread_signal_queue = NULL;
static gboolean check_thread_signals = FALSE;
static GIOChannel *thread_signal_channel = NULL;
static gint thread_signal_watch = 0;
static gint thread_signal_write_pipe = 0;


void GGaduSignal_free(GGaduSignal * sig)
{
    g_free(sig->source_plugin_name);
    g_free(sig->destination_plugin_name);
    g_free(sig);
}


GGaduSignal *signal_cpy(GGaduSignal * sig)
{
    GGaduSignal *tmpsignal = g_new0(GGaduSignal, 1);

    tmpsignal->name = sig->name;
    tmpsignal->source_plugin_name = g_strdup(sig->source_plugin_name);
    tmpsignal->destination_plugin_name = g_strdup(sig->destination_plugin_name);

    tmpsignal->data = sig->data;
    tmpsignal->free_me = sig->free_me;	/* flaga czy zwalniac signal po wykonaniu czy tez nie
    return tmpsignal;
}

/*
 * tylko wrzuca signal do listy signali funkcje zwalniajaca signal 
 * BOL? kiedy zdejmowac to z listy? nie wiem, a moze przybic to dosignala ? a moze nie
 */
GGaduSigID register_signal(GGaduPlugin * plugin_handler, gpointer name)
{
    GQuark q_name = g_quark_from_string(name);
    GGaduPlugin *tmplugin = (GGaduPlugin *) plugin_handler;
    GGaduSignalinfo *signalinfo;

    print_debug("%s : register_signal : %s %d\n", tmplugin->name, name, q_name);

    signalinfo = g_new0(GGaduSignalinfo, 1);
    signalinfo->name = q_name;
/*
    signalinfo->signal_free = signal_free;
 */

    tmplugin->signals = g_slist_append(tmplugin->signals, signalinfo);

#ifdef PERL_EMBED
/*
      register_signal_perl (name, NULL);      
*/
#endif
    return q_name;
}

void register_signal_perl(gchar * name, void (*perl_handler) (GGaduSignal *, gchar *, void *))
{
    GGaduSigID q_name = g_quark_from_string(name);
    GSList *list;
    GGaduSignalHook *hook;

    list = config->signal_hooks;
    while (list)
    {
	hook = (GGaduSignalHook *) list->data;
	if (hook->name == q_name)
	{
	    hook->perl_handler = perl_handler;
	    return;
	}
	list = list->next;
    }

    hook = g_new0(GGaduSignalHook, 1);
    hook->name = q_name;
    hook->perl_handler = perl_handler;
    hook->hooks = NULL;

    config->signal_hooks = g_slist_append(config->signal_hooks, hook);
    print_debug("register_signal_perl %s %d\n", name, q_name);
}

void hook_signal(GGaduSigID q_name, void (*hook) (GGaduSignal * signal, void (*perl_handler) (GGaduSignal *, gchar *, void *)))
{
    GGaduSignalHook *signalhook;
    GSList *list = config->signal_hooks;

    print_debug("hook_signal : %s\n", g_quark_to_string(q_name));

    while (list)
    {
	signalhook = (GGaduSignalHook *) list->data;
	if (signalhook->name == q_name)
	{
	    signalhook->hooks = g_slist_append(signalhook->hooks, (void *) hook);
	    return;
	}
	list = list->next;
    }

    /* unfortunately we didn't find existing signal
     * however, the perlscripts can try to register themselves before
     * we have any signals, so we just create one here
     */
    signalhook = g_new0(GGaduSignalHook, 1);
    signalhook->name = q_name;
    signalhook->perl_handler = NULL;
    signalhook->hooks = g_slist_append(signalhook->hooks, (void *) hook);

    config->signal_hooks = g_slist_append(config->signal_hooks, signalhook);
}

GGaduSignalinfo *find_signal(gpointer signal_name)
{
    GSList *tmp = config->plugins;
    GGaduPlugin *plugin_handler = NULL;
    GSList *signals_list = NULL;
    GGaduSignalinfo *signalinfo = NULL;

    if (signal_name == 0)
	return NULL;

    while (tmp)
    {
	plugin_handler = (GGaduPlugin *) tmp->data;

	if (plugin_handler == NULL)
	    return NULL;
	signals_list = (GSList *) plugin_handler->signals;

	while (signals_list)
	{
	    signalinfo = (GGaduSignalinfo *) signals_list->data;

	    if ((GQuark) signalinfo->name == (GQuark) signal_name)
		return signalinfo;

	    signals_list = signals_list->next;
	}

	tmp = tmp->next;
    }

    return NULL;
}


gpointer do_signal(GGaduSignal * tmpsignal, GGaduSignalinfo * signalinfo)
{
    GGaduPlugin *dest = NULL;
    GGaduPlugin *src = NULL;
    GSList *tmp = config->plugins;
    GSList *hooks = config->signal_hooks;
    void (*hook_func) (GGaduSignal *, void (*)(GGaduSignal *, gchar *, void *perl)) = NULL;

    while (hooks)
    {
	GGaduSignalHook *hook = (GGaduSignalHook *) hooks->data;
	/* print_debug("qq %d %s - %d %s\n",tmpsignal->name,g_quark_to_string(tmpsignal->name),hook->name,g_quark_to_string(hook->name)); */
	if (tmpsignal->name == hook->name)
	{
	    GSList *list = hook->hooks;

	    while (list)
	    {
		(void *) hook_func = (void *) list->data;
		hook_func(tmpsignal, hook->perl_handler);
		list = list->next;
	    }
	    break;
	}
	hooks = hooks->next;
    }

    if (tmpsignal->destination_plugin_name == NULL)
    {
	/* broadcast */
	while (tmp)
	{
	    dest = (GGaduPlugin *) tmp->data;
	    src = find_plugin_by_name(tmpsignal->source_plugin_name);

	    if ((src != NULL) && (dest != NULL) && (dest->name != 0))
	    {

		if (g_strcasecmp(src->name, dest->name))
		    dest->signal_receive_func((gpointer) tmpsignal->name, tmpsignal);

		while (g_main_pending())
		    g_main_iteration(TRUE);	/* ewentualne odrysowanie GUI */
	    }

	    tmp = tmp->next;
	}

    }
    else
    {
	/* one particular destination */
	GSList *dest_list = NULL;
	GSList *dest_list_to_remove = NULL;

	/* czy uzyto maski np. "sound*" */
	if (g_strrstr(tmpsignal->destination_plugin_name, "*"))
	    dest_list = find_plugin_by_pattern(tmpsignal->destination_plugin_name);
	else
	    dest_list = g_slist_append(dest_list, find_plugin_by_name(tmpsignal->destination_plugin_name));

	if ((dest_list != NULL) && (dest_list->data == NULL))
	{
	    GGaduSignal_free(tmpsignal);
	    return NULL;
	}

	dest_list_to_remove = dest_list;

	while (dest_list)
	{
	    dest = (GGaduPlugin *) dest_list->data;

	    if ((dest != NULL) && (dest->signal_receive_func != NULL))
		dest->signal_receive_func((gpointer) tmpsignal->name, tmpsignal);

	    dest_list = dest_list->next;
	}

	g_slist_free(dest_list_to_remove);

    }

    /* free signal data with function taken from signal_emit_full */
    if ((tmpsignal != NULL) && (tmpsignal->free != NULL) && (tmpsignal->free_me == TRUE))
	tmpsignal->free(tmpsignal->data);

/*
    if ((signalinfo != NULL) && (signalinfo->signal_free != NULL) && 
        (tmpsignal != NULL) && (tmpsignal->free_me) && (!tmpsignal->data_return))
    {
	signalinfo->signal_free(tmpsignal); /* ZONK
    }
*/

    if ((tmpsignal != NULL) && (tmpsignal->data_return != NULL))
	return (tmpsignal->data_return);

    return NULL;
}


void flush_queued_signals()
{
    GSList *signals = NULL;

    signals = config->waiting_signals;

    while (signals)
    {
	GGaduSignal *sig = (GGaduSignal *) signals->data;
	GGaduSignalinfo *signalinfo = NULL;

	if ((signalinfo = find_signal((gpointer) sig->name)) == NULL)
	{
	    print_debug("core : flush_queued_signals : Nie ma takiego czego zarejestrowanego : %d!!! \n", sig->name);
	    /* wiec zwalniam go i puszczam w niepamiec ? */
	    g_free(sig->source_plugin_name);
	    g_free(sig->destination_plugin_name);
	    if (sig->free && sig->free_me)
		sig->free(sig->data);
	    g_free(sig);

	}
	else
	{
	    do_signal(sig, signalinfo);
	    g_free(sig->source_plugin_name);
	    g_free(sig->destination_plugin_name);
	    g_free(sig);
	}

	signals = signals->next;
    }

    config->all_plugins_loaded = TRUE;

    g_slist_free(config->waiting_signals);
    config->waiting_signals = NULL;
}

/*
 * src_name     - nazwa nadawczego plugina
 * name         - nazwa signala
 * data		- dane do przeslania
 * dest_name    - nazwa plugina docelowego
 * signal_free  - funkcja zwalniajaca "data"
 */
void *signal_emit_full(gpointer src_name, gpointer name, gpointer data, gpointer dest_name, void (*signal_free) (gpointer))
{
    GQuark q_name = g_quark_from_string(name);
    GGaduSignal *tmpsignal = NULL;
    GGaduSignalinfo *signalinfo = NULL;
    gpointer ret = NULL;


    if (config->all_plugins_loaded && (signalinfo = find_signal((gpointer) q_name)) == NULL)
    {
	print_debug("core : Nie ma takiego czego zarejestrowanego : %s!!! \n", name);
	return NULL;
    }

    tmpsignal = g_new0(GGaduSignal, 1);

    tmpsignal->name = q_name;
    tmpsignal->source_plugin_name = g_strdup(src_name);
    tmpsignal->destination_plugin_name = g_strdup(dest_name);

    tmpsignal->data = data;
    tmpsignal->free_me = TRUE;	/* flaga czy zwalniac signal po wykonaniu czy tez nie */
    tmpsignal->free = signal_free;

    print_debug("%s : signal_emit %d %s\n", src_name, q_name, name);

    if (config->all_plugins_loaded == TRUE)
    {
	ret = do_signal(tmpsignal, signalinfo);
	g_free(tmpsignal->source_plugin_name);
	g_free(tmpsignal->destination_plugin_name);
	g_free(tmpsignal);
    }
    else
    {
	/* jesli nie wszystko zostalo zaladowane to wrzucamy do listy oczekujacych */
	config->waiting_signals = g_slist_append(config->waiting_signals, tmpsignal);
    }

    while (g_main_pending())
	g_main_iteration(TRUE);	/* ewentualne odrysowanie GUI */


    return ret;
}

/*
 * src_name     - nazwa nadawczego plugina
 * name         - nazwa signala
 * data		- dane do przeslania
 * dest_name    - nazwa plugina docelowego
 * signal_free  - funkcja zwalniajaca "data"
 */

void signal_emit_from_thread_full(gpointer src_name, gpointer name, gpointer data, gpointer dest_name, void (*signal_free) (gpointer))
{
    GQuark q_name = g_quark_from_string(name);
    GGaduSignal *tmpsignal = NULL;
    char dummy;

    tmpsignal = g_new0(GGaduSignal, 1);

    tmpsignal->name = q_name;
    tmpsignal->source_plugin_name = g_strdup(src_name);
    tmpsignal->destination_plugin_name = g_strdup(dest_name);

    tmpsignal->data = data;
    tmpsignal->free_me = TRUE;	/* flaga czy zwalniac signal po wykonaniu czy tez nie */
    tmpsignal->free = signal_free;

    print_debug("%s : signal_emit_from_thread %d %s\n", src_name, q_name, name);

    g_async_queue_push(thread_signal_queue, tmpsignal);

    /* wyslij komu trzeba ze jest sygnal do odebrania */
    write(thread_signal_write_pipe, &dummy, sizeof (dummy));
}

static gboolean thread_signal_test_chan(GIOChannel * source, GIOCondition cond, gpointer data)
{
    GGaduSignal *signal = NULL;
    print_debug("thread_signal_test_chan()\n");

    g_mutex_lock(thread_signal_mutex);

    if ((cond & G_IO_ERR) || (cond & G_IO_HUP) || !thread_signal_queue)
    {
	print_debug("IO_ERR || IO_HUP!\n");
	check_thread_signals = FALSE;

	thread_signal_watch = 0;

	g_io_channel_unref(thread_signal_channel);
	thread_signal_channel = NULL;

	g_mutex_unlock(thread_signal_mutex);

	return FALSE;
    }
    else if (cond & G_IO_IN)
    {
	while ((signal = g_async_queue_try_pop(thread_signal_queue)) != NULL)
	{
	    char c;
	    gint bytes_read;
	    print_debug("signal popped!\n");
	    config->waiting_signals = g_slist_append(config->waiting_signals, signal);
	    g_io_channel_read_chars(source, &c, sizeof (c), &bytes_read, NULL);
	}
	flush_queued_signals();
    }

    g_mutex_unlock(thread_signal_mutex);

    return TRUE;
}

gboolean signal_from_thread_enabled()
{
    gint p[2];

    print_debug("signal_thread_enabled()\n");

    if (!thread_signal_mutex)
	thread_signal_mutex = g_mutex_new();

    g_mutex_lock(thread_signal_mutex);

    if (thread_signal_channel)
    {
	g_mutex_unlock(thread_signal_mutex);
	return TRUE;
    }

    if (pipe(p) == -1)
    {
	print_debug("cannot create pipe\n");
	g_mutex_unlock(thread_signal_mutex);
	return FALSE;
    }

    thread_signal_channel = g_io_channel_unix_new(p[0]);

    if (!thread_signal_channel)
    {
	print_debug("cannot create channel\n");
	g_mutex_unlock(thread_signal_mutex);
	return FALSE;
    }

    if (!thread_signal_queue)
	thread_signal_queue = g_async_queue_new();

    thread_signal_watch = g_io_add_watch(thread_signal_channel, G_IO_IN | G_IO_ERR | G_IO_HUP, thread_signal_test_chan, NULL);

    thread_signal_write_pipe = p[1];

    g_mutex_unlock(thread_signal_mutex);
    return TRUE;
}

void signal_from_thread_disabled()
{
    print_debug("TODO?");
}
