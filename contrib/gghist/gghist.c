/*
   Author  : Kazik <kazik.cz@interia.pl> 
	     GNU Gadu Team (c) 2004
   License : GPL 
*/


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <gtk/gtk.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <ggadu_support.h>
#include "gg_history.h"


GtkWidget *text_view;
int lines = 0;
int display_pages = 20;
int pages = 0;
int page = 0;

gint close_window(GtkWindow * widget, gpointer gdata)
{
	gtk_main_quit();
	return (FALSE);
}

void clear_lines()
{
	GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

	gtk_text_buffer_set_text(buf, "", 0);
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(text_view), buf);
}

static void scroll_to_end()
{
	GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	GtkTextIter iter;
	gtk_text_buffer_get_end_iter(buf, &iter);
	gtk_text_buffer_place_cursor(buf, &iter);
	gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(text_view), gtk_text_buffer_get_insert(buf), 0.0, TRUE,
					     0.0, 0.0);
}


void first_page(GtkWidget * widget, gpointer gdata)
{
	if (lines > 20)
	{
		page = 0;
#ifdef GGADU_DEBUG
		g_print("Going to first page..\n");
#endif
		clear_lines();
		show_lines(page, page + 20, (int *) gdata);
	}
}

void last_page(GtkWidget * widget, gpointer gdata)
{
	if (lines > 20)
	{
		page = lines - 20;
#ifdef GGADU_DEBUG
		g_print("Going to last page..\n");
#endif
		clear_lines();
		show_lines(page, page + 20, (int *) gdata);
	}
}

void next_page(GtkWidget * widget, gpointer gdata)
{
	if (lines > 20)
	{
		if (page < lines - 20)
			page += 20;
		else
			page = lines - 20;
#ifdef GGADU_DEBUG
		g_print("Going to next page..%d\n", page);
#endif
		clear_lines();
		show_lines(page, page + 20, (int *) gdata);
	}
}

void prev_page(GtkWidget * widget, gpointer gdata)
{
	if (lines > 20)
	{
		if (page > 20)
			page -= 20;
		else
			page = 0;
#ifdef GGADU_DEBUG
		g_print("Going to prev page..\n");
#endif
		clear_lines();
		show_lines(page, page + 20, (int *) gdata);
	}
}


GtkWidget *PutNewButton(GtkWidget * field, char *Label)
{
	GtkWidget *button = gtk_button_new_with_mnemonic(Label);
	gtk_box_pack_start(GTK_BOX(field), button, FALSE, FALSE, 5);
	gtk_widget_show(button);

	return (button);
}


void AddText(GtkWidget * text_view, const gchar * text, gchar * type)
{
	GtkTextIter iter;
	GtkTextBuffer *buf;

	buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	gtk_text_buffer_get_end_iter(buf, &iter);
	gtk_text_buffer_insert_with_tags_by_name(buf, &iter, text, -1, type, NULL);
}


GtkWidget *PutNewHField(GtkWidget * field)
{
	GtkWidget *field2 = gtk_hbox_new(FALSE, 0);

	gtk_box_pack_start(GTK_BOX(field), field2, FALSE, FALSE, 3);
	gtk_widget_show(field2);

	return (field2);
}


void show_lines(int start, int end, int *list)
{
	int i;

	gchar *gtmp1 = NULL, *gtmp2 = NULL, *gtmp3 = NULL, *gtmp4 = NULL;

	struct gg_hist_line *hist_line = NULL;

	if (end > lines)
		end = lines;

	for (i = start; i < end; i++)
	{
#ifdef GGADU_DEBUG
		g_print("DEBUG: Processing line %d..", i);
#endif
		hist_line = formatline(list[lines + 2], list[i]);

		if (hist_line->type == GG_HIST_SEND)
		{
#ifdef GGADU_DEBUG
			g_print("chatsend..");
#endif
			gtmp1 = gg_hist_time(atoi(hist_line->timestamp1),TRUE);
			gtmp2 = g_strdup_printf("%s :: %s :: \n", "Me", gtmp1);
			AddText(text_view, gtmp2, "header");
			g_free(gtmp1);
			g_free(gtmp2);

			gtmp2 = g_strcompress(hist_line->data);
			gtmp3 = g_strdup_printf("%s\n", gtmp2);
			AddText(text_view, gtmp3, "");
			g_free(gtmp2);
			g_free(gtmp3);
		}
		else if (hist_line->type == GG_HIST_RCV)
		{
#ifdef GGADU_DEBUG
			g_print("chatrcv..");
#endif
			gtmp2 = gg_hist_time(atoi(hist_line->timestamp1),FALSE);
			gtmp3 = gg_hist_time(atoi(hist_line->timestamp2),TRUE);
			gtmp4 = g_strdup_printf("%s :: %s [%s] :: \n", hist_line->nick, gtmp2, gtmp3);
			AddText(text_view, gtmp4, "header2");
			g_free(gtmp2);
			g_free(gtmp3);
			g_free(gtmp4);

			gtmp2 = g_strcompress(hist_line->data);
			gtmp3 = g_strdup_printf("%s\n", gtmp2);
			AddText(text_view, gtmp3, "");
			g_free(gtmp2);
			g_free(gtmp3);
		}
		else if (hist_line->type == GG_HIST_STATUS)
		{
#ifdef GGADU_DEBUG
			g_print("status..");
#endif
#ifdef SHOW_STATUS
			gtmp1 = gg_hist_time(atoi(hist_line->timestamp1),TRUE);
			gtmp3 = g_strdup_printf("%s *** %s *** \n", gtmp1, hist_line->data);
			AddText(text_view, gtmp3, "status");
			g_free(gtmp1);
			g_free(gtmp3);
#endif
		}
		else
		{
#ifdef GGADU_DEBUG
			g_print("unknown!..");
#endif

		}
	        free(hist_line->id);
		free(hist_line->nick);
		free(hist_line->timestamp1);
		free(hist_line->timestamp2);
		free(hist_line->ip);
		free(hist_line->data);
		free(hist_line);

#ifdef GGADU_DEBUG
		g_print("\n");
#endif
	}
	scroll_to_end();
}



int main(int argc, char **argv)
{
	GtkWidget *window;
	GtkWidget *button;
	GtkWidget *field;
	GtkWidget *field_s;
	GtkWidget *base_field;
	GtkTextBuffer *buf;

	gtk_init(&argc, &argv);

	g_print("GNU Gadu History Prototype Reader version 0.07\ne-mail: kazik.cz@interia.pl, GG: 3781462\n");

	if (argc != 2)
	{
		g_error("\nPlease enter path to the history file, for example:\n" "%s /home/coth/.gg2/history/1234567\n%s /root/.gg2/history/7654321\n", argv[0], argv[0]);
	}

#ifdef GGADU_DEBUG
	g_print("Trying to open a file..\n");
#endif
	int fd = open(argv[1], O_RDONLY);

	if (fd == -1) {
		GtkWidget *dialog;
		
		
		dialog = gtk_message_dialog_new (NULL,GTK_DIALOG_MODAL,
						GTK_MESSAGE_WARNING,
						GTK_BUTTONS_CLOSE,
						_("There is no history file"));
						
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		gtk_exit(0);
	}
	

#ifdef GGADU_DEBUG
	g_print("Getting lines count..\n");
#endif
	lines = lines_count(fd);
	int list[lines + 2];


	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "GNU Gadu History Prototype");
	gtk_signal_connect(GTK_OBJECT(window), "delete_event", GTK_SIGNAL_FUNC(close_window), NULL);
	gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);
	gtk_container_border_width(GTK_CONTAINER(window), 1);

	/* Base container */
	base_field = gtk_vbox_new(FALSE, 0);

	/* Top side - textbox with vertical scrollbar */
	text_view = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_CHAR);
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text_view), 2);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), FALSE);

	buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	gtk_text_buffer_create_tag(buf, "header", "foreground", "brown", "font", "Arial Bold", NULL);
	gtk_text_buffer_create_tag(buf, "header2", "foreground", "blue", "font", "Arial Bold", NULL);
	gtk_text_buffer_create_tag(buf, "status", "foreground", "#995500", "font", "Arial Bold", NULL);
	gtk_text_buffer_create_tag(buf, "warning", "foreground", "#990099", "font", "Arial Bold", NULL);
	gtk_text_buffer_create_tag(buf, "", "foreground", "#000000", "font", "Arial", NULL);

	field_s = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(field_s), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_container_add(GTK_CONTAINER(field_s), text_view);

	gtk_box_pack_start(GTK_BOX(base_field), field_s, TRUE, TRUE, 3);
	gtk_widget_show_all(field_s);


	/* Bottom side - 4 buttons */
	field = PutNewHField(base_field);
	/* button = PutNewButton(field, _("Find")); */

	button = PutNewButton(field, _("_First page"));
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(first_page), list);

	button = PutNewButton(field, _("_Previous page"));
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(prev_page), list);

	button = PutNewButton(field, _("_Next page"));
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(next_page), list);

	button = PutNewButton(field, _("_Last page"));
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(last_page), list);

	button = PutNewButton(field, _("_Close"));
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(close_window), NULL);

	/* Add 'Base container' to window */
	gtk_container_add(GTK_CONTAINER(window), base_field);


	list[lines + 2] = fd;
#ifdef GGADU_DEBUG
	g_print("Reading offsets of lines..%d\n", lines);
	g_print("Found %d lines, and read %d of 'em\n", lines, get_lines(fd, list));
#endif

	/* last page ? */
	if (lines > display_pages)
	    show_lines(lines-display_pages, lines, list);
	else
	    show_lines(0, display_pages, list);

	gtk_widget_show_all(window);

	gtk_main();
	close(fd);

#ifdef GGADU_DEBUG
	g_print("\nClosing...\n");
#endif
	return 0;
}
