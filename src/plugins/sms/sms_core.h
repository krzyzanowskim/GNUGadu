/* $Id: sms_core.h,v 1.10 2003/09/24 14:19:31 shaster Exp $ */

#ifndef SMS_CORE_PLUGIN_H
#define SMS_CORE_PLUGIN_H 1

#define GGADU_SMS_PLUS_HOST 		"www.text.plusgsm.pl"
#define GGADU_SMS_PLUS_URL 		"/sms/sendsms.php"

#define GGADU_SMS_IDEA_HOST 		"sms.idea.pl"
#define GGADU_SMS_IDEA_URL_GET 		"/"
#define GGADU_SMS_IDEA_URL_SEND		"/sendsms.aspx"

#define GGADU_SMS_ERA_HOST 	"www.eraomnix.pl"
#define GGADU_SMS_ERA_URL 	"/sms/do/extern/tinker/free/send"

#define USER_AGENT          "User-Agent: Mozilla/5.0 (X11; U; Linux i686) Gecko/20030313 Galeon/1.3.4\r\n"
#define ACCEPT              "Accept: text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,video/x-mng,image/png,image/jpeg,image/gif;q=0.2,*/*;q=0.1\r\n"
#define ACCEPT_LANG         "Accept-Language: pl\r\n"
#define ACCEPT_ENCODING     "Accept-Encoding: gzip, deflate, compress;q=0.9\r\n"
#define ACCEPT_CHARSET      "Accept-Charset: ISO-8859-2,utf-8;q=0.7,*;q=0.7\r\n"
#define POST_CONTENT_TYPE   "Content-Type: application/x-www-form-urlencoded\r\n"
#define CONTENT_LENGTH      "Content-Length: "

#define GGADU_SMS_RECVBUFF_LEN 32768

#define IDEA_GFX "/tmp/idea_token.gfx"
#define GGADU_SMS_IDEA_TOKENLEN 36
#define RESERVED_CHARS	"!\"'()*+-.<>[]\\^_`{}|~\t#;/?:&=+,$% \r\n\v\x7f"

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

void send_sms(gboolean external, const gchar * sms_sender, gchar * sms_number, const gchar * sms_body,
	      const gchar * eralogin, const gchar * erapass);

void sms_message(gchar * sms_number, gchar * message);
void sms_warning(gchar * sms_number, gchar * warning);

int send_IDEA_stage2(gchar * pass, gpointer user_data);

gchar *ggadu_sms_urlencode(gchar * source);
gchar *ggadu_sms_append_char(gchar * url_string, gchar char_to_append, gboolean convert_to_hex);

typedef struct
{
    char method[6];
    char host[1024];
    char url[1024];
    char url_params[1024];
    char post_data[4096];
    int post_length;
} HTTPstruct;

#endif
