#ifndef GGadu_PERL_EMBED_H
#define GGadu_PERL_EMBED_H 1

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef PERL_EMBED
#define perl_load_script(x) /* */
#define perl_unload_script(x) /* */
#define perl_load_scripts() 0
#define perl_action_on_msg_receive(x,y) y
#else

int perl_load_script (char *script_name);
int perl_unload_script (char *script_name);

gint perl_load_scripts (void);

/* Frees msg and allocates memory for new text even if it is unchanged */
char *perl_action_on_msg_receive (char *uid, char *msg);

#endif
#endif
