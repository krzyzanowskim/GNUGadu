/* $Id: sms_core.h,v 1.7 2003/09/21 14:08:31 shaster Exp $ */

#ifndef SMS_CORE_PLUGIN_H
#define SMS_CORE_PLUGIN_H 1

#define USER_AGENT         "User-Agent: Mozilla/5.0 (X11; U; Linux i686) Gecko/20030313 Galeon/1.3.4\r\n"
#define ACCEPT             "Accept: text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,video/x-mng,image/png,image/jpeg,image/gif;q=0.2,*/*;q=0.1\r\n"
#define ACCEPT_LANG        "Accept-Language: pl\r\n"
#define ACCEPT_ENCODING    "Accept-Encoding: gzip, deflate, compress;q=0.9\r\n"
#define ACCEPT_CHARSET     "Accept-Charset: ISO-8859-2,utf-8;q=0.7,*;q=0.7\r\n"
#define POST_CONTENT_TYPE  "Content-Type: application/x-www-form-urlencoded\r\n"
#define CONTENT_LENGTH     "Content-Length: "

#define GGADU_SMS_IDEA_TOKEN_LEN 36
#define GGADU_SMS_RECV_BUFF_MAXLEN 32768
#define GGADU_SMS_HTTP_HEADER_MAXLEN 4096

#define IDEA_GFX "/tmp/idea_token.gfx"
#define RESERVED_CHARS	"!\"'()*+-<>[]\\^_`{}|~\t#;/?:&=+,$% \r\n\v\x7f"

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
    ERR_WRITE_TOKEN,
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
