/*
** Sms send plugin for GNU Gadu 2
** Bartlomiej Pawlak Copyright (c) 2003
**
**
** This code is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License.
**
*/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "gg-types.h"
#include "unified-types.h"
#include "plugins.h"
#include "signals.h"
#include "menu.h"
#include "support.h"
#include "dialog.h"
#include "sms_gui.h"
#include "sms_core.h"

#define USER_AGENT		"User-Agent: Mozilla/5.0 (X11; U; Linux i686) Gecko/20030313 Galeon/1.3.4\r\n"
#define POST_ACCEPT		"Accept: text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,video/x-mng,image/png,image/jpeg,image/gif;q=0.2,*/*;q=0.1\r\n"
#define POST_ACCEPT_LANG	"Accept-Language: pl\r\n"
#define POST_ACCEPT_ENCODING	"Accept-Encoding: gzip, deflate, compress;q=0.9\r\n"
#define POST_ACCEPT_CHARSET	"Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n"
#define POST_CONTENT_TYPE	"Content-Type: application/x-www-form-urlencoded\r\n"
#define POST_CONTENT_LENGTH	"Content-Length: "

int sock_s;
struct sockaddr_in servAddr;
struct hostent *h;
HTTPstruct HTTP;


/* laczenie sie z wybranym hostem */
int sms_connect(gchar *sms_info, gchar *sms_host)
{
    int sock_r;

    if ((h = gethostbyname(sms_host)) == NULL)
    {
	print_debug("%s : Unknown host\n",sms_info);
	return FALSE;
    }

    sock_s = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_s<0)
    {
	print_debug("%s : Cannot open socket\n",sms_info);
	return FALSE;	
    }

    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(80);
    servAddr.sin_addr = *((struct in_addr *)h->h_addr);
    bzero(&(servAddr.sin_zero), 8);
    sock_r = connect(sock_s, (struct sockaddr *) &servAddr, sizeof (struct sockaddr));

    if (sock_r<0)
    {
	print_debug("%s : Cannot connect\n",sms_info);
	return FALSE;	
    }

    return TRUE;
}

/* tu bedzie wymiana na cos innego, GUI musi to obslugiwac a nie "samowolka" ;-) */
gboolean IDEA_logo(gpointer user_data)
{
    GGaduDialog *d = ggadu_dialog_new();
    ggadu_dialog_set_title(d, _("IDEA token"));
    ggadu_dialog_callback_signal(d, "get token");
    d->user_data = user_data;
    
    ggadu_dialog_add_entry(&(d->optlist), 0, "", VAR_IMG, IDEA_GFX, VAR_FLAG_NONE);
    ggadu_dialog_add_entry(&(d->optlist), 1, _("Enter token text"), VAR_STR, NULL, VAR_FLAG_NONE);
    
    signal_emit("sms", "gui show dialog", d, "main-gui");
    
    return FALSE;
}

/* POST/GET handling */
int HTTP_io(HTTPstruct hdata)
{
    gint   len,i;
    char   io_buf[4096];
    
    snprintf(io_buf, sizeof(io_buf), "%s %s%sHTTP/1.0\r\n",hdata.Command,hdata.Url,hdata.Url_Params);
    len = strlen(io_buf);
    
    if (!strcmp(hdata.Command, "POST"))
    {
	snprintf(io_buf + len, sizeof(io_buf) - len, "%s%s%s%s%s%s%s%s%s%d%s",
		 hdata.Host, "\r\n",
		 USER_AGENT,
		 POST_ACCEPT,
		 POST_ACCEPT_LANG,
		 POST_ACCEPT_ENCODING,
		 POST_ACCEPT_CHARSET,
		 POST_CONTENT_TYPE,
		 POST_CONTENT_LENGTH, hdata.Post_Length,
		 "\r\n\r\n"
		);

        len = strlen(io_buf);
        for (i = 0;i < hdata.Post_Length; i++)
	    if (hdata.Post_Data[i] == ' ')
		hdata.Post_Data[i] = '+';
	memcpy(io_buf + len,hdata.Post_Data,hdata.Post_Length);
        len += hdata.Post_Length;
        io_buf[len++] = 0x0D;
        io_buf[len++] = 0x0A;
        io_buf[len] = 0;
    }
    if (!strcmp(hdata.Command, "GET"))
    {
        io_buf[len++] = 0x0D;
        io_buf[len++] = 0x0A;
        io_buf[len++] = 0x0D;
        io_buf[len++] = 0x0A;
        io_buf[len] = 0;
    }

    send(sock_s, io_buf, len, MSG_WAITALL);
    
    print_debug("\n%s\n",io_buf);

    return TRUE;
}

/* wyslanie na idee */
int send_IDEA(gchar *sms_sender,gchar *sms_number,gchar *sms_body)
{
    gchar *token = NULL;
    gchar *temp = NULL;
    gchar *recv_buff = NULL;
    gint i,j,k;
    FILE *idea_logo;

    /* pobranie adresu do obrazka */
    if (!sms_connect("IDEA","213.218.116.131"))
	return ERR_SERVICE;

    i = 0;
    temp = g_malloc0(2);
    recv_buff = g_malloc0(10000);
    strcpy(HTTP.Command,"GET");
    strcpy(HTTP.Host,"Host: 213.218.116.131");
    strcpy(HTTP.Url,"/default.asp");
    strcpy(HTTP.Url_Params," ");
    HTTP_io(HTTP);
    while (recv(sock_s, temp, 1, MSG_WAITALL))
    {
	recv_buff[i++] = temp[0];
    }
    
    recv_buff = g_strstr_len(recv_buff, i, "rotate_vt.asp?token");
    if (!recv_buff)
	return ERR_READ_TOKEN;

    temp = g_strstr_len(recv_buff, strlen(recv_buff), "\"");
    if (!temp)
    {
	g_free(recv_buff);
	return ERR_READ_TOKEN;
    }

    recv_buff = g_strndup(recv_buff, strlen(recv_buff) - strlen(temp));

    /* pobranie i zapis obazka */
    token = g_strdup(recv_buff);
    token = g_strstr_len(token, strlen(token), "=");
    if (!token)
	return ERR_READ_TOKEN;

    recv_buff = g_strconcat("/", recv_buff, NULL);

    if (!sms_connect("IDEA","213.218.116.131"))
	return ERR_SERVICE;

    i = 0;
    strcpy(HTTP.Command,"GET");
    strcpy(HTTP.Host,"Host: 213.218.116.131");
    strcpy(HTTP.Url,recv_buff);
    strcpy(HTTP.Url_Params," ");
    HTTP_io(HTTP);

    recv_buff = g_malloc0(8000);

    while (recv(sock_s, temp, 1, MSG_WAITALL))
    {
	recv_buff[i++] = temp[0];
    }

    for (j = 0; j < i; j++)
    { 
	if (recv_buff[j] == '\r' && recv_buff[j+1] == '\n' &&
	    recv_buff[j+2] == '\r' && recv_buff[j+3] == '\n')
	    break;
    }
    j+=4;

    if (j >= i)
    {
	g_free(recv_buff);
        return ERR_READ_TOKEN;    
    }


    for (k = 0; k < i-j; k++)
    {
	recv_buff[k] = recv_buff[k+j];
    }
    recv_buff[k] = 0;

    /* oops, bail out if IDEA_GFX cannot be written. */
    if (!(idea_logo = fopen(IDEA_GFX, "w")))
	return FALSE;

    fwrite(recv_buff, 1, i-j, idea_logo);
    fclose(idea_logo);

    recv_buff = g_strconcat("token", token, "&SENDER=", sms_sender, "&RECIPIENT=", sms_number, "&SHORT_MESSAGE=", sms_body, NULL);

    IDEA_logo(recv_buff);

    return TRUE;
}

int send_IDEA_stage2(gchar *pass, gpointer user_data)
{
    gchar *recv_buff;
    gchar temp[2];
    gint i;
    gchar *sms_number = g_malloc0(strlen(user_data));

    sms_number = g_strstr_len(user_data, strlen(user_data), "&RECIPIENT=");
    sms_number = g_strndup(sms_number+11, 9);

    print_debug("token %s, data %s\n", pass, user_data);
    
    if (!pass) {
	sms_message(sms_number, _("Please enter token"));
	return FALSE;
    }
    
    recv_buff = g_strconcat(user_data, "&pass=", pass, "\n", NULL);

    remove(IDEA_GFX);

    if (!sms_connect("IDEA","213.218.116.131")) 
    {
	sms_message(sms_number, _("Cannot connect!"));
	return FALSE;
    }

    strcpy(HTTP.Command,"POST");
    strcpy(HTTP.Host,"Host: 213.218.116.131");
    strcpy(HTTP.Url,"/sendsms.asp");
    strcpy(HTTP.Url_Params," ");
    strcpy(HTTP.Post_Data,recv_buff);
    HTTP.Post_Length = strlen(recv_buff);
    HTTP_io(HTTP);

    i = 0;
    recv_buff = g_malloc0(10000);
    while (recv(sock_s, temp, 1, MSG_WAITALL))
    {
	recv_buff[i++] = temp[0];
    }

    if (!recv_buff)
    {
	sms_message(sms_number, _("Cannot connect!"));
	return FALSE;
    }

    /* sprawdzenie czy doszlo */
    if (g_strstr_len(recv_buff, i, "tekstowa zosta�a wys�ana"))
    {
	sms_message(sms_number, _("SMS has been sent"));
	g_free(recv_buff);
	return TRUE;
    }

    if (g_strstr_len(recv_buff, i, "Object moved"))
    {
	sms_message(sms_number, _("Bad token entered!"));
	g_free(recv_buff);
	return FALSE;
    }

    if (g_strstr_len(recv_buff, i, "wyczerpany"))
    {
	sms_message(sms_number, _("Daily limit exceeded!"));
	g_free(recv_buff);
	return FALSE;
    }

    if (g_strstr_len(recv_buff, i, "serwis chwilowo"))
    {
	sms_message(sms_number, _("Gateway error!"));
	g_free(recv_buff);
	return FALSE;
    }

    if (g_strstr_len(recv_buff, i, "Odbiorca nie ma aktywnej"))
    {
	sms_message(sms_number, _("Service not activated!"));
	g_free(recv_buff);
	return FALSE;
    }

    if (g_strstr_len(recv_buff, i, "adres odbiorcy wiadomosci jest nieprawid"))
    {
	sms_message(sms_number, _("Invalid number"));
	g_free(recv_buff);
	return FALSE;
    }

    g_free(recv_buff);
    return FALSE;
}

/* wyslanie na plusa */
int send_PLUS(gchar *sms_sender,gchar *sms_number,gchar *sms_body)
{
    gchar *recv_buff = NULL;
    gchar *recv_buff_1 = NULL;
    gchar tprefix[4];
    gint i;
    gchar temp[2];
    
    if (!sms_connect("PLUS","www.text.plusgsm.pl"))
	return ERR_SERVICE;

    strncpy(tprefix, sms_number, 3);
    tprefix[3] = 0;
    recv_buff_1 = g_strconcat("tprefix=",tprefix,"&numer=",(sms_number+3),"&odkogo=",sms_sender,"&tekst=",sms_body, NULL);
    
    strcpy(HTTP.Command,"POST");
    strcpy(HTTP.Host,"Host: www.text.plusgsm.pl");
    strcpy(HTTP.Url,"/sms/sendsms.php");
    strcpy(HTTP.Url_Params," ");
    strcpy(HTTP.Post_Data,recv_buff_1);
    HTTP.Post_Length = strlen(recv_buff_1);
    HTTP_io(HTTP);
    g_free(recv_buff_1);

    i = 0;
    recv_buff = g_malloc0(10000);
    while (recv(sock_s, temp, 1, MSG_WAITALL))
    {
	recv_buff[i++] = temp[0];
    }

    if (!recv_buff)
	return ERR_SERVICE;
    
    /* sprawdzenie czy doszlo */
    if (g_strstr_len(recv_buff, i, "Twoja wiadomo�� zosta�a wys�ana na numer"))
    {
        g_free(recv_buff);
	return TRUE;
    }
    
    if (g_strstr_len(recv_buff, i, "podano z�y numer"))
    {
        g_free(recv_buff);
	return FALSE;
    }

    if (g_strstr_len(recv_buff, i, "Z powodu przekroczenia limit�w bramki"))
    {
        g_free(recv_buff);
	return ERR_LIMIT_EX;
    }

    g_free(recv_buff);
    return FALSE;
}

/* wyslanie na ere */
int send_ERA(gchar *sms_sender,gchar *sms_number,gchar *sms_body)
{
    gchar *temp = g_malloc0(10);
    gchar *token_name = g_malloc0(10);
    gchar *token = g_malloc0(10);
    gchar *recv_buff = g_malloc0(10000);
    gchar *cookie = g_malloc0(2000);
    gint i;

    if (!sms_connect("ERA","boa.eragsm.com.pl"))
	return ERR_SERVICE;

    strcpy(HTTP.Command,"POST");
    strcpy(HTTP.Host,"Host: boa.eragsm.com.pl");
    strcpy(HTTP.Url,"/sms/sendsms.asp");
    strcpy(HTTP.Url_Params," ");
    strcpy(HTTP.Post_Data,"sms=1");
    HTTP.Post_Length = 5;
    HTTP_io(HTTP);
    i = 0;
    while (recv(sock_s, temp, 1, MSG_WAITALL))
    {
	recv_buff[i++] = temp[0];
    }

    if (g_strstr_len(recv_buff, i, "Serwis jest chwilowo niedostepny"))
    {
        g_free(recv_buff);
	return ERR_GATEWAY;
    }

    recv_buff = g_strstr_len(recv_buff, i, "Cookie:");
    if (!recv_buff)
	return ERR_READ_TOKEN;

    cookie = g_strstr_len(recv_buff, strlen(recv_buff), ";");
    if (!cookie)
	return ERR_READ_TOKEN;

    cookie = g_strndup(recv_buff, strlen(recv_buff) - strlen(cookie));

    recv_buff = g_strstr_len(recv_buff, strlen(recv_buff), "\"Kod");
    if (!recv_buff)
	return ERR_READ_TOKEN;

    token_name = g_strndup(recv_buff + 1, 5);

    recv_buff = g_strstr_len(recv_buff, strlen(recv_buff), "=\"");
    if (!recv_buff)
	return ERR_READ_TOKEN;

    token = g_strndup(recv_buff + 2, 4);

    if (token[2] == '"')
	token[2] = 0;
    if (token_name[4] == '"')
	token_name[4] = 0;
    
    if (!sms_connect("ERA","boa.eragsm.com.pl"))
	return ERR_SERVICE;

    recv_buff = g_strconcat("Referer: http://boa.eragsm.com.pl/sms/sendsms.asp\n", cookie, "\nHost: boa.eragsm.com.pl", NULL);
    strcpy(HTTP.Command,"POST");
    strcpy(HTTP.Host,recv_buff);
    strcpy(HTTP.Url,"/sms/sendsms.asp");
    strcpy(HTTP.Url_Params," ");
    recv_buff = g_strconcat("Telefony=Telefony&Kasuj=nie-skasowa�&", token_name, "=", token, "&kod=", token, "&Send=  tak-nada�  &kontakt=&podpis=", sms_sender, "&messagebis=", sms_body, "&message=", sms_body, "&ksiazka=ksi��ka telefoniczna&bookopen=&numer=", sms_number, NULL);
    strcpy(HTTP.Post_Data, recv_buff);
    HTTP.Post_Length = strlen(recv_buff);
    HTTP_io(HTTP);

    i = 0;
    recv_buff = g_malloc0(10000);
    while (recv(sock_s, temp, 1, MSG_WAITALL))
    {
	recv_buff[i++] = temp[0];
    }

    g_free(temp);

    if (!recv_buff)
	return ERR_SERVICE;

    /* sprawdzenie czy doszlo */
    if (g_strstr_len(recv_buff, i, "zosta�a wys�ana!"))
    {
        g_free(recv_buff);
	return TRUE;
    }

    if (g_strstr_len(recv_buff, i, "B��d autoryzacji"))
    {
        g_free(recv_buff);
	return ERR_GATEWAY;
    }

    g_free(recv_buff);

    return FALSE;
}

/* sprawdzenie jaka siec */
int check_operator(gchar *sms_number)
{
    if (strlen(sms_number) != 9)
	return FALSE;

    if (*sms_number == '5')
	return SMS_IDEA;
	
    if (*sms_number == '6') {
	if ((sms_number[2] - '0') % 2) 
	    return SMS_PLUS;
	else 
	    return SMS_ERA;
    }
    
    return FALSE;
}

void sms_message(gchar *sms_number, gchar *message)
{
    if (method == GGADU_SMS_METHOD_POPUP)
	signal_emit("sms", "gui show warning", g_strdup_printf("%s", message), "main-gui");

    if (method == GGADU_SMS_METHOD_CHAT) {
	GGaduMsg *msg = g_new0(GGaduMsg,1);
	msg->id = (sms_number ? sms_number : g_strdup(_("None")));
	msg->class = GGADU_CLASS_CHAT;
	msg->message = g_strconcat(_("SMS plugin : "), message, NULL);
	signal_emit("sms", "gui msg receive", msg, "main-gui");
    }
}

/* wywolanie z sms_gui.c , tutaj wybiera co zrobic */
void send_sms(gboolean external,gchar *sms_sender,gchar *sms_number,gchar *sms_body)
{
    gint result, gsm_oper;
    
    if (!sms_number) 
    {
	sms_message(sms_number, _("Specify recipient number!"));
	return;
    }

    if (!sms_sender) 
    {
	sms_message(sms_number, _("Specify sender name!"));
	return;
    }

    if (!sms_body) 
    {
	sms_message(sms_number, _("Message is empty!"));
	return;
    }
    
    if (*sms_number == '+') {
	*sms_number++;

	if (sms_number[0] == '4' && sms_number[1] == '8')
            sms_number += 2;

    } else if (sms_number[0] == '0')
        *sms_number++;


    gsm_oper = check_operator(sms_number);
    switch (gsm_oper)
    {
	case SMS_IDEA:
	    if (external) 
	    {
		sms_message(sms_number, _("IDEA does not work this way!"));
		return;
	    }
	    else
	        result = send_IDEA(sms_sender,sms_number,sms_body);

	    break;

	case SMS_PLUS:
	    if (external) 
	    {
	        result = system(g_strconcat("sms ", sms_number, " \"", sms_body, " ",sms_sender, "\"", NULL));
		return;
	    }
	    else
	        result = send_PLUS(sms_sender,sms_number,sms_body);

	    break;

        case SMS_ERA:
	    if (external) 
	    {
		result = system(g_strconcat("sms ", sms_number, " \"", sms_body, " ",sms_sender, "\"", NULL));
		return;
	    }
	    else
	        result = send_ERA(sms_sender,sms_number,sms_body);

	    break;

	case FALSE:
	    sms_message(sms_number, _("Invalid number!"));
	    return;	    
    }

    if (!result) 
    {
	sms_message(sms_number, _("SMS not delivered!"));
	return;
    }

    if (result == 1 && method == GGADU_SMS_METHOD_CHAT && gsm_oper != SMS_IDEA)
    {
	sms_message(sms_number, _("SMS has been sent"));
	return;
    }

    switch (result)
    {
	case ERR_READ_TOKEN:
	    sms_message(sms_number, _("Token not found!"));
	    break;
	case ERR_LIMIT_EX:
	    sms_message(sms_number, _("Daily limit exceeded!"));
	    break;
	case ERR_GATEWAY:
	    sms_message(sms_number, _("Gateway error!"));
	    break;
	case ERR_SERVICE:
	    sms_message(sms_number, _("Cannot connect!"));
	    break;
    }
    
    return;
}
