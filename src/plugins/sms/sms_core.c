/* $Id: sms_core.c,v 1.38 2004/05/29 00:35:15 shaster Exp $ */

/*
 * SMS plugin for GNU Gadu 2
 *
 * Copyright (C) 2003 Bartlomiej Pawlak
 * Copyright (C) 2003-2004 GNU Gadu Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

#include "ggadu_types.h"
#include "plugins.h"
#include "signals.h"
#include "ggadu_menu.h"
#include "ggadu_support.h"
#include "ggadu_dialog.h"
#include "sms_gui.h"
#include "sms_core.h"

extern char *idea_token_path;
extern gint method;

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

/* laczenie sie z wybranym hostem, zwraca -1 przy b³edzie, inaczej 0 */
gint sms_connect(gchar * sms_info, gchar * sms_host, int *sock_s)
{
	struct hostent *h;
	struct sockaddr_in servAddr;
	int sock_r = -1;

	if ((h = gethostbyname(sms_host)) == NULL)
	{
		print_debug("%s : Unknown host\n", sms_info);
		return sock_r;
	}

	*sock_s = socket(AF_INET, SOCK_STREAM, 0);
	if (*sock_s < 0)
	{
		print_debug("%s : Cannot open socket\n", sms_info);
		return sock_r;
	}

	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(80);
	servAddr.sin_addr = *((struct in_addr *) h->h_addr);
	bzero(&(servAddr.sin_zero), 8);
	sock_r = connect(*sock_s, (struct sockaddr *) &servAddr, sizeof (struct sockaddr));

	if (sock_r < 0)
	{
		print_debug("%s : Cannot connect\n", sms_info);
		close(*sock_s);
	}

	return sock_r;
}

/* POST/GET handling */
gint HTTP_io(HTTPstruct * hdata, int sock_s)
{
	gchar *io_buf = NULL;

	if (hdata == NULL)
		return FALSE;

	/* *INDENT-OFF* */
	if (!strcmp(hdata->method, "POST"))
		io_buf = g_strdup_printf("POST %s%sHTTP/1.0\r\nHost: %s\r\n%s%s%s%s%s%s%s%d\r\n\r\n%s",
				hdata->url, hdata->url_params, hdata->host, USER_AGENT,
				ACCEPT, ACCEPT_LANG, ACCEPT_ENCODING, ACCEPT_CHARSET,
				POST_CONTENT_TYPE, CONTENT_LENGTH, hdata->post_length, hdata->post_data);
	else if (!strcmp(hdata->method, "GET"))
		io_buf = g_strdup_printf("GET %s%sHTTP/1.0\r\nHost: %s\r\n%s\r\n\r\n",
				hdata->url, hdata->url_params, hdata->host, USER_AGENT);
	else
		io_buf = g_strdup("GET /c-programming-howto.html HTTP/1.0\r\n\r\n");
	/* *INDENT-ON* */

	print_debug("Sending:\n%s\n", io_buf);
	send(sock_s, io_buf, strlen(io_buf), MSG_WAITALL);
	g_free(io_buf);

	return TRUE;
}

HTTPstruct *httpstruct_new(void)
{
	HTTPstruct *h = g_new0(HTTPstruct, 1);
	return h;
}

void httpstruct_free(HTTPstruct * h)
{
	if (h == NULL)
		return;

	g_free(h->method);
	g_free(h->host);
	g_free(h->url);
	g_free(h->url_params);
	g_free(h->post_data);
	g_free(h);
}

void SMS_free(SMS * message)
{
	if (!message)
		return;

	g_free(message->number);
	g_free(message->sender);
	g_free(message->body);
	g_free(message->idea_token);
	g_free(message->idea_pass);
	g_free(message);

	return;
}

/* tu bedzie wymiana na cos innego, GUI musi to obslugiwac a nie "samowolka" ;-) */
gboolean IDEA_logo(SMS * user_data)
{
	GGaduDialog *dialog = ggadu_dialog_new_full(GGADU_DIALOG_GENERIC, _("IDEA token"), "get token", user_data);
	
	ggadu_dialog_add_entry(dialog, 0, "", VAR_IMG, idea_token_path, VAR_FLAG_NONE);
	ggadu_dialog_add_entry(dialog, 1, _("Enter token text"), VAR_STR, NULL, VAR_FLAG_NONE);

	signal_emit_from_thread("sms", "gui show dialog", dialog, "main-gui");

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
			signal_emit_from_thread("sms", "gui show warning", g_strdup(message), "main-gui");
		else if (type == GGADU_SMS_TYPE_INFO)
			signal_emit_from_thread("sms", "gui show message", g_strdup(message), "main-gui");
	}

	if (method == GGADU_SMS_METHOD_CHAT)
	{
		GGaduMsg *msg = g_new0(GGaduMsg, 1);
		msg->id = g_strdup(sms_number ? sms_number : _("None"));
		msg->class = GGADU_CLASS_CHAT;
		msg->message = g_strconcat(_("SMS plugin: "), message, NULL);
		signal_emit_from_thread("sms", "gui msg receive", msg, "main-gui");
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
gint send_IDEA(SMS * message)
{
	gchar *token = NULL;
	gchar temp[2];
	gchar *gettoken = NULL;
	gchar *recv_buff = NULL;
	gchar *buf = NULL;
	gint i = 0, j, k, retries = 3;
	FILE *idea_logo;
	HTTPstruct *HTTP = NULL;
	SMS *message2 = NULL;
	int sock_s;

	HTTP = httpstruct_new();
	HTTP->method = g_strdup("GET");
	HTTP->host = g_strdup(GGADU_SMS_IDEA_HOST);
	HTTP->url = g_strdup(GGADU_SMS_IDEA_URL_GET);
	HTTP->url_params = g_strdup(" ");

      get_mainpage:
	/* pobranie adresu do obrazka */
	if (sms_connect("IDEA", "213.218.116.131", &sock_s))
	{
		httpstruct_free(HTTP);
		return ERR_SERVICE;
	}

	HTTP_io(HTTP, sock_s);

	recv_buff = g_malloc0(GGADU_SMS_RECVBUFF_LEN);

	i = 0;
	while (recv(sock_s, temp, 1, MSG_WAITALL) && i < GGADU_SMS_RECVBUFF_LEN)
		recv_buff[i++] = temp[0];

	close(sock_s);

	print_debug("\n=======retries left: %d=====\nIDEA RECVBUFF1: %s\n\n", retries - 1, recv_buff);

	if (!g_strstr_len(recv_buff, i, "200 OK"))
	{
		g_free(recv_buff);
		if (--retries > 0)
			goto get_mainpage;
		else
		{
			httpstruct_free(HTTP);
			return ERR_GATEWAY;
		}
	}
	else
		retries = 3;

	httpstruct_free(HTTP);

	if (!(buf = g_strstr_len(recv_buff, i, "rotate_token.aspx?token=")))
	{
		g_free(recv_buff);
		return ERR_READ_TOKEN;
	}

	if (!(token = g_strndup(buf + 24, GGADU_SMS_IDEA_TOKENLEN)))
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

	HTTP = httpstruct_new();
	HTTP->method = g_strdup("GET");
	HTTP->host = g_strdup(GGADU_SMS_IDEA_HOST);
	HTTP->url = g_strdup(gettoken);
	HTTP->url_params = g_strdup(" ");

      get_token:
	if (sms_connect("IDEA", GGADU_SMS_IDEA_HOST, &sock_s))
	{
		httpstruct_free(HTTP);
		return ERR_SERVICE;
	}

	HTTP_io(HTTP, sock_s);

	i = 0;
	recv_buff = g_malloc0(GGADU_SMS_RECVBUFF_LEN);

	while (recv(sock_s, temp, 1, MSG_WAITALL) && i < GGADU_SMS_RECVBUFF_LEN)
		recv_buff[i++] = temp[0];

	close(sock_s);

	print_debug("\n============retries left: %d=================\nIDEA RECVBUFF2: %s\n\n", retries, recv_buff);

	if (!g_strstr_len(recv_buff, i, "200 OK"))
	{
		g_free(recv_buff);
		if (--retries > 0)
			goto get_token;
		else
		{
			httpstruct_free(HTTP);
			g_free(gettoken);
			g_free(token);
			return ERR_GATEWAY;
		}
	}
	else
	{
		httpstruct_free(HTTP);
		g_free(gettoken);
	}

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

	/* oops, bail out if idea_token_path cannot be written. */
	if (!(idea_logo = fopen(idea_token_path, "w")))
	{
		g_free(token);
		g_free(recv_buff);
		return ERR_WRITE_TOKEN;
	}

	/* write token image to file, close fd */
	fwrite(recv_buff, 1, i - j, idea_logo);
	fclose(idea_logo);

	g_free(recv_buff);

	/* allocate another message, because this one will be free()d outside this function */
	message2 = (SMS *) g_new0(SMS, 1);
	message2->number = g_strdup(message->number);
	message2->sender = g_strdup(message->sender);
	message2->body = g_strdup(message->body);

	message2->idea_token = token;
	message2->idea_pass = NULL;

	IDEA_logo(message2);

	return TRUE;
}

gpointer send_IDEA_stage2(SMS * message)
{
	gchar *recv_buff = NULL;
	gchar *post = NULL;
	gchar *sms_number = message->number;
	gchar temp[2];
	gchar *sender = NULL;
	gchar *body = NULL;
	gint i, retries = 3;
	HTTPstruct *HTTP = NULL;
	int sock_s;

	/* is there any better place for this? */
	unlink(idea_token_path);

	if (!message)
	{
		print_debug("Oops! message==NULL!\n");
		goto out;
	}

	if (!message->idea_pass)
	{
		sms_warning(message->number, _("Please enter token"));
		goto out;
	}
	
	/* Cut-off prefixes. 'just-in-case' */
	if (g_str_has_prefix(message->number, "+"))
		sms_number++;

	if (g_str_has_prefix(message->number, "48"))
		sms_number += 2;

	if (g_str_has_prefix(message->number, "0"))
		sms_number++;

	sender = ggadu_sms_urlencode(g_strdup(message->sender));
	body = ggadu_sms_urlencode(g_strdup(message->body));
	post = g_strconcat("token=", message->idea_token, "&SENDER=", sender, "&RECIPIENT=", sms_number, "&SHORT_MESSAGE=", body, "&pass=",
			   message->idea_pass, "&respInfo=2", NULL);

	g_free(sender);
	g_free(body);

	print_debug("Post data: %s\n", post);

	HTTP = httpstruct_new();
	HTTP->method = g_strdup("POST");
	HTTP->host = g_strdup(GGADU_SMS_IDEA_HOST);
	HTTP->url = g_strdup(GGADU_SMS_IDEA_URL_SEND);
	HTTP->url_params = g_strdup(" ");
	HTTP->post_data = g_strdup(post);
	HTTP->post_length = strlen(post);
	g_free(post);

      send_sms:
	if (sms_connect("IDEA", "213.218.116.131", &sock_s))
	{
		sms_warning(message->number, _("Cannot connect!"));
		httpstruct_free(HTTP);
		goto out;
	}

	HTTP_io(HTTP, sock_s);

	i = 0;
	recv_buff = g_malloc0(GGADU_SMS_RECVBUFF_LEN);
	while (recv(sock_s, temp, 1, MSG_WAITALL) && i < GGADU_SMS_RECVBUFF_LEN)
		recv_buff[i++] = temp[0];

	close(sock_s);

	print_debug("\n============retries left: %d===================\nIDEA RECVBUFF3: %s\n\n", retries, recv_buff);

	if (!g_strstr_len(recv_buff, i, "200 OK"))
	{
		g_free(recv_buff);
		if (--retries > 0)
			goto send_sms;
		else
		{
			httpstruct_free(HTTP);
			goto out;
		}
	}
	else
	{
		httpstruct_free(HTTP);
	}

	if (g_strstr_len(recv_buff, i, "SMS zosta³ wys³any"))
		sms_message(message->number, _("SMS has been sent"));

	else if (g_strstr_len(recv_buff, i, "Podano b³êdne has³o, SMS nie zosta³ wys³any"))
		sms_warning(message->number, _("Bad token!"));

	else if (g_strstr_len(recv_buff, i, "Object moved"))
		sms_warning(message->number, _("Bad token entered!"));

	else if (g_strstr_len(recv_buff, i, "wyczerpany"))
		sms_warning(message->number, _("Daily limit exceeded!"));

	else if (g_strstr_len(recv_buff, i, "serwis chwilowo"))
		sms_warning(message->number, _("Gateway error!"));

	else if (g_strstr_len(recv_buff, i, "Odbiorca nie ma aktywnej"))
		sms_warning(message->number, _("Service not activated!"));

	else if (g_strstr_len(recv_buff, i, "adres odbiorcy wiadomosci jest nieprawid"))
		sms_warning(message->number, _("Invalid number"));

	g_free(recv_buff);

      out:
	SMS_free(message);

	g_thread_exit(NULL);
	return NULL;
}

/* wyslanie na plusa */
gint send_PLUS(SMS * message)
{
	gchar *recv_buff = NULL;
	gchar *post = NULL;
	gchar tprefix[4];
	gchar temp[2];
	gchar *sms_number = message->number;
	gchar *sender = NULL;
	gchar *body = NULL;
	gint i = 0, ret = ERR_UNKNOWN;
	HTTPstruct *HTTP = NULL;
	int sock_s;

	if (sms_connect("PLUS", GGADU_SMS_PLUS_HOST, &sock_s))
	{
		ret = ERR_SERVICE;
		goto out;
	}

	/* Cut-off prefixes. 'just-in-case' */
	if (g_str_has_prefix(message->number, "+"))
		sms_number++;

	if (g_str_has_prefix(message->number, "48"))
		sms_number += 2;

	if (g_str_has_prefix(message->number, "0"))
		sms_number++;

	strncpy(tprefix, sms_number, 3);
	tprefix[3] = 0;

	sender = ggadu_sms_urlencode(g_strdup(message->sender));
	body = ggadu_sms_urlencode(g_strdup(message->body));

	/* *INDENT-OFF* */
	post = g_strconcat("tprefix=", tprefix, "&numer=", (sms_number + 3),
			   "&odkogo=", sender, "&tekst=", body, NULL);
	/* *INDENT-ON* */

	g_free(sender);
	g_free(body);

	HTTP = httpstruct_new();
	HTTP->method = g_strdup("POST");
	HTTP->host = g_strdup(GGADU_SMS_PLUS_HOST);
	HTTP->url = g_strdup(GGADU_SMS_PLUS_URL);
	HTTP->url_params = g_strdup(" ");
	HTTP->post_data = g_strdup(post);
	HTTP->post_length = strlen(post);
	HTTP_io(HTTP, sock_s);
	httpstruct_free(HTTP);
	g_free(post);

	recv_buff = g_malloc0(GGADU_SMS_RECVBUFF_LEN);
	while (recv(sock_s, temp, 1, MSG_WAITALL) && i < GGADU_SMS_RECVBUFF_LEN)
		recv_buff[i++] = temp[0];

	close(sock_s);

	if (!strlen(recv_buff))
		ret = ERR_SERVICE;
	else if (g_strstr_len(recv_buff, i, "wiadomo¶æ zosta³a wys³ana na numer"))
		ret = TRUE;
	else if (g_strstr_len(recv_buff, i, "podano z³y numer"))
		ret = ERR_BAD_RCPT;
	else if (g_strstr_len(recv_buff, i, "Z powodu przekroczenia limitów bramki"))
		ret = ERR_LIMIT_EX;

	g_free(recv_buff);

      out:
	return ret;
}

/* wyslanie na ere */
gint send_ERA(SMS * message, int *era_left)
{
	gchar *recv_buff = NULL;
	gchar *returncode = NULL;
	gchar *get = NULL;
	gchar temp[2];
	gchar *sms_number = message->number;
	gchar *sender = NULL;
	gchar *body = NULL;
	gchar *era_login = NULL;
	gchar *era_password = NULL;
	gint i = 0;
	gint ret = ERR_UNKNOWN;
	HTTPstruct *HTTP;
	int sock_s;

	if (sms_connect("ERA", GGADU_SMS_ERA_HOST, &sock_s))
	{
		ret = ERR_SERVICE;
		goto out;
	}

	/* Cut-off prefixes. 'just-in-case' */
	if (g_str_has_prefix(message->number, "+"))
		sms_number++;

	if (g_str_has_prefix(message->number, "48"))
		sms_number += 2;

	if (g_str_has_prefix(message->number, "0"))
		sms_number++;

	sender = ggadu_sms_urlencode(g_strdup(message->sender));
	body = ggadu_sms_urlencode(g_strdup(message->body));
	era_login = ggadu_sms_urlencode(g_strdup(message->era_login));
	era_password = ggadu_sms_urlencode(g_strdup(message->era_password));

	/* *INDENT-OFF* */
	get = g_strconcat ("?login=", era_login, "&password=", era_password,
			"&message=", body, "&number=48", sms_number,
			"&contact=", "&signature=", sender,
			"&success=OK", "&failure=FAIL", "&minute=", "&hour= ", NULL);
	/* *INDENT-ON* */

	g_free(sender);
	g_free(body);
	g_free(era_login);
	g_free(era_password);

	HTTP = httpstruct_new();
	HTTP->method = g_strdup("GET");
	HTTP->host = g_strdup(GGADU_SMS_ERA_HOST);
	HTTP->url = g_strdup(GGADU_SMS_ERA_URL);
	HTTP->url_params = g_strdup(get);
	HTTP_io(HTTP, sock_s);
	httpstruct_free(HTTP);
	g_free(get);

	recv_buff = g_malloc0(GGADU_SMS_RECVBUFF_LEN);
	while (recv(sock_s, temp, 1, MSG_WAITALL) && i < GGADU_SMS_RECVBUFF_LEN)
		recv_buff[i++] = temp[0];

	close(sock_s);

	if (!strlen(recv_buff))
	{
		ret = ERR_SERVICE;
		goto out;
	}

	if ((returncode = g_strstr_len(recv_buff, i, "OK?X-ERA-counter=")) != NULL)
	{
		*era_left = (int) (*(returncode + 17) - '0');
		ret = TRUE;
	}
	else if ((returncode = g_strstr_len(recv_buff, i, "FAIL?X-ERA-error=")) != NULL)
	{
		gint r = (gint) (*(returncode + 17) - '0');

		/* error codes from www.eraomnix.pl/service/gw/bAPIPrv.jsp */
		if (r == GGADU_SMS_ERA_ERR_NONE)
			ret = ERR_NONE;
		else if (r == GGADU_SMS_ERA_ERR_GATEWAY)
			ret = ERR_GATEWAY;
		else if (r == GGADU_SMS_ERA_ERR_UNAUTH)
			ret = ERR_UNAUTH;
		else if (r == GGADU_SMS_ERA_ERR_ACCESS_DENIED)
			ret = ERR_ACCESS_DENIED;
		else if (r == GGADU_SMS_ERA_ERR_SYNTAX)
			ret = ERR_SYNTAX;
		else if (r == GGADU_SMS_ERA_ERR_LIMIT_EX)
			ret = ERR_LIMIT_EX;
		else if (r == GGADU_SMS_ERA_ERR_BAD_RCPT)
			ret = ERR_BAD_RCPT;
		else if (r == GGADU_SMS_ERA_ERR_MSG_TOO_LONG)
			ret = ERR_MSG_TOO_LONG;
		else
			ret = ERR_UNKNOWN;
	}

      out:
	g_free(recv_buff);
	g_free(returncode);

	return ret;
}

/* sprawdzenie jaka siec */
gint check_operator(gchar * number)
{
	gchar *sms_number = number;

	/* Cut-off prefixes. 'just-in-case' */
	if (g_str_has_prefix(sms_number, "+"))
		sms_number++;

	if (g_str_has_prefix(sms_number, "48"))
		sms_number += 2;

	if (g_str_has_prefix(sms_number, "0"))
		sms_number++;

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

	if (*sms_number == '8')
		return SMS_ERA;

	return FALSE;
}

/* wywolanie z sms_gui.c , tutaj wybiera co zrobic */
gpointer send_sms(SMS * message)
{
	gint result, gsm_oper;
	gint era_left = 10;

	if (!message)
	{
		print_debug("OOPS! Something BAD happened!\n");
		g_thread_exit(NULL);
		return NULL;
	}

	if (!message->number)
	{
		sms_warning(message->number, _("Specify recipient number!"));
		goto out;
	}

	if (!message->sender)
	{
		sms_warning(message->number, _("Specify sender name!"));
		goto out;
	}

	if (!message->body)
	{
		sms_warning(message->number, _("Message is empty!"));
		goto out;
	}

	gsm_oper = check_operator(message->number);
	switch (gsm_oper)
	{
	case SMS_IDEA:
		if (message->external)
		{
			result = system(g_strconcat("sms ", message->number, " \"", message->body, " ", message->sender, "\"", NULL));
			goto out;
		}
		else
			result = send_IDEA(message);

		break;

	case SMS_PLUS:
		if (message->external)
		{
			result = system(g_strconcat("sms ", message->number, " \"", message->body, " ", message->sender, "\"", NULL));
			goto out;
		}
		else
			result = send_PLUS(message);

		break;

	case SMS_ERA:
		if (message->external)
		{
			result = system(g_strconcat("sms ", message->number, " \"", message->body, " ", message->sender, "\"", NULL));
			goto out;
		}
		else
		{
			if (!message->era_login)
			{
				sms_warning(message->number, _("Empty Era login!"));
				goto out;
			}

			if (!message->era_password)
			{
				sms_warning(message->number, _("Empty Era password!"));
				goto out;
			}
			result = send_ERA(message, &era_left);
		}

		break;

	case FALSE:
		sms_warning(message->number, _("Invalid number!"));
		goto out;
	}


	/* handle results */
	switch (result)
	{
		/* successes */
	case TRUE:
		/* dirty IDEA workaround: we can't handle send_idea*() return values here,
		   send_idea*() eventually notifies about success itself. */
		if (gsm_oper == SMS_PLUS)
			sms_message(message->number, _("SMS has been sent"));
		else if (gsm_oper == SMS_ERA)
		{
			gchar *tmp = g_strdup_printf(_("SMS has been sent. Left: %d"), era_left);
			sms_message(message->number, tmp);
			g_free(tmp);
		}
		break;

		/* failures */
	case FALSE:
		sms_warning(message->number, _("SMS not delivered!"));
		break;
	case ERR_UNAUTH:
		sms_warning(message->number, _("Unauthorized!"));
		break;
	case ERR_ACCESS_DENIED:
		sms_warning(message->number, _("Access denied!"));
		break;
	case ERR_SYNTAX:
		sms_warning(message->number, _("Syntax error!"));
		break;
	case ERR_BAD_RCPT:
		sms_warning(message->number, _("Invalid recipient!"));
		break;
	case ERR_MSG_TOO_LONG:
		sms_warning(message->number, _("Message too long!"));
		break;
	case ERR_READ_TOKEN:
		sms_warning(message->number, _("Error while reading token!"));
		break;
	case ERR_WRITE_TOKEN:
		sms_warning(message->number, _("Cannot write token image to file!"));
		break;
	case ERR_LIMIT_EX:
		sms_warning(message->number, _("Daily limit exceeded!"));
		break;
	case ERR_GATEWAY:
		sms_warning(message->number, _("Gateway error!"));
		break;
	case ERR_SERVICE:
		sms_warning(message->number, _("Cannot connect!"));
		break;

		/* unknowns */
	case ERR_UNKNOWN:
		sms_warning(message->number, _("Unknown error!"));
		break;
	case ERR_NONE:
		print_debug("SMS: Strange, we got ERR_NONE for %s. Shouldn't happen.\n", message->number);
		break;
	}

      out:
	SMS_free(message);
	g_thread_exit(NULL);
	return NULL;
}
