/* $Id: perl_embed.c,v 1.10 2003/05/11 14:13:41 zapal Exp $ */

/* Written by Bartosz Zapalowski <zapal@users.sf.net>
 * based on perl plugin in X-Chat
 *
 * Thanks to Adam Wujek for help in testing.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef PERL_EMBED

#include <stdio.h>
#include <EXTERN.h>
#include <XSUB.h>
#include <perl.h>
#include <glib.h>
#include <libgen.h>

#include "perl_embed.h"
#include "signals.h"
#include "support.h"

extern GGaduConfig *config;

typedef struct {
  char *name;
  char *func;
} signal_hook;

typedef struct {
  char loaded;
  char *filename;

  GSList *hooks;

  PerlInterpreter *perl;
} perlscript;

GSList *perlscripts = NULL;

PerlInterpreter *my_perl;

extern void boot_DynaLoader (pTHX_ CV* cv);

perlscript *find_perl_script (gchar *name)
{
  perlscript *script;
  GSList *list = perlscripts;

  while (list)
  {
    gchar *fn_dup, *tmp;
    script = (perlscript *) list->data;
    fn_dup = g_strdup (script->filename);
    tmp = basename (fn_dup);
    if (!strcmp (tmp, name))
    {
      g_free (fn_dup);
      return script;
    } else
      g_free (fn_dup);
    list = list->next;
  }

  return NULL;
}

int execute_perl (char *function, char **perl_args)
{
//  char *perl_args[2] = {args, NULL};
  int count, ret_value = 1;
  SV *sv;

  dSP;
  ENTER;
  SAVETMPS;
  PUSHMARK(sp);
  count = perl_call_argv (function, G_EVAL | G_SCALAR, perl_args);
  SPAGAIN;

  sv = GvSV (gv_fetchpv ("@", TRUE, SVt_PV));
  if (SvTRUE (sv))
  {
    printf ("perl: Error %s\n", SvPV (sv, count));
    POPs;
  } else if (count != 1)
  {
    printf ("perl: Error: expected 1 value from %s, got %d\n", function, count);
  } else
  {
    ret_value = POPi;
  }

  PUTBACK;
  FREETMPS;
  LEAVE;

  return ret_value;
}

int execute_perl_one (char *function, char *args)
{
  char *perl_args[2] = {args, NULL};
  return execute_perl (function, perl_args);
}

char *execute_perl_string (char *function, char **perl_args)
{
//  char *perl_args[2] = {args, NULL};
  int count;
  char *ret_value = NULL;
  SV *sv;
  STRLEN n_a;

  dSP;
  ENTER;
  SAVETMPS;
  PUSHMARK(sp);
  count = perl_call_argv (function, G_EVAL | G_SCALAR, perl_args);
  SPAGAIN;

  sv = GvSV (gv_fetchpv ("@", TRUE, SVt_PV));
  if (SvTRUE (sv))
  {
    printf ("perl: Error %s\n", SvPV (sv, count));
    POPs;
  } else if (count != 1)
  {
    printf ("perl: Error: expected 1 value from %s, got %d\n", function, count);
  } else
  {
    ret_value = g_strdup (POPpx);
  }

  PUTBACK;
  FREETMPS;
  LEAVE;

  return ret_value;
}

char *execute_perl_string_one (char *function, char *args)
{
  char *perl_args[2] = {args, NULL};
  return execute_perl_string (function, perl_args);
}

void sig_xosd_show_message (GGaduSignal *signal, gchar *perl_func)
{
  int count, junk;
  SV *sv_name;
  SV *sv_src;
  SV *sv_dst;
  SV *sv_data;
    
  dSP;
 
  ENTER;
  SAVETMPS;

  sv_name = sv_2mortal (newSVpv (signal->name, 0));
  sv_src  = sv_2mortal (newSVpv (signal->source_plugin_name, 0));
  if (signal->destination_plugin_name)
    sv_dst  = sv_2mortal (newSVpv (signal->destination_plugin_name, 0));
  else
    sv_dst  = sv_2mortal (newSVpv ("", 0));
  sv_data = sv_2mortal (newSVpv (signal->data, 0));

  PUSHMARK (SP);
  XPUSHs (sv_name);
  XPUSHs (sv_src);
  XPUSHs (sv_dst);
  XPUSHs (sv_data);
  PUTBACK;

  count = call_pv (perl_func, G_DISCARD);

  if (count == 0)
  {
    gchar *dst;
    signal->name = g_strdup (SvPV (sv_name, junk));
    signal->source_plugin_name = g_strdup (SvPV (sv_src, junk));
    dst = SvPV (sv_dst, junk);
    if (dst[0] != '\0')
      signal->destination_plugin_name = g_strdup (dst);
    signal->data = g_strdup (SvPV (sv_data, junk));
  }

  FREETMPS;
  LEAVE;
}

void sig_gui_msg_receive (GGaduSignal *signal, gchar *perl_func)
{
  int count, junk;
  GGaduMsg *msg = (GGaduMsg *) signal->data;
  SV *sv_name;
  SV *sv_src;
  SV *sv_dst;
  SV *sv_data_id;
  SV *sv_data_message;
  SV *sv_data_class;
  SV *sv_data_time;
    
  dSP;
 
  ENTER;
  SAVETMPS;

  sv_name = sv_2mortal (newSVpv (signal->name, 0));
  sv_src  = sv_2mortal (newSVpv (signal->source_plugin_name, 0));
  if (signal->destination_plugin_name)
    sv_dst  = sv_2mortal (newSVpv (signal->destination_plugin_name, 0));
  else
    sv_dst  = sv_2mortal (newSVpv ("", 0));
  sv_data_id      = sv_2mortal (newSVpv (msg->id, 0));
  sv_data_message = sv_2mortal (newSVpv (msg->message, 0));
  sv_data_class   = sv_2mortal (newSViv (msg->class));
  sv_data_time    = sv_2mortal (newSViv (msg->time));

  PUSHMARK (SP);
  XPUSHs (sv_name);
  XPUSHs (sv_src);
  XPUSHs (sv_dst);
  XPUSHs (sv_data_id);
  XPUSHs (sv_data_message);
  XPUSHs (sv_data_class);
  XPUSHs (sv_data_time);
  PUTBACK;

  count = call_pv (perl_func, G_DISCARD);

  if (count == 0)
  {
    gchar *dst;
    signal->name = g_strdup (SvPV (sv_name, junk));
    signal->source_plugin_name = g_strdup (SvPV (sv_src, junk));
    dst = SvPV (sv_dst, junk);
    if (dst[0] != '\0')
      signal->destination_plugin_name = g_strdup (dst);
    msg->id      = g_strdup (SvPV (sv_data_id, junk));
    msg->message = g_strdup (SvPV (sv_data_message, junk));
    msg->class   = SvIV (sv_data_class);
    msg->time    = SvIV (sv_data_time);
  }

  FREETMPS;
  LEAVE;
}

void hook_handler (GGaduSignal *signal, void (*perl_func) (GGaduSignal *, gchar *, void *))
{
  GSList *list_script;
  GSList *list_hooks;
  perlscript *script;
  signal_hook *hook;

  list_script = perlscripts;
  while (list_script)
  {
    script = (perlscript *) list_script->data;

    list_hooks = script->hooks;
    while (list_hooks)
    {
      hook = (signal_hook *) list_hooks->data;

      if (!ggadu_strcasecmp (signal->name, hook->name))
      {
	PERL_SET_CONTEXT (script->perl);
	my_perl = script->perl;
	perl_func (signal, hook->func, (void *) script->perl);
      }
      
      list_hooks = list_hooks->next;
    }
    
    list_script = list_script->next;
  }
}

static XS (XS_GGadu_register_script)
{
  char *name;
  int junk;
  perlscript *script;
  dXSARGS;

  items = items;

  name = SvPV (ST (0), junk);

  print_debug ("registering %s\n", name);
  
  script = find_perl_script (name);
  if (!script)
    XSRETURN (1);
  script->loaded = 1;
  print_debug ("found %s in %s\n", name, script->filename);
  XSRETURN (0);
}

static XS (XS_GGadu_hello)
{
  char *world;
  int junk;
  dXSARGS;

  items = items;

  world = SvPV (ST (0), junk);
  printf ("Perl: Hello %s\n", world);
  XSRETURN_EMPTY;
}

static XS (XS_GGadu_signal_emit)
{
  char *signame;
  void *sigdata;
  char *sigdst;
  int must_dup;
  int junk;
  dXSARGS;

  items = items;

  signame = SvPV (ST (0), junk);
  sigdata = SvPV (ST (1), junk);
  sigdst  = SvPV (ST (2), junk);
  must_dup = SvIV (ST (3));

  signal_emit_full ("perl:", signame,
      sigdata ? (must_dup ? g_strdup (sigdata) : sigdata) : NULL, sigdst, NULL);
  XSRETURN_EMPTY;
}

static XS (XS_GGadu_signal_hook)
{
  char *name;
  char *signame;
  char *func;
  int junk;
  signal_hook *hook;
  perlscript *script;
  dXSARGS;

  items = items;

  name    = SvPV (ST(0), junk);
  signame = SvPV (ST(1), junk);
  func    = SvPV (ST(2), junk);

  print_debug ("hooking %s, %s, %s\n", name, signame, func);

  script = find_perl_script (name);
  if (!script)
    XSRETURN (1);

  print_debug ("still hooking\n");

  hook = g_new0 (signal_hook, 1);
  hook->name = g_strdup (signame);
  hook->func = g_strdup (func);
  script->hooks = g_slist_append (script->hooks, hook);
  hook_signal (signame, hook_handler);
  
  XSRETURN (0);
}

static void xs_init (pTHX)
{
  char *file = __FILE__;
  dXSUB_SYS;

  newXS ("DynaLoader::boot_DynaLoader", boot_DynaLoader, file);

  newXS ("GGadu::register_script", XS_GGadu_register_script, "GGadu");
  newXS ("GGadu::signal_emit", XS_GGadu_signal_emit, "GGadu");
  newXS ("GGadu::hello", XS_GGadu_hello, "GGadu");
  newXS ("GGadu::signal_hook", XS_GGadu_signal_hook, "GGadu");
}

int perl_load_script (char *script_name)
{
  perlscript *script;
  char *perl_args[] = {"", "-e", "0", "-w"};
  const char perl_definitions[] = {
	  "sub load_file{"
	    "my $f_name=shift;"
	    "local $/=undef;"
	    "open FH,$f_name or return \"__FAILED__\";"
	    "$_=<FH>;"
	    "close FH;"
	    "return $_;"
	  "}"
	  "sub load_n_eval{"
	    "my $f_name=shift;"
	    "my $strin=load_file($f_name);"
	    "return 2 if($strin eq \"__FAILED__\");"
	    "eval $strin;"
	    "if($@){"
	    /*"  #something went wrong\n"*/
	      "print\"Errors loading file $f_name:\\n\";"
	      "print\"$@\\n\";"
	      "return 1;"
	    "}"
	    "return 0;"
	  "}"
	  "$SIG{__WARN__}=sub{print\"$_[0]\n\";};"
	};

  script = g_new0 (perlscript, 1);
  script->loaded = 0;
  script->filename = g_strdup (script_name);
  script->perl = perl_alloc ();
  PERL_SET_CONTEXT (script->perl);
  my_perl = script->perl;
  perl_construct (script->perl);
  perl_parse (script->perl, xs_init, 4, perl_args, NULL);
  eval_pv (perl_definitions, TRUE);

  perlscripts = g_slist_append (perlscripts, script);

  execute_perl_one ("load_n_eval", script->filename);

  printf ("aaaa\n");
  
  return 0;
};

int perl_unload_script (char *script_name)
{
  perlscript *script;
  GSList *list;

  list = perlscripts;
  while (list)
  {
    script = (perlscript *) list->data;
    print_debug ("script_name: %s, script->filename: %s\n", script_name, script->filename);
    if (!strcmp (script_name, script->filename))
    {
      PERL_SET_CONTEXT (script->perl);
      perl_destruct (script->perl);
      perl_free (script->perl);
      perlscripts = g_slist_remove (perlscripts, script);
      g_free (script);
      break;
    }
    list = list->next;
  }
  
  return 0;
}

gint perl_load_scripts (void)
{
  GIOChannel *ch = NULL;
  GString *buffer = g_string_new (NULL);
  gchar *filename;
  gint loaded = 0;

  filename = g_build_filename (config->configdir, "perl.load", NULL);

  ch = g_io_channel_new_file (filename, "r", NULL);
  g_free (filename);

  if (!ch)
  {
    g_string_free (buffer, TRUE);
    return 0;
  }

  while (g_io_channel_read_line_string (ch, buffer, NULL, NULL) != G_IO_STATUS_EOF)
  {
    print_debug ("Loading script %s\n", buffer->str);
    buffer->str[buffer->len - 1] = '\0';
    perl_load_script (buffer->str);
    loaded++;
  }

  g_io_channel_shutdown (ch, TRUE, NULL);

  g_string_free (buffer, TRUE);

  return loaded;
}
/*
char *perl_action_on_msg_receive (char *uid, char *msg)
{
  char *ret = msg;
  GSList *list;
  perlscript *script;

  print_debug ("Hejlo\n");
  
  list = perlscripts;
  while (list)
  {
    script = (perlscript *) list->data;
    if (script->loaded && script->on_msg_receive)
    {
      PERL_SET_CONTEXT (script->perl);
      my_perl = script->perl;
      ret = execute_perl_string_one (script->on_msg_receive, ret);
    }
    list = list->next;
  }

  return ret;
}
*/
#endif
