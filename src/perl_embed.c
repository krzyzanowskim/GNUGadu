#ifndef PERL_EMBED

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

int perl_load_script (char *script_name)
{
  /* empty */
  return -1;
}

void perl_unload_script (char *script_name)
{
  /* empty */
}

#else

#include <stdio.h>
#include <EXTERN.h>
#include <XSUB.h>
#include <perl.h>
#include <glib.h>
#include <libgen.h>

#include "perl_embed.h"
#include "signals.h"
#include "support.h"

typedef struct {
  char loaded;
  char *filename;

  char *on_msg_receive;
  
  PerlInterpreter *perl;
} perlscript;

GSList *perlscripts = NULL;

extern void boot_DynaLoader (pTHX_ CV* cv);

int execute_perl (char *function, char *args)
{
  char *perl_args[2] = {args, NULL};
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

char *execute_perl_string (char *function, char *args)
{
  char *perl_args[2] = {args, NULL};
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
static XS (XS_GGadu_register_script)
{
  char *name;
  char *on_msg_receive;
  int junk;
  char *tmp;
  perlscript *script;
  GSList *list;
  dXSARGS;

  name = SvPV (ST (0), junk);
  on_msg_receive = SvPV (ST (0), junk);

  list = perlscripts;
  while (list)
  {
    script = (perlscript *) list->data;
    tmp = basename (script->filename);
    if (!strcmp (tmp, name))
    {
      script->loaded = 1;

      script->on_msg_receive = g_strdup (on_msg_receive);
      
      g_free (tmp);
    }
    g_free (tmp);
    list = list->next;
  }
}

static XS (XS_GGadu_hello)
{
  char *world;
  int junk;
  dXSARGS;

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

  signame = SvPV (ST (0), junk);
  sigdata = SvPV (ST (1), junk);
  sigdst  = SvPV (ST (2), junk);
  must_dup = SvIV (ST (3));

  signal_emit_full ("perl:", signame,
      sigdata ? (must_dup ? g_strdup (sigdata) : sigdata) : NULL, sigdst, NULL);
  XSRETURN_EMPTY;
}

static void xs_init (pTHX)
{
  char *file = __FILE__;
  dXSUB_SYS;

  newXS ("DynaLoader::boot_DynaLoader", boot_DynaLoader, file);

  newXS ("GGadu::register_script", XS_GGadu_register_script, "GGadu");
  newXS ("GGadu::signal_emit", XS_GGadu_signal_emit, "GGadu");
  newXS ("GGadu::hello", XS_GGadu_hello, "GGadu");
}

int perl_load_script (char *script_name)
{
  perlscript *script;
  char *perl_args[] = {"", "-e", "0", "-w"};

  script = g_new0 (perlscript, 1);
  script->loaded = 0;
  script->filename = g_strdup (script_name);
  script->perl = perl_alloc ();
  PERL_SET_CONTEXT (script->perl);
  perl_construct (script->perl);
  perl_parse (script->perl, xs_init, 3, perl_args, NULL);
  perl_run (script->perl);
  
  perlscripts = g_slist_append (perlscripts, script);
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
      break;
    }
    list = list->next;
  }
  
  return 0;
}

#endif

