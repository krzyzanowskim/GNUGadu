/* $Id: perl_embed.h,v 1.2 2003/06/09 18:24:36 shaster Exp $ */

#ifndef GGadu_PERL_EMBED_H
#define GGadu_PERL_EMBED_H 1

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef PERL_EMBED
#define perl_load_script(x) /* */
#define perl_unload_script(x) /* */
#define perl_load_scripts() 0
#else

int perl_load_script (char *script_name);
int perl_unload_script (char *script_name);

gint perl_load_scripts (void);

#endif
#endif
