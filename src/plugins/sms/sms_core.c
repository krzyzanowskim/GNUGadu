/* $Id: sms_core.c,v 1.21 2003/11/07 22:57:18 shaster Exp $ */

/*
 * Sms send plugin for GNU Gadu 2
 * Bartlomiej Pawlak Copyright (c) 2003
 *
 * Copyright (C) 2003 GNU Gadu Team
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
#include <unistd.h>

#include "gg-types.h"
#include "unified-types.h"
#include "plugins.h"
#include "signals.h"
#include "menu.h"
#include "support.h"
#include "dialog.h"
#include "sms_gui.h"
#include "sms_core.h"

int sock_s;
struct sockaddr_in servAddr;
struct hostent *h;
HTTPstruct HTTP;


/* URLencoding code by Ahmad Baitalmal <ahmad@bitbuilder.com>
 * with tweaks by Jakub Jankowski <shasta@atn.pl>
 */
gchar *ggadu_sms_urlencode(gchar * source)
{
    gint length = strlen(source);
    gint i;
    gchar *encoded;

    g_return_val_if_fail(source != NULL, source);
    encoded = g_strdup("");

    for (i = 0; i < length; i++)
	encoded = ggadu_sms_append_char(encoded, source[i], (gboolean) strchr(RESERVED_CHARS, source[i]));

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

void sms_disconnect(void)
{
    if (sock_s >= 0)
	close(sock_s);

    sock_s = -1;

    return;
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

    /* close any remaining socket. */
    sms_disconnect();

    sock_s = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_s < 0)
    {
	print_debug("%s : Cannot open socket\n", sms_info);
	sock_s = -1;
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
	sms_disconnect();
	return FALSE;
    }

    return TRUE;
}

/* POST/GET handling */
int HTTP_io(HTTPstruct hdata)
{
    gchar *io_buf = NULL;

    /* *INDENT-OFF* */
    if (!strcmp(hdata.method, "POST"))
	io_buf = g_strdup_printf("POST %s%sHTTP/1.0\r\nHost: %s\r\n%s%s%s%s%s%s%s%d\r\n\r\n%s",
			    hdata.url, hdata.url_params, hdata.host, USER_AGENT,
			    ACCEPT, ACCEPT_LANG, ACCEPT_ENCODING, ACCEPT_CHARSET,
			    POST_CONTENT_TYPE, CONTENT_LENGTH, hdata.post_length, hdata.post_data);
    else if (!strcmp(hdata.method, "GET"))
	io_buf = g_strdup_printf("GET %s%sHTTP/1.0\r\nHost: %s\r\n%s\r\n\r\n",
			    hdata.url, hdata.url_params, hdata.host, USER_AGENT);
    else
	io_buf = g_strdup("GET /c-programming-howto.html HTTP/1.0\r\n\r\n");
    /* *INDENT-ON* */

    print_debug("Sending:\n%s\n", io_buf);
    send(sock_s, io_buf, strlen(io_buf), MSG_WAITALL);
    g_free(io_buf);

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

/* This should get sms_number in the same form as in userlist.
 * Otherwise, if GGADU_SMS_METHOD_CHAT, you'll get messages from different
 * number than you've sent msgs. It's better to cut prefixes while loading userlist
 * than here.
 */
void sms_dialog_box(const gchar * sms_number, const gchar * message, gint type)
{
    if (method == GGADU_SMS_METHOD_POPUP)
    {
	if (type == GGADU_SMS_TYPE_WARN)
	    signal_emit("sms", "gui show warning", g_strdup(message), "main-gui");
	else if (type == GGADU_SMS_TYPE_INFO)
	    signal_emit("sms", "gui show message", g_strdup(message), "main-gui");
    }

    if (method == GGADU_SMS_METHOD_CHAT)
    {
	GGaduMsg *msg = g_new0(GGaduMsg, 1);
	msg->id = g_strdup(sms_number ? sms_number : _("None"));
	msg->class = GGADU_CLASS_CHAT;
	msg->message = g_strconcat(_("SMS plugin: "), message, NULL);
	signal_emit("sms", "gui msg receive", msg, "main-gui");
    }
}

/* handy wrapper */
void sms_message(const gchar * sms_number, const gchar * message)
{
    sms_dialog_box(sms_number, message, GGADU_SMS_TYPE_INFO);
}

/* handy wrapper */
void sms_warning(const gchar * sms_number, const gchar * warning)
{
    sms_dialog_box(sms_number, warning, GGADU_SMS_TYPE_WARN);
}

/* wyslanie na idee */
int send_IDEA(const gchar * sms_sender, const gchar * sms_number, const gchar * sms_body)
{
    gchar *token = NULL;
    gchar temp[2];
    gchar *gettoken = NULL;
    gchar *recv_buff = NULL;
    gchar *buf = NULL;
    gint i = 0, j, k, retries = 3;
    FILE *idea_logo;

get_mainpage:
    /* pobranie adresu do obrazka */
    if (!sms_connect("IDEA", "213.218.116.131"))
	return ERR_SERVICE;

    strcpy(HTTP.method, "GET");
    strcpy(HTTP.host, GGADU_SMS_IDEA_HOST);
    strcpy(HTTP.url, GGADU_SMS_IDEA_URL_GET);
    strcpy(HTTP.url_params, " ");
    HTTP_io(HTTP);

    recv_buff = g_malloc0(GGADU_SMS_RECVBUFF_LEN);

    i = 0;
    while (recv(sock_s, temp, 1, MSG_WAITALL) && i < GGADU_SMS_RECVBUFF_LEN)
	recv_buff[i++] = temp[0];

    sms_disconnect();

    print_debug("\n=======retries left: %d=====\nIDEA RECVBUFF1: %s\n\n", retries-1, recv_buff);

    if (!g_strstr_len(recv_buff, i, "200 OK"))
    {
	g_free(recv_buff);
	if (--retries > 0)
	    goto get_mainpage;
	else
	    return ERR_GATEWAY;
    } else
	retries = 3;

    if (!(buf = g_strstr_len(recv_buff, i, "rotate_token.aspx?token=")))
    {
	g_free(recv_buff);
	return ERR_READ_TOKEN;
    }

    if (!(token = g_strndup(buf+24, GGADU_SMS_IDEA_TOKENLEN)))
    {
	g_free(recv_buff);
	return ERR_READ_TOKEN;
    }

    if (strlen(token) < GGADU_SMS_IDEA_TOKENLEN)
    {
	g_free(token);
	g_free(recv_buff);
	return ERR_READ_TOKEN;
    }

    gettoken = g_strconcat("/", "rotate_token.aspx?token=", token, NULL);
    g_free(recv_buff);

get_token:
    if (!sms_connect("IDEA", GGADU_SMS_IDEA_HOST))
	return ERR_SERVICE;

    strcpy(HTTP.method, "GET");
    strcpy(HTTP.host, GGADU_SMS_IDEA_HOST);
    strcpy(HTTP.url, gettoken);
    strcpy(HTTP.url_params, " ");
    HTTP_io(HTTP);

    i = 0;
    recv_buff = g_malloc0(GGADU_SMS_RECVBUFF_LEN);

    while (recv(sock_s, temp, 1, MSG_WAITALL) && i < GGADU_SMS_RECVBUFF_LEN)
	recv_buff[i++] = temp[0];

    sms_disconnect();

    print_debug("\n============retries left: %d=================\nIDEA RECVBUFF2: %s\n\n", retries, recv_buff);

    if (!g_strstr_len(recv_buff, i, "200 OK"))
    {
	g_free(recv_buff);
	if (--retries > 0)
	    goto get_token;
	else
	{
	    g_free(gettoken);
	    g_free(token);
	    return ERR_GATEWAY;
	}
    } else
	g_free(gettoken);

    for (j = 0; j < i; j++)
    {
	if (recv_buff[j] == '\r' && recv_buff[j + 1] == '\n' && recv_buff[j + 2] == '\r' && recv_buff[j + 3] == '\n')
	    break;
    }
    j += 4;

    if (j >= i)
    {
	g_free(token);
	g_free(recv_buff);
	return ERR_READ_TOKEN;
    }

    for (k = 0; k < i - j; k++)
	recv_buff[k] = recv_buff[k + j];
    recv_buff[k] = 0;

    /* oops, bail out if IDEA_GFX cannot be written. */
    if (!(idea_logo = fopen(IDEA_GFX, "w")))
    {
	g_free(token);
	g_free(recv_buff);
	return ERR_WRITE_TOKEN;
    }

    /* write token image to file, close fd */
    fwrite(recv_buff, 1, i - j, idea_logo);
    fclose(idea_logo);

    /* *INDENT-OFF* */
    /* no need to urlencode number, it's nothing but 9 digits */
    g_free(recv_buff);
    recv_buff =	g_strconcat("token=", token,
			"&SENDER=", ggadu_sms_urlencode(g_strdup(sms_sender)),
			"&RECIPIENT=", sms_number,
			"&SHORT_MESSAGE=", ggadu_sms_urlencode(g_strdup(sms_body)), NULL);
    /* *INDENT-ON* */

    IDEA_logo(recv_buff);

    return TRUE;
}

int send_IDEA_stage2(gchar * pass, gpointer user_data)
{
    gchar *recv_buff = NULL;
    gchar *post = NULL;
    gchar *sms_number = NULL;
    gchar *p = NULL;
    gchar temp[2];
    gint i, retries = 3;

    p = g_strstr_len(user_data, strlen(user_data), "&RECIPIENT=");
    sms_number = g_strndup(p + 11, 9);

    if (!pass)
    {
	sms_warning(sms_number, _("Please enter token"));
	g_free(sms_number);
	return FALSE;
    }

    post = g_strconcat((gchar *) user_data, "&pass=", pass, "&respInfo=2", NULL);

    /* is there any better place for this? */
    unlink(IDEA_GFX);

send_sms:
    if (!sms_connect("IDEA", "213.218.116.131"))
    {
	sms_warning(sms_number, _("Cannot connect!"));
	g_free(post);
	g_free(sms_number);
	return FALSE;
    }

    strcpy(HTTP.method, "POST");
    strcpy(HTTP.host, GGADU_SMS_IDEA_HOST);
    strcpy(HTTP.url, GGADU_SMS_IDEA_URL_SEND);
    strcpy(HTTP.url_params, " ");
    strcpy(HTTP.post_data, post);
    HTTP.post_length = strlen(post);
    HTTP_io(HTTP);

    i = 0;
    recv_buff = g_malloc0(GGADU_SMS_RECVBUFF_LEN);
    while (recv(sock_s, temp, 1, MSG_WAITALL) && i < GGADU_SMS_RECVBUFF_LEN)
	recv_buff[i++] = temp[0];

    sms_disconnect();

    print_debug("\n============retries left: %d===================\nIDEA RECVBUFF3: %s\n\n", retries, recv_buff);

    if (!g_strstr_len(recv_buff, i, "200 OK"))
    {
	g_free(recv_buff);
	if (--retries > 0)
	    goto send_sms;
	else
	{
	    g_free(post);
	    g_free(sms_number);
	    return ERR_GATEWAY;
	}
    } else
	g_free(post);

    if (g_strstr_len(recv_buff, i, "SMS zosta³ wys³any"))
	sms_message(sms_number, _("SMS has been sent"));

    else if (g_strstr_len(recv_buff, i, "Podano b³êdne has³o, SMS nie zosta³ wys³any"))
	sms_warning(sms_number, _("Bad token!"));

    else if (g_strstr_len(recv_buff, i, "Object moved"))
	sms_warning(sms_number, _("Bad token entered!"));

    else if (g_strstr_len(recv_buff, i, "wyczerpany"))
	sms_warning(sms_number, _("Daily limit exceeded!"));

    else if (g_strstr_len(recv_buff, i, "serwis chwilowo"))
	sms_warning(sms_number, _("Gateway error!"));

    else if (g_strstr_len(recv_buff, i, "Odbiorca nie ma aktywnej"))
	sms_warning(sms_number, _("Service not activated!"));

    else if (g_strstr_len(recv_buff, i, "adres odbiorcy wiadomosci jest nieprawid"))
	sms_warning(sms_number, _("Invalid number"));

    g_free(sms_number);
    g_free(recv_buff);
    return FALSE;
}

/* wyslanie na plusa */
int send_PLUS(const gchar * sms_sender, const gchar * sms_number, const gchar * sms_body)
{
    gchar *recv_buff = NULL;
    gchar *post = NULL;
    gchar tprefix[4];
    gchar temp[2];
    int i = 0, ret = ERR_UNKNOWN;

    if (!sms_connect("PLUS", GGADU_SMS_PLUS_HOST))
	return ERR_SERVICE;

    strncpy(tprefix, sms_number, 3);
    tprefix[3] = 0;

    /* *INDENT-OFF* */
    post = g_strconcat("tprefix=", tprefix, "&numer=", (sms_number + 3),
		"&odkogo=", ggadu_sms_urlencode(g_strdup(sms_sender)),
		"&tekst=", ggadu_sms_urlencode(g_strdup(sms_body)), NULL);
    /* *INDENT-ON* */

    strcpy(HTTP.method, "POST");
    strcpy(HTTP.host, GGADU_SMS_PLUS_HOST);
    strcpy(HTTP.url, GGADU_SMS_PLUS_URL);
    strcpy(HTTP.url_params, " ");
    strcpy(HTTP.post_data, post);
    HTTP.post_length = strlen(post);
    HTTP_io(HTTP);
    g_free(post);

    recv_buff = g_malloc0(GGADU_SMS_RECVBUFF_LEN);
    while (recv(sock_s, temp, 1, MSG_WAITALL) && i < GGADU_SMS_RECVBUFF_LEN)
	recv_buff[i++] = temp[0];

    sms_disconnect();

    if (!strlen(recv_buff))
	ret = ERR_SERVICE;
    else if (g_strstr_len(recv_buff, i, "wiadomo¶æ zosta³a wys³ana na numer"))
	ret = TRUE;
    else if (g_strstr_len(recv_buff, i, "podano z³y numer"))
	ret = ERR_BAD_RCPT;
    else if (g_strstr_len(recv_buff, i, "Z powodu przekroczenia limitów bramki"))
	ret = ERR_LIMIT_EX;

    g_free(recv_buff);
    return ret;
}

/* wyslanie na ere */
int send_ERA(const gchar * sms_sender, const gchar * sms_number, const gchar * sms_body, const gchar * era_login,
	     const gchar * era_password, int *era_left)
{
    gchar *recv_buff = NULL;
    gchar *returncode = NULL;
    gchar *get = NULL;
    gchar temp[2];
    int i = 0;
    int ret = ERR_UNKNOWN;

    if (!sms_connect("ERA", GGADU_SMS_ERA_HOST))
	return ERR_SERVICE;

    /* *INDENT-OFF* */
    get = g_strconcat ("?login=", ggadu_sms_urlencode(g_strdup(era_login)),
		"&password=", ggadu_sms_urlencode(g_strdup(era_password)),
		"&message=", ggadu_sms_urlencode(g_strdup(sms_body)),
		"&number=48", sms_number,
		"&contact=", "&signature=", ggadu_sms_urlencode(g_strdup(sms_sender)),
		"&success=OK", "&failure=FAIL", 
		"&minute=", "&hour= ", NULL);
    /* *INDENT-ON* */

    strcpy(HTTP.method, "GET");
    strcpy(HTTP.host, GGADU_SMS_ERA_HOST);
    strcpy(HTTP.url, GGADU_SMS_ERA_URL);
    strcpy(HTTP.url_params, get);
    HTTP_io(HTTP);
    g_free(get);

    recv_buff = g_malloc0(GGADU_SMS_RECVBUFF_LEN);
    while (recv(sock_s, temp, 1, MSG_WAITALL) && i < GGADU_SMS_RECVBUFF_LEN)
	recv_buff[i++] = temp[0];

    sms_disconnect();

    if (!strlen(recv_buff))
    {
	g_free(recv_buff);
	return ERR_SERVICE;
    }

    if ((returncode = g_strstr_len(recv_buff, i, "OK?X-ERA-counter=")) != NULL)
    {
	*era_left = (int) (*(returncode + 17) - '0');
	ret = TRUE;
    }

    if ((returncode = g_strstr_len(recv_buff, i, "FAIL?X-ERA-error=")) != NULL)
    {
	int r = (int) (*(returncode + 17) - '0');

	/* error codes from www.eraomnix.pl/service/gw/bAPIPrv.jsp */
	if (r == 0)
	    ret = ERR_NONE;
	else if (r == 1)
	    ret = ERR_GATEWAY;
	else if (r == 2)
	    ret = ERR_UNAUTH;
	else if (r == 3)
	    ret = ERR_ACCESS_DENIED;
	else if (r == 5)
	    ret = ERR_SYNTAX;
	else if (r == 7)
	    ret = ERR_LIMIT_EX;
	else if (r == 8)
	    ret = ERR_BAD_RCPT;
	else if (r == 9)
	    ret = ERR_MSG_TOO_LONG;
	else
	    ret = ERR_UNKNOWN;
    }

    g_free(recv_buff);
    return ret;
}

/* sprawdzenie jaka siec */
int check_operator(const gchar * sms_number)
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

/* wywolanie z sms_gui.c , tutaj wybiera co zrobic */
void send_sms(gboolean external, const gchar * sms_sender, const gchar * sms_number, const gchar * sms_body,
	      const gchar * era_login, const gchar * era_password)
{
    gint result, gsm_oper;
    int era_left = 10;
    gchar *sms_body_iso = NULL;

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

    /* Cut-off prefixes. 'just-in-case' */
    if (g_str_has_prefix(sms_number, "+48"))
	sms_number += 3;

    if (g_str_has_prefix(sms_number, "0"))
	sms_number++;

    from_utf8("ISO-8859-2", sms_body, sms_body_iso);

    gsm_oper = check_operator(sms_number);
    switch (gsm_oper)
    {
    case SMS_IDEA:
	if (external)
	{
	    result = system(g_strconcat("sms ", sms_number, " \"", sms_body_iso, " ", sms_sender, "\"", NULL));
	    g_free(sms_body_iso);
	    return;
	}
	else
	    result = send_IDEA(sms_sender, sms_number, sms_body_iso);

	break;

    case SMS_PLUS:
	if (external)
	{
	    result = system(g_strconcat("sms ", sms_number, " \"", sms_body_iso, " ", sms_sender, "\"", NULL));
	    g_free(sms_body_iso);
	    return;
	}
	else
	    result = send_PLUS(sms_sender, sms_number, sms_body_iso);

	break;

    case SMS_ERA:
	if (external)
	{
	    result = system(g_strconcat("sms ", sms_number, " \"", sms_body_iso, " ", sms_sender, "\"", NULL));
	    g_free(sms_body_iso);
	    return;
	}
	else
	{
	    if (!era_login)
	    {
		sms_warning(sms_number, _("Empty Era login!"));
		g_free(sms_body_iso);
		return;
	    }

	    if (!era_password)
	    {
		sms_warning(sms_number, _("Empty Era password!"));
		g_free(sms_body_iso);
		return;
	    }
	    result = send_ERA(sms_sender, sms_number, sms_body_iso, era_login, era_password, &era_left);
	}

	break;

    case FALSE:
	sms_warning(sms_number, _("Invalid number!"));
	g_free(sms_body_iso);
	return;
    }


    /* handle results */
    switch (result)
    {
	/* successes */
    case TRUE:
	/* dirty IDEA workaround: we can't handle send_idea*() return values here,
	   send_idea*() eventually notifies about success itself. */
	if (gsm_oper == SMS_PLUS)
	    sms_message(sms_number, _("SMS has been sent"));
	else if (gsm_oper == SMS_ERA)
	    sms_message(sms_number, g_strdup_printf(_("SMS has been sent. Left: %d"), era_left));
	break;

	/* failures */
    case FALSE:
	sms_warning(sms_number, _("SMS not delivered!"));
	break;
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
	sms_warning(sms_number, _("Error while reading token!"));
	break;
    case ERR_WRITE_TOKEN:
	sms_warning(sms_number, _("Cannot write token image to file!"));
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

	/* unknowns */
    case ERR_UNKNOWN:
	sms_warning(sms_number, _("Unknown error!"));
	break;
    case ERR_NONE:
	print_debug("SMS: Strange, we got ERR_NONE for %s. Shouldn't happen.\n", sms_number);
	break;
    }

    g_free(sms_body_iso);

    return;
}
