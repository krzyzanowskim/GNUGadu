#ifndef GGadu_PERL_EMBED_H
#define GGadu_PERL_EMBED_H 1

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

int perl_load_script (char *script_name);
int perl_unload_script (char *script_name);

#endif
