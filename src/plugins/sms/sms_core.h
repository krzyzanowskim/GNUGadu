#ifndef SMS_CORE_PLUGIN_H
#define SMS_CORE_PLUGIN_H 1

#define SMS_IDEA 2
#define SMS_ERA  3
#define SMS_PLUS 4

#define ERR_BAD_TOKEN  2
#define ERR_READ_TOKEN 3
#define ERR_LIMIT_EX   4
#define ERR_GATEWAY    5
#define ERR_SERVICE    6
#define ERR_DISABLE_G  7

#define IDEA_GFX "/tmp/idea_token.gfx"

void send_sms(gboolean external,gchar *sms_sender,gchar *sms_number,gchar *sms_body);

void sms_message(gchar *sms_number, gchar *message);

int send_IDEA_stage2(gchar *pass, gpointer user_data);

typedef struct 
{
    char Command[6];
    char Host[1024];
    char Url[1024];
    char Url_Params[1024];
    char Post_Data[4096];
    int  Post_Length;
} HTTPstruct;

#endif
