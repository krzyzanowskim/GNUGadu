/* $Id: sms_core.h,v 1.5 2003/09/10 22:31:59 shaster Exp $ */

#ifndef SMS_CORE_PLUGIN_H
#define SMS_CORE_PLUGIN_H 1

#define IDEA_GFX "/tmp/idea_token.gfx"
#define RESERVED_CHARS	"!\"'()*+-.<>[]\\^_`{}|~\t#;/?:@&=+,$% \r\n\v\x7f"

enum
{
    SMS_IDEA = 2,
    SMS_ERA,
    SMS_PLUS
};

enum
{
    ERR_NONE = 2,
    ERR_BAD_TOKEN,
    ERR_READ_TOKEN,
    ERR_LIMIT_EX,
    ERR_GATEWAY,
    ERR_SERVICE,
    ERR_DISABLE_G,
    ERR_UNAUTH,
    ERR_ACCESS_DENIED,
    ERR_SYNTAX,
    ERR_BAD_RCPT,
    ERR_MSG_TOO_LONG,
    ERR_UNKNOWN
};

enum
{
    GGADU_SMS_TYPE_WARN = 1,
    GGADU_SMS_TYPE_INFO
};

void send_sms(gboolean external, gchar * sms_sender, gchar * sms_number, gchar * sms_body, gchar * eralogin,
	      gchar * erapass);

void sms_message(gchar * sms_number, gchar * message);
void sms_warning(gchar * sms_number, gchar * warning);

int send_IDEA_stage2(gchar * pass, gpointer user_data);

gchar *ggadu_sms_formencode(gchar * source);
gchar *ggadu_sms_append_char(gchar * url_string, gchar char_to_append, gboolean convert_to_hex);


typedef struct
{
    char Command[6];
    char Host[1024];
    char Url[1024];
    char Url_Params[1024];
    char Post_Data[4096];
    int Post_Length;
} HTTPstruct;

#endif
