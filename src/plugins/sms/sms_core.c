/* $Id: sms_core.c,v 1.11 2003/09/10 22:31:59 shaster Exp $ */

/*
 * Sms send plugin for GNU Gadu 2
 * Bartlomiej Pawlak Copyright (c) 2003
 *
 *
 * This code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>

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


/* URLencoding code by Ahmad Baitalmal <ahmad@bitbuilder.com 
 * with tweaks by me
 */
/*
   Not the most effecient way of doing this,
   If you can come up with something better
   let me know,
   Ahmad Baitalmal <ahmad@bitbuilder.com>
 */

gchar *ggadu_sms_formencode(gchar * source)
{
    gint length = strlen(source);
    gint i;
    gchar *encoded;

    g_return_val_if_fail(source != NULL, source);
    encoded = g_strdup("");
    for (i = 0; i < length; i++)
    {
	encoded = ggadu_sms_append_char(encoded, source[i], (gboolean) strchr(RESERVED_CHARS, source[i]));
    }
    g_free(source);
    return encoded;
}

gchar *ggadu_sms_append_char(gchar * url_string, gchar char_to_append, gboolean convert_to_hex)
{
    gchar *new_string;
    if (convert_to_hex)
    {
	switch (char_to_append)
	{
	case ' ':
	    new_string = g_strdup_printf("%s+", url_string);
	    break;
	case '\n':
	    new_string = g_strdup_printf("%s%%0D%%%02X", url_string, char_to_append);
	    break;
	default:
	    new_string = g_strdup_printf("%s%%%02X", url_string, char_to_append);
	}
    }
    else
	new_string = g_strdup_printf("%s%c", url_string, char_to_append);

    g_free(url_string);
    return new_string;
}

/* laczenie sie z wybranym hostem */
int sms_connect(gchar * sms_info, gchar * sms_host)
{
    int sock_r;

    if ((h = gethostbyname(sms_host)) == NULL)
    {
	print_debug("%s : Unknown host\n", sms_info);
	return FALSE;
    }

    sock_s = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_s < 0)
    {
	print_debug("%s : Cannot open socket\n", sms_info);
	return FALSE;
    }

    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(80);
    servAddr.sin_addr = *((struct in_addr *) h->h_addr);
    bzero(&(servAddr.sin_zero), 8);
    sock_r = connect(sock_s, (struct sockaddr *) &servAddr, sizeof (struct sockaddr));

    if (sock_r < 0)
    {
	print_debug("%s : Cannot connect\n", sms_info);
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
    gint len, i;
    char io_buf[4096];

    snprintf(io_buf, sizeof (io_buf), "%s %s%sHTTP/1.0\r\n", hdata.Command, hdata.Url, hdata.Url_Params);
    len = strlen(io_buf);

    if (!strcmp(hdata.Command, "POST"))
    {
	snprintf(io_buf + len, sizeof (io_buf) - len, "%s%s%s%s%s%s%s%s%s%d%s", hdata.Host, "\r\n", USER_AGENT,
		 POST_ACCEPT, POST_ACCEPT_LANG, POST_ACCEPT_ENCODING, POST_ACCEPT_CHARSET, POST_CONTENT_TYPE,
		 POST_CONTENT_LENGTH, hdata.Post_Length, "\r\n\r\n");

	len = strlen(io_buf);
	for (i = 0; i < hdata.Post_Length; i++)
	    if (hdata.Post_Data[i] == ' ')
		hdata.Post_Data[i] = '+';
	memcpy(io_buf + len, hdata.Post_Data, hdata.Post_Length);
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

    print_debug("\n%s\n", io_buf);

    return TRUE;
}

/* wyslanie na idee */
int send_IDEA(gchar * sms_sender, gchar * sms_number, gchar * sms_body)
{
    gchar *token = NULL;
    gchar *temp = NULL;
    gchar *recv_buff = NULL;
    gint i, j, k;
    FILE *idea_logo;

    /* pobranie adresu do obrazka */
    if (!sms_connect("IDEA", "213.218.116.131"))
	return ERR_SERVICE;

    i = 0;
    temp = g_malloc0(2);
    recv_buff = g_malloc0(30000);
    strcpy(HTTP.Command, "GET");
    strcpy(HTTP.Host, "Host: 213.218.116.131");
    strcpy(HTTP.Url, "/default.aspx");
    strcpy(HTTP.Url_Params, " ");
    HTTP_io(HTTP);
    while (recv(sock_s, temp, 1, MSG_WAITALL))
    {
	recv_buff[i++] = temp[0];
    }

    recv_buff = g_strstr_len(recv_buff, i, "rotate_token.aspx?token");
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

    if (!sms_connect("IDEA", "213.218.116.131"))
	return ERR_SERVICE;

    i = 0;
    strcpy(HTTP.Command, "GET");
    strcpy(HTTP.Host, "Host: 213.218.116.131");
    strcpy(HTTP.Url, recv_buff);
    strcpy(HTTP.Url_Params, " ");
    HTTP_io(HTTP);

    recv_buff = g_malloc0(8000);

    while (recv(sock_s, temp, 1, MSG_WAITALL))
    {
	recv_buff[i++] = temp[0];
    }

    for (j = 0; j < i; j++)
    {
	if (recv_buff[j] == '\r' && recv_buff[j + 1] == '\n' && recv_buff[j + 2] == '\r' && recv_buff[j + 3] == '\n')
	    break;
    }
    j += 4;

    if (j >= i)
    {
	g_free(recv_buff);
	return ERR_READ_TOKEN;
    }


    for (k = 0; k < i - j; k++)
    {
	recv_buff[k] = recv_buff[k + j];
    }
    recv_buff[k] = 0;

    /* oops, bail out if IDEA_GFX cannot be written. */
    if (!(idea_logo = fopen(IDEA_GFX, "w")))
	return FALSE;

    fwrite(recv_buff, 1, i - j, idea_logo);
    fclose(idea_logo);

    recv_buff =
	g_strconcat("token", token, "&SENDER=", sms_sender, "&RECIPIENT=", sms_number, "&SHORT_MESSAGE=", sms_body,
		    NULL);

    IDEA_logo(recv_buff);

    return TRUE;
}

int send_IDEA_stage2(gchar * pass, gpointer user_data)
{
    gchar *recv_buff;
    gchar temp[2];
    gint i;
    gchar *sms_number = g_malloc0(strlen(user_data));

    sms_number = g_strstr_len(user_data, strlen(user_data), "&RECIPIENT=");
    sms_number = g_strndup(sms_number + 11, 9);

    print_debug("token %s, data %s\n", pass, user_data);

    if (!pass)
    {
	sms_warning(sms_number, _("Please enter token"));
	return FALSE;
    }

    recv_buff = g_strconcat(user_data, "&pass=", pass, "\n", NULL);

    remove(IDEA_GFX);

    if (!sms_connect("IDEA", "213.218.116.131"))
    {
	sms_warning(sms_number, _("Cannot connect!"));
	return FALSE;
    }

    strcpy(HTTP.Command, "POST");
    strcpy(HTTP.Host, "Host: 213.218.116.131");
    strcpy(HTTP.Url, "/sendsms.aspx");
    strcpy(HTTP.Url_Params, " ");
    strcpy(HTTP.Post_Data, recv_buff);
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
	sms_warning(sms_number, _("Cannot connect!"));
	return FALSE;
    }

    print_debug("recv_buff:\n%s\n", recv_buff);

    /* sprawdzenie czy doszlo */
    if (g_strstr_len(recv_buff, i, "tekstowa zosta³a wys³ana"))
    {
	sms_message(sms_number, _("SMS has been sent"));
	g_free(recv_buff);
	return TRUE;
    }

    if (g_strstr_len(recv_buff, i, "Podano b³êdne has³o, SMS nie zosta³ wys³any"))
    {
	sms_warning(sms_number, _("SMS has NOT been sent!"));
	g_free(recv_buff);
	return FALSE;
    }

    if (g_strstr_len(recv_buff, i, "Object moved"))
    {
	sms_warning(sms_number, _("Bad token entered!"));
	g_free(recv_buff);
	return FALSE;
    }

    if (g_strstr_len(recv_buff, i, "wyczerpany"))
    {
	sms_warning(sms_number, _("Daily limit exceeded!"));
	g_free(recv_buff);
	return FALSE;
    }

    if (g_strstr_len(recv_buff, i, "serwis chwilowo"))
    {
	sms_warning(sms_number, _("Gateway error!"));
	g_free(recv_buff);
	return FALSE;
    }

    if (g_strstr_len(recv_buff, i, "Odbiorca nie ma aktywnej"))
    {
	sms_warning(sms_number, _("Service not activated!"));
	g_free(recv_buff);
	return FALSE;
    }

    if (g_strstr_len(recv_buff, i, "adres odbiorcy wiadomosci jest nieprawid"))
    {
	sms_warning(sms_number, _("Invalid number"));
	g_free(recv_buff);
	return FALSE;
    }

    g_free(recv_buff);
    return FALSE;
}

/* wyslanie na plusa */
int send_PLUS(gchar * sms_sender, gchar * sms_number, gchar * sms_body)
{
    gchar *recv_buff = NULL;
    gchar *recv_buff_1 = NULL;
    gchar tprefix[4];
    gint i;
    gchar temp[2];

    if (!sms_connect("PLUS", "www.text.plusgsm.pl"))
	return ERR_SERVICE;

    strncpy(tprefix, sms_number, 3);
    tprefix[3] = 0;
    recv_buff_1 =
	g_strconcat("tprefix=", tprefix, "&numer=", (sms_number + 3), "&odkogo=", sms_sender, "&tekst=", sms_body,
		    NULL);

    strcpy(HTTP.Command, "POST");
    strcpy(HTTP.Host, "Host: www.text.plusgsm.pl");
    strcpy(HTTP.Url, "/sms/sendsms.php");
    strcpy(HTTP.Url_Params, " ");
    strcpy(HTTP.Post_Data, recv_buff_1);
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
    if (g_strstr_len(recv_buff, i, "Twoja wiadomo¶æ zosta³a wys³ana na numer"))
    {
	g_free(recv_buff);
	return TRUE;
    }

    if (g_strstr_len(recv_buff, i, "podano z³y numer"))
    {
	g_free(recv_buff);
	return FALSE;
    }

    if (g_strstr_len(recv_buff, i, "Z powodu przekroczenia limitów bramki"))
    {
	g_free(recv_buff);
	return ERR_LIMIT_EX;
    }

    g_free(recv_buff);
    return FALSE;
}

/* wyslanie na ere */
int send_ERA(gchar * sms_sender, gchar * sms_number, gchar * sms_body, gchar * eraomnix_login,
	     gchar * eraomnix_password)
{
    gchar *recv_buff = NULL;
    gchar *post = NULL;
    gchar *returncode = NULL;
    gint i;
    gchar temp[2];

    if (!sms_connect("ERA", "www.eraomnix.pl"))
	return ERR_SERVICE;

    /* *INDENT-OFF* */
    post = g_strconcat ("login=", eraomnix_login, "&password=", eraomnix_password,
			"&number=48", sms_number,
			"&message=", ggadu_sms_formencode(g_strdup(sms_body)),
			"&contact=", "&signature=", ggadu_sms_formencode(g_strdup(sms_sender)),
			"&success=OK", "&failure=FAIL", 
			"&minute=", "&hour=", NULL);
    /* *INDENT-ON* */

    strcpy(HTTP.Command, "POST");
    strcpy(HTTP.Host, "Host: www.eraomnix.pl");
    strcpy(HTTP.Url, "/sms/do/extern/tinker/free/send");
    strcpy(HTTP.Url_Params, " ");
    strcpy(HTTP.Post_Data, post);
    HTTP.Post_Length = strlen(post);
    HTTP_io(HTTP);
    g_free(post);

    i = 0;
    recv_buff = g_malloc0(10000);
    while (recv(sock_s, temp, 1, MSG_WAITALL))
    {
	recv_buff[i++] = temp[0];
    }

    if (!recv_buff)
	return ERR_SERVICE;

    /* sprawdzenie czy doszlo */
    if ((returncode = g_strstr_len(recv_buff, i, "OK?X-ERA-counter=")) != NULL)
    {
	gchar *r = g_strndup(returncode + strlen("OK?X-ERA-counter="), 1);
	sms_message(sms_number, g_strdup_printf(_("SMS has been sent. Left: %s"), r));
	g_free(r);
	g_free(recv_buff);
	return ERR_NONE;
    }

    if ((returncode = g_strstr_len(recv_buff, i, "FAIL?X-ERA-error=")) != NULL)
    {
	gchar *r = g_strndup(returncode + strlen("FAIL?X-ERA-error="), 1);
	int ret = ERR_UNKNOWN;

	/* *INDENT-OFF* */
	/* error codes from www.eraomnix.pl/service/gw/bAPIPrv.jsp */
	if (*r == '0') ret = TRUE;
	if (*r == '1') ret = ERR_GATEWAY;
	if (*r == '2') ret = ERR_UNAUTH;
	if (*r == '3') ret = ERR_ACCESS_DENIED;
	if (*r == '5') ret = ERR_SYNTAX;
	if (*r == '7') ret = ERR_LIMIT_EX;
	if (*r == '8') ret = ERR_BAD_RCPT;
	if (*r == '9') ret = ERR_MSG_TOO_LONG;
	/* *INDENT-ON* */

	g_free(r);
	g_free(recv_buff);
	return ret;
    }

    g_free(recv_buff);
    return FALSE;
}

/* sprawdzenie jaka siec */
int check_operator(gchar * sms_number)
{
    if (strlen(sms_number) != 9)
	return FALSE;

    if (*sms_number == '5')
	return SMS_IDEA;

    if (*sms_number == '6')
    {
	if ((sms_number[2] - '0') % 2)
	    return SMS_PLUS;
	else
	    return SMS_ERA;
    }

    return FALSE;
}

void sms_dialog_box(gchar * sms_number, gchar * message, gint type)
{
    if (method == GGADU_SMS_METHOD_POPUP)
    {
	if (type == GGADU_SMS_TYPE_WARN)
	    signal_emit("sms", "gui show warning", g_strdup_printf("%s", message), "main-gui");
	else if (type == GGADU_SMS_TYPE_INFO)
	    signal_emit("sms", "gui show message", g_strdup_printf("%s", message), "main-gui");
    }

    if (method == GGADU_SMS_METHOD_CHAT)
    {
	GGaduMsg *msg = g_new0(GGaduMsg, 1);
	msg->id = (sms_number ? sms_number : g_strdup(_("None")));
	msg->class = GGADU_CLASS_CHAT;
	msg->message = g_strconcat(_("SMS plugin : "), message, NULL);
	signal_emit("sms", "gui msg receive", msg, "main-gui");
    }
}

void sms_message(gchar * sms_number, gchar * message)
{
    sms_dialog_box(sms_number, message, GGADU_SMS_TYPE_INFO);
}

void sms_warning(gchar * sms_number, gchar * warning)
{
    sms_dialog_box(sms_number, warning, GGADU_SMS_TYPE_WARN);
}

/* wywolanie z sms_gui.c , tutaj wybiera co zrobic */
void send_sms(gboolean external, gchar * sms_sender, gchar * sms_number, gchar * sms_body, gchar * eraomnix_login,
	      gchar * eraomnix_password)
{
    gint result, gsm_oper;

    if (!sms_number)
    {
	sms_warning(sms_number, _("Specify recipient number!"));
	return;
    }

    if (!sms_sender)
    {
	sms_warning(sms_number, _("Specify sender name!"));
	return;
    }

    if (!sms_body)
    {
	sms_warning(sms_number, _("Message is empty!"));
	return;
    }

    if (g_str_has_prefix(sms_number, "+48"))
	sms_number += 3;

    if (g_str_has_prefix(sms_number, "0"))
	sms_number++;

    gsm_oper = check_operator(sms_number);
    switch (gsm_oper)
    {
    case SMS_IDEA:
	if (external)
	{
	    sms_warning(sms_number, _("IDEA does not work this way!"));
	    return;
	}
	else
	    result = send_IDEA(sms_sender, sms_number, sms_body);

	break;

    case SMS_PLUS:
	if (external)
	{
	    result = system(g_strconcat("sms ", sms_number, " \"", sms_body, " ", sms_sender, "\"", NULL));
	    return;
	}
	else
	    result = send_PLUS(sms_sender, sms_number, sms_body);

	break;

    case SMS_ERA:
	if (external)
	{
	    result = system(g_strconcat("sms ", sms_number, " \"", sms_body, " ", sms_sender, "\"", NULL));
	    return;
	}
	else
	{
	    if (!eraomnix_login)
	    {
		sms_warning(sms_number, _("Empty EraOmnix login!"));
		return;
	    }

	    if (!eraomnix_password)
	    {
		sms_warning(sms_number, _("Empty EraOmnix password!"));
		return;
	    }
	    result = send_ERA(sms_sender, sms_number, sms_body, eraomnix_login, eraomnix_password);
	}

	break;

    case FALSE:
	sms_warning(sms_number, _("Invalid number!"));
	return;
    }

    if (result == FALSE)
    {
	sms_warning(sms_number, _("SMS not delivered!"));
	return;
    }

    if (result == TRUE && method == GGADU_SMS_METHOD_CHAT && gsm_oper != SMS_IDEA)
    {
	sms_message(sms_number, _("SMS has been sent"));
	return;
    }

    switch (result)
    {
    case ERR_UNAUTH:
	sms_warning(sms_number, _("Unauthorized!"));
	break;
    case ERR_ACCESS_DENIED:
	sms_warning(sms_number, _("Access denied!"));
	break;
    case ERR_SYNTAX:
	sms_warning(sms_number, _("Syntax error!"));
	break;
    case ERR_BAD_RCPT:
	sms_warning(sms_number, _("Invalid recipient!"));
	break;
    case ERR_MSG_TOO_LONG:
	sms_warning(sms_number, _("Message too long!"));
	break;
    case ERR_READ_TOKEN:
	sms_warning(sms_number, _("Token not found!"));
	break;
    case ERR_LIMIT_EX:
	sms_warning(sms_number, _("Daily limit exceeded!"));
	break;
    case ERR_GATEWAY:
	sms_warning(sms_number, _("Gateway error!"));
	break;
    case ERR_SERVICE:
	sms_warning(sms_number, _("Cannot connect!"));
	break;
    case ERR_UNKNOWN:
	sms_warning(sms_number, _("Unknown error!"));
	break;
    }

    return;
}
