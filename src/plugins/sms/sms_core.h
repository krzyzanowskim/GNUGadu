/* $Id: sms_core.h,v 1.20 2005/03/27 00:16:07 shaster Exp $ */

/* 
 * SMS plugin for GNU Gadu 2 
 * 
 * Copyright (C) 2003 Bartlomiej Pawlak 
 * Copyright (C) 2003-2005 GNU Gadu Team 
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

#ifndef SMS_CORE_PLUGIN_H
#define SMS_CORE_PLUGIN_H 1

#define GGADU_SMS_PLUS_HOST 		"www.text.plusgsm.pl"
#define GGADU_SMS_PLUS_URL 		"/sms/sendsms.php"

#define GGADU_SMS_IDEA_HOST 		"sms.idea.pl"
#define GGADU_SMS_IDEA_URL_GET 		"/"
#define GGADU_SMS_IDEA_URL_SEND		"/sendsms.aspx"

#define GGADU_SMS_ERA_HOST 	"www.eraomnix.pl"
#define GGADU_SMS_ERA_URL 	"/sms/do/extern/tinker/free/send"

/* error codes from www.eraomnix.pl/service/gw/bAPIPrv.jsp */
#define GGADU_SMS_ERA_ERR_NONE 0
#define GGADU_SMS_ERA_ERR_GATEWAY 1
#define GGADU_SMS_ERA_ERR_UNAUTH 2
#define GGADU_SMS_ERA_ERR_ACCESS_DENIED 3
#define GGADU_SMS_ERA_ERR_SYNTAX 5
#define GGADU_SMS_ERA_ERR_LIMIT_EX 7
#define GGADU_SMS_ERA_ERR_BAD_RCPT 8
#define GGADU_SMS_ERA_ERR_MSG_TOO_LONG 9


#define USER_AGENT          "User-Agent: Mozilla/5.0 (X11; U; Linux i686) Gecko/20030313 Galeon/1.3.4\r\n"
#define ACCEPT              "Accept: text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,video/x-mng,image/png,image/jpeg,image/gif;q=0.2,*/*;q=0.1\r\n"
#define ACCEPT_LANG         "Accept-Language: pl\r\n"
#define ACCEPT_ENCODING     "Accept-Encoding: gzip, deflate, compress;q=0.9\r\n"
#define ACCEPT_CHARSET      "Accept-Charset: ISO-8859-2,utf-8;q=0.7,*;q=0.7\r\n"
#define POST_CONTENT_TYPE   "Content-Type: application/x-www-form-urlencoded\r\n"
#define CONTENT_LENGTH      "Content-Length: "

#define GGADU_SMS_RECVBUFF_LEN 32768

#define IDEA_GFX "/idea_token.gfx"
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
	SMS_METHOD_GET,
	SMS_METHOD_POST
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

typedef struct
{
	gint method;
	gchar *host;
	gchar *url;
	gchar *url_params;
	gchar *post_data;
	gint post_length;
} HTTPstruct;

typedef struct
{
	gboolean external;
	gchar *number;		/* free() me */
	gchar *body;		/* free() me */
	gchar *sender;		/* free() me */
	gchar *era_login;
	gchar *era_password;
	gchar *idea_token;	/* free() me */
	gchar *idea_pass;	/* free() me */
} SMS;

gpointer send_sms(SMS * message);
gpointer send_IDEA_stage2(SMS * message);

void sms_message(const gchar * sms_number, const gchar * message);
void sms_warning(const gchar * sms_number, const gchar * warning);

gchar *ggadu_sms_urlencode(gchar * source);
gchar *ggadu_sms_append_char(gchar * url_string, gchar char_to_append, gboolean convert_to_hex);

#endif
