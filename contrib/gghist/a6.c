#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <gtk/gtk.h>

#include "gg_history.c"


GtkWidget *text_view;
int  lines;
int  display_pages = 20,
     pages = 0,
     page = 0;

void (show_lines)(int start, int end, int *list);



gint CloseWindow(GtkWindow *widget, gpointer gdata)
{
    gtk_main_quit();
    return (FALSE);
}


void clear_lines()
{
    GtkTextIter   iter;
    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	gtk_text_buffer_set_text(buf, "", 0);
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(text_view), buf);
}

void first_page(GtkWidget *widget, gpointer gdata)
{
    if(lines>20)
    {
	page = 0;
	g_print("Going to first page..\n");
	clear_lines();
	show_lines(page, page+20, (int *)gdata);
    }
}

void last_page(GtkWidget *widget, gpointer gdata)
{
    if(lines>20)
    {
	page = lines-20;
	g_print("Going to last page..\n");
	clear_lines();
	show_lines(page, page+20, (int *)gdata);
    }
}

void next_page(GtkWidget *widget, gpointer gdata)
{
    if(lines>20)
    {
	if(page<lines-20) page+=20; else page = lines-20;
	g_print("Going to next page..\n", page);
	clear_lines();
	show_lines(page, page+20, (int *)gdata);
    }
}

void prev_page(GtkWidget *widget, gpointer gdata)
{
    if(lines>20)
    {
	if(page>20) page-=20; else page = 0;
	g_print("Going to prev page..\n");
	clear_lines();
	(void)show_lines(page, page+20, (int *)gdata);
    }
}


GtkWidget *PutNewButton(GtkWidget *field, char *Label)
{
    GtkWidget *button = gtk_button_new_with_label(Label);
	gtk_box_pack_start(GTK_BOX(field), button, FALSE, FALSE, 3);
	gtk_widget_show(button);
    
    return (button);
}


void AddText(GtkWidget *text_view, gchar *text, gchar *type)
{
    GtkTextIter   iter;
    GtkTextBuffer *buf;
    gchar	  *tmp;
    
    buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	gtk_text_buffer_get_end_iter(buf, &iter);
	gtk_text_buffer_insert_with_tags_by_name(buf, &iter, text, -1, type, NULL);
}


GtkWidget *PutNewHField(GtkWidget *field)
{
    GtkWidget *field2 = gtk_hbox_new(FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(field), field2, FALSE, FALSE, 3);
    gtk_widget_show(field2);
    
    return (field2);
}


void show_lines(int start, int end, int *list)
{
    int	    i;

    gchar   *gtmp1 = NULL,
	    *gtmp2 = NULL,
	    *gtmp3 = NULL,
	    *gtmp4 = NULL;

    struct gg_hist_line
	    *hist_line = NULL;

    if(end>lines) end = lines;

    for(i=start;i<end;i++)
    {
	g_print("DEBUG: Processing line %d..", i);
	hist_line = formatline(list[lines+2], list[i]);
	
	if(hist_line->type == GG_HIST_SEND)
	{
	    g_print("chatsend..");
	    
		gtmp1 = gg_hist_time(atoi(hist_line->timestamp1));
		gtmp2 = g_strdup_printf("%s :: %s :: \n", "Me", gtmp1);
	    AddText(text_view, gtmp2, "header");
		g_free(gtmp1);
		g_free(gtmp2);
		
		gtmp1 = gg_convert_utf8(hist_line->data);
		gtmp2 = g_strcompress(gtmp1);
		gtmp3 = g_strdup_printf("%s\n", gtmp2);
	    AddText(text_view, gtmp3, "");
		g_free(gtmp1);
		g_free(gtmp2);
		g_free(gtmp3);
	}
	else if(hist_line->type == GG_HIST_RCV)
	{
	    g_print("chatrcv..");
	    
		gtmp1 = gg_convert_utf8(hist_line->nick);
		gtmp2 = gg_hist_time(atoi(hist_line->timestamp1));
		gtmp3 = gg_hist_time(atoi(hist_line->timestamp2));
		gtmp4 = g_strdup_printf("%s :: %s (%s) :: \n", gtmp1, gtmp2, gtmp3);
	    AddText(text_view, gtmp4, "header2");
		g_free(gtmp1);
		g_free(gtmp2);
		g_free(gtmp3);
		g_free(gtmp4);
	    
		gtmp1 = gg_convert_utf8(hist_line->data);
		gtmp2 = g_strcompress(gtmp1);
		gtmp3 = g_strdup_printf("%s\n", gtmp2);
	    AddText(text_view, gtmp3, "");
		g_free(gtmp1);
		g_free(gtmp2);
		g_free(gtmp3);
	}
	else if(hist_line->type == GG_HIST_STATUS)
	{
	    g_print("status..");
#ifdef SHOW_STATUS	    
		gtmp1 = gg_hist_time(atoi(hist_line->timestamp1));	
		gtmp2 = gg_convert_utf8(hist_line->data);
		gtmp3 = g_strdup_printf("%s *** %s *** \n", gtmp1, gtmp2);
	    AddText(text_view, gtmp3, "status");
		g_free(gtmp1);
		g_free(gtmp2);
		g_free(gtmp3);
#endif
	}
	else
	{
	    g_print("unknown!..");
	    
//		gtmp1 = g_strdup_printf("*** Error *** Unable to read message ***\n");
//	    AddText(text_view, gtmp1, "warning");
//		g_free(gtmp1);
	}
Gool:	free(hist_line->id);
	free(hist_line->nick);
	free(hist_line->timestamp1);
	free(hist_line->timestamp2);
	free(hist_line->ip);
	free(hist_line->data);
	free(hist_line);
	
	g_print("\n");
    }
}







int main(int argc, char **argv)
{
    GtkWidget *window;
    GtkWidget *button;
    GtkWidget *field;
    GtkWidget *base_field;
    
    gtk_init(&argc, &argv);
    
    g_print("\nGnuGadu2 History Prototype Reader\n"
	    "Version 0.07\n"
	    "e-mail: <kazik.cz@interia.pl>\n"
	    "GG: 3781462\n"
	    "\n");
	    
    if(argc != 2)	g_error("Please enter path to the history file\n"
				"For example:\n"
				"%s /home/coth/.gg/history/1234567\n"
				"%s /root/.gg/history/7654321\n"
				"\n", argv[0], argv[0]);


    g_print("Opening file..\n");
    int fd = open(argv[1], O_RDONLY);
    if(fd == -1) g_error("Failed to open history file! %s\n", argv[1]);
    

    g_print("Getting lines count..\n");
    lines = lines_count(fd);
    int list[lines+2];
    

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	     gtk_window_set_title(GTK_WINDOW(window), "GnuGadu2 History Prototype");
	     gtk_signal_connect(GTK_OBJECT(window), "delete_event", GTK_SIGNAL_FUNC(CloseWindow), NULL);
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
	    
	    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	    gtk_text_buffer_create_tag(buf, "header", "foreground", "brown", "font", "Trebuchet MS Bold 11", NULL);
	    gtk_text_buffer_create_tag(buf, "header2", "foreground", "blue", "font", "Trebuchet MS Bold 11", NULL);
	    gtk_text_buffer_create_tag(buf, "status", "foreground", "#995500", "font", "Trebuchet MS Bold 11", NULL);
	    gtk_text_buffer_create_tag(buf, "warning", "foreground", "#990099", "font", "Trebuchet MS Bold 11", NULL);
	    gtk_text_buffer_create_tag(buf, "", "foreground", "#000000", "font", "Trebuchet MS 11", NULL);

	field = gtk_scrolled_window_new(NULL, NULL);
	    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(field), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	    gtk_container_add(GTK_CONTAINER(field), text_view);

	gtk_box_pack_start(GTK_BOX(base_field), field, TRUE, TRUE, 3);
	gtk_widget_show(field);
	

	/* Bottom side - 4 buttons */
	field = PutNewHField(base_field);
	    button = PutNewButton(field, "Find");

	    button = PutNewButton(field, "First page");
		gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(first_page), list);

	    button = PutNewButton(field, "Next page");
		gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(next_page), list);

	    button = PutNewButton(field, "Previous page");
		gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(prev_page), list);

	    button = PutNewButton(field, "Last page");
		gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(last_page), list);

	    button = PutNewButton(field, "Close");
		gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(CloseWindow), NULL);
    
    /* Add 'Base container' to window */
    gtk_container_add(GTK_CONTAINER(window), base_field);
    

    int	pos = 0,
	i = 0;
    list[lines+2] = fd;
    g_print("Reading offsets of lines..%d\n", lines);
    g_print("Found %d lines, and read %d of 'em\n", lines, get_lines(fd, list));


    int start = 0,
	end = display_pages;
    show_lines(start, end, list);

    gtk_widget_show(text_view);
    gtk_widget_show(base_field);
    gtk_widget_show(window);
    
    gtk_main();
    close(fd);
    
    g_print("\n"
	    "Closing...\n"
	    "\n");
    
    return 0;
}
