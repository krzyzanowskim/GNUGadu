#ifndef JABBER_PLUGIN_ROSTER_H
#define JABBER_PLUGIN_ROSTER_H 1

#include <iksemel.h>

enum {
    J_ROSTER_NONE = 0,
    J_ROSTER_TO,
    J_ROSTER_FROM,
    J_ROSTER_BOTH,
    J_ROSTER_REMOVE
};


void roster_update_presence(ikspak *pak);
void roster_update_item(iks *x, gint type);
void roster_update(ikspak *pak);
void presence_parse(ikspak *pak);
void remove_roster(char *jid);

#endif
