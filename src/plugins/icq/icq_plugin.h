/* $Id: icq_plugin.h,v 1.2 2003/04/09 17:03:40 thrulliq Exp $ */

enum {
    ICQ_UIN,
    ICQ_PASSWORD,
    ICQ_NICK
};

void load_contacts(gchar *encoding);

void save_addressbook_file(gpointer userlist);

