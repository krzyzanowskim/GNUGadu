/* $Id: perl_embed.c,v 1.13 2004/01/20 22:08:05 krzyzak Exp $ */

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
#include "repo.h"

typedef struct
{
    GGaduSigID q_name;
    char *func;
} signal_hook;

typedef struct
{
    char *name;
    char *repo_name;
    char *func;
} repo_hook;

typedef struct
{
    char loaded;
    char *filename;

    GSList *hooks;
    GSList *userlist_watch;

    PerlInterpreter *perl;
} perlscript;

GSList *perlscripts = NULL;

PerlInterpreter *my_perl;

extern void boot_DynaLoader(pTHX_ CV * cv);

perlscript *find_perl_script(gchar * name)
{
    perlscript *script;
    GSList *list = perlscripts;

    while (list)
    {
	gchar *fn_dup, *tmp;
	script = (perlscript *) list->data;
	fn_dup = g_strdup(script->filename);
	tmp = basename(fn_dup);
	if (!strcmp(tmp, name))
	{
	    g_free(fn_dup);
	    return script;
	}
	else
	    g_free(fn_dup);
	list = list->next;
    }

    return NULL;
}

int execute_perl(char *function, char **perl_args)
{
/*
    char *perl_args[2] = {args, NULL};
*/
    int count, ret_value = 1;
    SV *sv;

    dSP;
    ENTER;
    SAVETMPS;
    PUSHMARK(sp);
    count = perl_call_argv(function, G_EVAL | G_SCALAR, perl_args);
    SPAGAIN;

    print_debug("EXECUTE_PERL\n");

    sv = GvSV(gv_fetchpv("@", TRUE, SVt_PV));
    if (SvTRUE(sv))
    {
	printf("perl: Error %s\n", SvPV(sv, count));
	POPs;
    }
    else if (count != 1)
    {
	printf("perl: Error: expected 1 value from %s, got %d\n", function, count);
    }
    else
    {
	ret_value = POPi;
    }

    PUTBACK;
    FREETMPS;
    LEAVE;

    return ret_value;
}

int execute_perl_one(char *function, char *args)
{
    char *perl_args[2] = { args, NULL };
    return execute_perl(function, perl_args);
}

char *execute_perl_string(char *function, char **perl_args)
{
/*
    char *perl_args[2] = {args, NULL};
*/
    int count;
    char *ret_value = NULL;
    SV *sv;
    STRLEN n_a;

    dSP;
    ENTER;
    SAVETMPS;
    PUSHMARK(sp);
    count = perl_call_argv(function, G_EVAL | G_SCALAR, perl_args);
    SPAGAIN;

    sv = GvSV(gv_fetchpv("@", TRUE, SVt_PV));
    if (SvTRUE(sv))
    {
	printf("perl: Error %s\n", SvPV(sv, count));
	POPs;
    }
    else if (count != 1)
    {
	printf("perl: Error: expected 1 value from %s, got %d\n", function, count);
    }
    else
    {
	ret_value = g_strdup(POPpx);
    }

    PUTBACK;
    FREETMPS;
    LEAVE;

    return ret_value;
}

char *execute_perl_string_one(char *function, char *args)
{
    char *perl_args[2] = { args, NULL };
    return execute_perl_string(function, perl_args);
}

void hook_handler(GGaduSignal * signal, void (*perl_func) (GGaduSignal *, gchar *, void *))
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

	    if (signal->name == hook->q_name)
	    {
		PERL_SET_CONTEXT(script->perl);
		my_perl = script->perl;
		perl_func(signal, hook->func, (void *) script->perl);
	    }

	    list_hooks = list_hooks->next;
	}

	list_script = list_script->next;
    }
}

void userlist_handler(gchar * repo_name, gpointer key, gint actions)
{
    GGaduContact *k = NULL;
    GSList *list_script, *list;
    perlscript *script;
    repo_hook *hook;

    k = ggadu_repo_find_value(repo_name, key);

    g_return_if_fail(k != NULL);

    list_script = perlscripts;
    while (list_script)
    {
	script = (perlscript *) list_script->data;
	list = script->userlist_watch;
	while (list)
	{
	    hook = (repo_hook *) list->data;
	    if (hook->repo_name == NULL || !strcmp(hook->repo_name, repo_name))
	    {
		int count;
		SV *sv_name;
		SV *sv_action;
		SV *sv_user_id;

		dSP;

		PERL_SET_CONTEXT(script->perl);
		my_perl = script->perl;

		ENTER;
		SAVETMPS;

		sv_name = sv_2mortal(newSVpv(repo_name, 0));
		sv_action = sv_2mortal(newSViv(actions));
		sv_user_id = sv_2mortal(newSVpv(k->id, 0));

		PUSHMARK(SP);
		XPUSHs(sv_name);
		XPUSHs(sv_action);
		XPUSHs(sv_user_id);
		PUTBACK;

		count = call_pv(hook->func, G_DISCARD);

		FREETMPS;
		LEAVE;
	    }
	    list = list->next;
	}
	list_script = list_script->next;
    }
}

static XS(XS_GGadu_register_script)
{
    char *name;
    int junk;
    perlscript *script;
    dXSARGS;

    items = items;

    name = SvPV(ST(0), junk);

    print_debug("registering %s\n", name);

    script = find_perl_script(name);
    if (!script)
	XSRETURN(1);
    script->loaded = 1;
    print_debug("found %s in %s\n", name, script->filename);
    XSRETURN(0);
}

static XS(XS_GGadu_hello)
{
    char *world;
    int junk;
    dXSARGS;

    items = items;

    world = SvPV(ST(0), junk);
    printf("Perl: Hello %s\n", world);
    XSRETURN_EMPTY;
}

static XS(XS_GGadu_signal_emit)
{
    char *signame;
    void *sigdata;
    char *sigdst;
    int must_dup;
    int junk;
    dXSARGS;

    items = items;

    signame = SvPV(ST(0), junk);
    sigdata = SvPV(ST(1), junk);
    sigdst = SvPV(ST(2), junk);
    must_dup = SvIV(ST(3));

    signal_emit_full("perl:", signame, sigdata ? (must_dup ? g_strdup(sigdata) : sigdata) : NULL, sigdst, NULL);
    XSRETURN_EMPTY;
}

static XS(XS_GGadu_signal_hook)
{
    char *name;
    char *signame;
    GGaduSigID q_signame;
    char *func;
    int junk;
    signal_hook *hook;
    perlscript *script;
    dXSARGS;

    items = items;

    name = SvPV(ST(0), junk);
    signame = SvPV(ST(1), junk);
    func = SvPV(ST(2), junk);
    q_signame = g_quark_from_string(signame);

    print_debug("hooking %s, %s, %s, %d\n", name, signame, func, q_signame);

    script = find_perl_script(name);
    if (!script)
	XSRETURN(1);

    print_debug("still hooking\n");

    hook = g_new0(signal_hook, 1);
    hook->q_name = q_signame;
    hook->func = g_strdup(func);
    script->hooks = g_slist_append(script->hooks, hook);
    hook_signal((GGaduSigID) g_quark_from_string(signame), hook_handler);

    XSRETURN(0);
}

static XS(XS_GGadu_repo_watch_userlist)
{
    char *name;
    char *protocol;
    char *func;
    int junk;
    repo_hook *hook;
    perlscript *script;
    dXSARGS;

    items = items;

    name = SvPV(ST(0), junk);
    protocol = SvPV(ST(1), junk);
    func = SvPV(ST(2), junk);

    print_debug("watching userlist %s for %s, %s\n", protocol, name, func);

    script = find_perl_script(name);
    if (!script)
	XSRETURN(1);

    hook = g_new0(repo_hook, 1);
    hook->name = g_strdup(name);
    hook->func = g_strdup(func);
    if (protocol[0] == '*')
	hook->repo_name = NULL;
    else
	hook->repo_name = g_strdup(protocol);
    script->userlist_watch = g_slist_append(script->userlist_watch, hook);
    ggadu_repo_watch_add(hook->repo_name, REPO_ACTION_VALUE_CHANGE, REPO_VALUE_CONTACT, userlist_handler);

    XSRETURN(0);
}

static XS(XS_GGadu_find_user_id)
{
    char *id;
    char *protocol;
    char *interest;
    int junk;
    gpointer index;
    GGaduContact *k;
    gchar *key;
    dXSARGS;

    items = items;

    id = SvPV(ST(0), junk);
    protocol = SvPV(ST(1), junk);
    interest = SvPV(ST(2), junk);

    print_debug("Finding id='%s' in '%s'\n", id, protocol);

    index = ggadu_repo_value_first(protocol, REPO_VALUE_CONTACT, (gpointer *) & key);
    while (index)
    {
	k = ggadu_repo_find_value(protocol, key);
	print_debug("id='%s', k->id='%s'\n", id, k->id);
#define ODDAJ(x,y) if (y) XST_mPV(x,y); else XST_mPV(x,"<unset>")
	if (!ggadu_strcasecmp(id, k->id))
	{
	    ODDAJ(0, k->id);
	    ODDAJ(1, k->first_name);
	    ODDAJ(2, k->last_name);
	    ODDAJ(3, k->nick);
	    ODDAJ(4, k->mobile);
	    ODDAJ(5, k->email);
	    ODDAJ(6, k->gender);
	    ODDAJ(7, k->group);
	    ODDAJ(8, k->comment);
	    ODDAJ(9, k->birthdate);
	    ODDAJ(10, k->status_descr);
	    ODDAJ(11, k->ip);
	    ODDAJ(12, k->city);
	    ODDAJ(13, k->age);
	    XST_mIV(14, k->status);
	    XSRETURN(15);
	}
	index = ggadu_repo_value_next(protocol, REPO_VALUE_CONTACT, (gpointer *) & key, index);
    }

    XSRETURN(0);
}

static void xs_init(pTHX)
{
    char *file = __FILE__;
    dXSUB_SYS;

    newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, file);

    newXS("GGadu::register_script", XS_GGadu_register_script, "GGadu");
    newXS("GGadu::signal_emit", XS_GGadu_signal_emit, "GGadu");
    newXS("GGadu::hello", XS_GGadu_hello, "GGadu");
    newXS("GGadu::signal_hook", XS_GGadu_signal_hook, "GGadu");
    newXS("GGadu::repo_watch_userlist", XS_GGadu_repo_watch_userlist, "GGadu");
    newXS("GGadu::find_user_id", XS_GGadu_find_user_id, "GGadu");
}

int perl_load_script(char *script_name)
{
    perlscript *script;
    char *perl_args[] = { "", "-e", "0", "-w" };
    const char perl_definitions[] = {
	"sub load_file{" "my $f_name=shift;" "local $/=undef;" "open FH,$f_name or return \"__FAILED__\";" "$_=<FH>;" "close FH;" "return $_;" "}"
	    "sub load_n_eval{" "my $f_name=shift;" "my $strin=load_file($f_name);" "return 2 if($strin eq \"__FAILED__\");" "eval $strin;" "if($@){"
	    /*"  #something went wrong\n" */
	"print\"Errors loading file $f_name:\\n\";" "print\"$@\\n\";" "return 1;" "}" "return 0;" "}" "$SIG{__WARN__}=sub{print\"$_[0]\n\";};"
    };

    print_debug("gotta load %s\n", script_name);
    script = g_new0(perlscript, 1);
    script->loaded = 0;
    script->filename = g_strdup(script_name);
    script->perl = perl_alloc();
    PERL_SET_CONTEXT(script->perl);
    my_perl = script->perl;
    perl_construct(script->perl);
    perl_parse(script->perl, xs_init, 4, perl_args, NULL);
    eval_pv(perl_definitions, TRUE);

    perlscripts = g_slist_append(perlscripts, script);

    execute_perl_one("load_n_eval", script->filename);

    print_debug("aaaa\n");

    return 0;
};

int perl_unload_script(char *script_name)
{
    perlscript *script;
    GSList *list;

    list = perlscripts;
    while (list)
    {
	script = (perlscript *) list->data;
	print_debug("script_name: %s, script->filename: %s\n", script_name, script->filename);
	if (!strcmp(script_name, script->filename))
	{
	    PERL_SET_CONTEXT(script->perl);
	    perl_destruct(script->perl);
	    perl_free(script->perl);
	    perlscripts = g_slist_remove(perlscripts, script);
	    g_free(script);
	    break;
	}
	list = list->next;
    }

    return 0;
}

gint perl_load_scripts(void)
{
/*  GIOChannel *ch = NULL;
  GString *buffer = g_string_new (NULL);*/
    gchar *filename;
    gint loaded = 0;
    GDir *dir;
    gchar *file;

    filename = g_build_filename(config->configdir, "perl.scripts", NULL);
    dir = g_dir_open(filename, 0, NULL);
    g_free(filename);
    if (!dir)
	return 0;

    while ((file = (gchar *) g_dir_read_name(dir)))
    {
	gchar *real_filename;
	print_debug("perl: Autoloading script %s\n", file);
	real_filename = g_build_filename(config->configdir, "perl.scripts", file, NULL);
	perl_load_script(real_filename);
	g_free(real_filename);
	print_debug("perl: %s loaded\n", file);
	loaded++;
    }

    g_dir_close(dir);

    print_debug("perl: Loaded %d scripts.\n", loaded);

/*
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
*/
    return loaded;
}

#endif
