/*
 *  Copyright (C) 2002-2003 Henrik Öhman
 *
 *  This file is part of Sirius.
 *
 *  Sirius is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Sirius is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Sirius; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Author: Henrik Öhman <henrik@bitvis.nu>
 *
 */

#include <siriusid.h>
CVSID("$Id: sirius.c,v 1.40 2003/08/04 07:23:36 ohman Exp $");

#include <config.h>
#include <gnome.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libgnomeui/gnome-window-icon.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#include "sirius.h"
#include "board.h"
#include "evaluation.h"
#include "book.h"
#include "search.h"
#include "timer.h"
#include "gnome-canvas-pimage.h"

#ifdef DEBUG
#include <testboard.h>
#endif

#define BOARD_SIZE   8
#define BOARD_BORDER 20
#define CELL_SIZE    57
#define CELL_PAD     1
#define GRID_SIZE    (8 * CELL_PAD)
#define BOARD_WIDHT  (BOARD_SIZE * CELL_SIZE + 2 * BOARD_BORDER + GRID_SIZE)

#define DEFAULT_SHOW_LEGALS     0    /* no */
#define DEFAULT_SHOW_LAST       0    /* no */
#define DEFAULT_USE_BOOK        1    /* yes */
#define DEFAULT_FIXDEPTH        0    /* time dependent */
#define DEFAULT_COMPUTER_TIME   1
#define DEFAULT_HUMAN_TIME      5

GtkWidget *   window;
GtkWidget *   canvas;
GtkWidget *   black_label = NULL;
GtkWidget *   white_label = NULL;
GtkWidget *   pref_dialog = NULL;
GnomeAppBar * appbar;

GdkPixmap *pix;
GdkPixbuf *image;

pthread_mutex_t search_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t thread;
int lock;
int paus;


board *game = NULL;
undo_info ui;

int computer[2];
int human[2];
int show_legal;
int show_last;
int use_book;
int fixdepth;
int computer_time;
int human_time;
int disc_count[2];
int time_up;

GConfClient *client;
int last;
guint time_timer;
guint comp_timer;

GnomeCanvasGroup *bg;

typedef struct _tile {
	int                x;
	int                y;
	int                image;
	GdkPixbuf *        img;
	GnomeCanvasItem *  item;
} tile;


typedef struct _history {
	board *game;
	struct _history *next;
} history;

history *undo_struct = NULL;
history *redo_struct = NULL;

tile tiles[BOARD_SIZE][BOARD_SIZE];
GnomeCanvasItem **legals;
GnomeCanvasItem **lasts;

void  update_board(board *game);
void  create_window();
void  create_board();
void  setup_pixmaps();
void  game_over();
void  get_prefs();
void  set_prefs();
void  new_game_reply_cb(gint reply, gpointer data);
gint  check_for_computer_move(gpointer data);
char *get_time_str(board *game, int color);
gint  update_time_and_score(gpointer data);
void  computer_thread(int *m);
void  update_appbar();

void quit_game_cb(GtkWidget *widget, gpointer data);
void new_game_reply_cb(gint reply, gpointer data);
void new_game_cb(GtkWidget *widget, gpointer data);
void hint_cb(gint reply, gpointer data);
static void pref_dialog_response (GtkDialog *dialog, gint response, gpointer data);
void undo_cb(GtkWidget *widget, gpointer data);
void redo_cb(GtkWidget *widget, gpointer data);
void preferences_cb(GtkWidget *widget, gpointer data);
void about_cb(GtkWidget *widget, gpointer data);
history *add_to_history(history *h, board *g);
void free_history(history *h);

/* MENU */
GnomeUIInfo gamemenu[] = {
        GNOMEUIINFO_MENU_NEW_GAME_ITEM(new_game_cb, NULL),
        GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_UNDO_MOVE_ITEM(undo_cb, NULL),
	GNOMEUIINFO_MENU_REDO_MOVE_ITEM(redo_cb, NULL),
	GNOMEUIINFO_SEPARATOR, 
	GNOMEUIINFO_MENU_HINT_ITEM(hint_cb, NULL),
	GNOMEUIINFO_SEPARATOR, 
        GNOMEUIINFO_MENU_EXIT_ITEM(quit_game_cb, NULL),
        GNOMEUIINFO_END
};

GnomeUIInfo settingsmenu[] = {
        GNOMEUIINFO_MENU_PREFERENCES_ITEM(preferences_cb, NULL),
        GNOMEUIINFO_END
};

GnomeUIInfo helpmenu[] = {
        GNOMEUIINFO_MENU_ABOUT_ITEM(about_cb, NULL),
        GNOMEUIINFO_END
};

GnomeUIInfo mainmenu[] = {
        GNOMEUIINFO_MENU_GAME_TREE(gamemenu),
        GNOMEUIINFO_MENU_SETTINGS_TREE(settingsmenu),
        GNOMEUIINFO_MENU_HELP_TREE(helpmenu),
        GNOMEUIINFO_END
};


void message_box(gchar *message) {
	GtkWidget *box;

	paus = 1;

	box = gtk_message_dialog_new (GTK_WINDOW (window), GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, message);
	gtk_dialog_run (GTK_DIALOG(box));
	gtk_widget_destroy(box);

	paus = 0;
}

void quit_game_cb(GtkWidget *widget, gpointer data) {
	set_prefs();
	
	if(lock) {
		pthread_cancel(thread); 
		pthread_join(thread, NULL);
		lock = 0;
	}

	deinit_board(game);	
	deinit_timer();

	free_history(undo_struct);
        free_history(redo_struct);

	gtk_main_quit();
}

void new_game_reply_cb(gint reply, gpointer data) {
	if(reply == GNOME_YES) {
		if(game != NULL) {
			gtk_timeout_remove(time_timer);
			gtk_timeout_remove(comp_timer);
			
			if(lock) {
				pthread_cancel(thread);
				pthread_join(thread, NULL);

				lock = 0;
			}

			deinit_board(game);
	                game = NULL;
	        }

		last    = 0;
		time_up = 0;
		lock    = 0;

		free_history(undo_struct);
		free_history(redo_struct);

		undo_struct = NULL;
		redo_struct = NULL;
		
		gtk_widget_set_sensitive(gamemenu[2].widget, FALSE);
		gtk_widget_set_sensitive(gamemenu[3].widget, FALSE);
		gtk_widget_set_sensitive(gamemenu[5].widget, FALSE);
		
		if(human[0] == -1 && human[0] == human[1]) {
			game = init_board(game, computer_time * 60000, computer_time * 60000);
		} else {
			if(computer[0] == -1 && computer[0] == computer[1]) {
				game = init_board(game, human_time * 60000, human_time * 60000);
			} else {
				if(computer[0] == BLACK || computer[1] == BLACK) {
					game = init_board(game, computer_time * 60000, human_time * 60000);
				} else {
					game = init_board(game, human_time * 60000, computer_time * 60000);
				}
			}
		}

		legal_moves(game,0);

	        update_board(game);

		update_appbar();
		
		gtk_widget_set_sensitive(black_label, TRUE);
		gtk_widget_set_sensitive(white_label, TRUE);

		comp_timer = gtk_timeout_add(400, check_for_computer_move, NULL);
		time_timer = gtk_timeout_add(500, update_time_and_score, game);

		if(human[0] == game->color_to_move || human[1] == game->color_to_move) {
			start_timer(0, BLACK);
		}
	}
}


void new_game_cb(GtkWidget *widget, gpointer data) {
			
	get_prefs();
	if(game != NULL) {
       		gtk_timeout_remove(time_timer);
		gtk_timeout_remove(comp_timer);

		if(lock) {
			pthread_cancel(thread);
			pthread_join(thread, NULL);

			lock = 0;
		}

		deinit_board(game);
		game = NULL;
	}
	
	lock    = 0;
	last    = 0;
	time_up = 0;
	
	free_history(undo_struct);
	free_history(redo_struct);

	undo_struct = NULL;
	redo_struct = NULL;
	
	gtk_widget_set_sensitive(gamemenu[2].widget, FALSE);
	gtk_widget_set_sensitive(gamemenu[3].widget, FALSE);
	gtk_widget_set_sensitive(gamemenu[5].widget, FALSE);
	
	if(human[0] == -1 && human[0] == human[1]) {
		game = init_board(game, computer_time * 60000, computer_time * 60000);
	} else {
		if(computer[0] == -1 && computer[0] == computer[1]) {
			game = init_board(game, human_time * 60000, human_time * 60000);
		} else {
			if(computer[0] == BLACK || computer[1] == BLACK) {
				game = init_board(game, computer_time * 60000, human_time * 60000);
			} else {
				game = init_board(game, human_time * 60000, computer_time * 60000);
			}
		}
	}

 	legal_moves(game, 0);

	update_board(game);

	update_appbar();
	
	gtk_widget_set_sensitive(black_label, TRUE);
	gtk_widget_set_sensitive(white_label, TRUE);
	
	comp_timer = gtk_timeout_add(400, check_for_computer_move, NULL);
	time_timer = gtk_timeout_add(500, update_time_and_score, game);
	
	if(human[0] == game->color_to_move || human[1] == game->color_to_move) {
		start_timer(0, BLACK);
	}
}

void hint_cb(gint reply, gpointer data) {

	if(game != NULL && game->game_over == 0) {
		char *tmp;
		int score;

		paus = 1;
		score = computer_move(game, 0, 500, 1);
	
		if(score > 0) {
			tmp = (char *) malloc(32);
			sprintf(tmp, _("%s looks good..."), int2pos_mirror(score));
			message_box(tmp);
		}
		paus = 0;
	}
}

static void pref_dialog_response (GtkDialog *dialog, gint response, gpointer data) {
	if (response == GTK_RESPONSE_HELP) {
		/* this page should really be made... */
		gnome_help_display("http://sirius.bitvis.nu/prefs.html", NULL, NULL);
		return;
	}

	set_prefs();
	gtk_widget_destroy (pref_dialog);
	pref_dialog = NULL;
}

void show_legals_cb(GtkWidget *widget, gpointer data) {
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
		show_legal = 1;
	} else {
		show_legal = 0;
	}
	
	gconf_client_set_int(client,"/apps/sirius/show_legals", show_legal, NULL);
	if(game != NULL) {
		update_board(game);
	}
}

void show_last_cb(GtkWidget *widget, gpointer data) {
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
		show_last = 1;
	} else {
		show_last = 0;
	}

	gconf_client_set_int(client,"/apps/sirius/show_last", show_last, NULL);
	if(game != NULL) {
		update_board(game);
	}
}

void use_book_cb(GtkWidget *widget, gpointer data) {
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
		use_book = 1;
	} else {
		use_book = 0;
	}
	
	gconf_client_set_int(client,"/apps/sirius/use_book", use_book, NULL);
}


void player_black_cb(GtkWidget *widget, char *data) {

	if(strcmp(data,_("computer")) == 0) {
		computer[0] = BLACK;
		human[0]    = -1;
	}
	if(strcmp(data,_("human")) == 0) {
		computer[0] = -1;
		human[0]    = BLACK;
	}
	gconf_client_set_int(client,"/apps/sirius/computer_1", computer[0], NULL);
	gconf_client_set_int(client,"/apps/sirius/human_1", human[0], NULL);
}

void player_white_cb(GtkWidget *widget, char *data) {

	if(strcmp(data,_("computer")) == 0) {
		computer[1] = WHITE;
		human[1]    = -1;
	}
	if(strcmp(data,_("human")) == 0) {
		computer[1] = -1;
		human[1]    = WHITE;
	}
	gconf_client_set_int(client,"/apps/sirius/computer_2", computer[1], NULL);
	gconf_client_set_int(client,"/apps/sirius/human_2", human[1], NULL);
}

void computer_time_cb(GtkWidget *widget, char *data) {
	int time;

	sscanf(data, "%d min", &time);
	computer_time = time;

	gconf_client_set_int(client,"/apps/sirius/computer_time", time, NULL);
	if(game != NULL) {
		message_box(_("This will take effekt when you start a new game, or restart Sirius"));
	}
}

void human_time_cb(GtkWidget *widget, char *data) {
	int time;

	if(strcmp(data, _("Infinit")) == 0) {
		human_time = -1;
	} else {
		sscanf(data, "%d min", &time);
		human_time = time;
	}

	gconf_client_set_int(client,"/apps/sirius/human_time", time, NULL);
	if(game != NULL) {
		message_box(_("This will take effekt when you start a new game, or restart Sirius"));
	}
}

void depth_cb(GtkWidget *widget, char *data) {
	int depth;

	if(strcmp(data,_("Depend on time")) == 0) {
		depth = 0;
	} else {
		sscanf(data, "%d ply", &depth);
	}
	fixdepth = depth;

	gconf_client_set_int(client,"/apps/sirius/fixdepth", fixdepth, NULL);
}	

void undo_cb(GtkWidget *widget, gpointer data) {
	if(undo_struct != NULL && game->game_over == 0) {

		stop_timer(game);
		gtk_timeout_remove(time_timer); 

		if(lock) {
			pthread_cancel(thread);
			pthread_join(thread, NULL);
			lock = 0;
		} else {
			redo_struct = add_to_history(redo_struct, game);
		}

			
		free(game); 
		game = undo_struct->game;
      		undo_struct = undo_struct->next;
		
		time_timer = gtk_timeout_add(500, update_time_and_score, game);
		start_timer(0, game->color_to_move);
		
		update_board(game);

		update_appbar();

		gtk_widget_set_sensitive(gamemenu[3].widget, TRUE);

		if(undo_struct == NULL) {
			gtk_widget_set_sensitive(widget, FALSE);
		} else {
			gtk_widget_set_sensitive(widget, TRUE);
		}		
	}
}

void redo_cb(GtkWidget *widget, gpointer data) {
	if(redo_struct != NULL && game->game_over == 0) {
		stop_timer(game);
		gtk_timeout_remove(time_timer);

		if(lock) {
			pthread_cancel(thread);
			pthread_join(thread, NULL);
			lock = 0;
                } else {
			undo_struct = add_to_history(undo_struct, game);
		}

		free(game);
		game = redo_struct->game;
		redo_struct = redo_struct->next;

		time_timer = gtk_timeout_add(500, update_time_and_score, game);
                start_timer(0, game->color_to_move);

		update_board(game);

		update_appbar();

		if(redo_struct == NULL) {
			gtk_widget_set_sensitive(widget, FALSE);
		} else {
			gtk_widget_set_sensitive(widget, TRUE);
		}
	}
}


void preferences_cb(GtkWidget *widget, gpointer data) {
	GtkWidget *l, *hb, *f, *fv, *cols, *col1, *col2;
	GtkWidget *option_p_black;
	GtkWidget *menu_p_black;
	GtkWidget *option_p_white;
	GtkWidget *menu_p_white;
	GtkWidget *checkbox_legal;
	GtkWidget *checkbox_last;
	GtkWidget *option_depth;
	GtkWidget *menu_depth;
	GtkWidget *option_c_time;
	GtkWidget *menu_c_time;
	GtkWidget *option_h_time;
	GtkWidget *menu_h_time;
	GtkWidget *checkbox_book;
	int i;
	char **str;
	char *player_str[] = {_("computer"), _("human"), NULL};
	char *depth_str[] = {"1 ply","2 ply","3 ply","4 ply","5 ply","6 ply",_("Depend on time"),NULL};
	char *time_computer_str[]  = {"1 min","2 min","5 min","10 min","20 min","30 min",NULL};
	char *time_human_str[]  = {"1 min","2 min","5 min","10 min","20 min","30 min",_("Infinit"),NULL};
	
	if (pref_dialog) {
		gtk_window_present(GTK_WINDOW (pref_dialog));
		return;
	}

	pref_dialog = gtk_dialog_new_with_buttons (_("Sirius - Preferences"),
						   GTK_WINDOW (window),
						   GTK_DIALOG_DESTROY_WITH_PARENT,
						   GTK_STOCK_HELP, GTK_RESPONSE_HELP,
						   GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
						   NULL);

	gtk_dialog_set_default_response (GTK_DIALOG (pref_dialog),
					 GTK_RESPONSE_OK);
	g_signal_connect (G_OBJECT (pref_dialog), "response",
			  G_CALLBACK(pref_dialog_response), NULL);

        cols = gtk_hbox_new (FALSE, FALSE);
        col1 = gtk_vbox_new (FALSE, FALSE);
        col2 = gtk_vbox_new (FALSE, FALSE);

	f = gtk_frame_new(_("Players"));
	gtk_container_set_border_width (GTK_CONTAINER (f), 5);
	gtk_box_pack_start_defaults (GTK_BOX(col1), f);

        /* black players */
	hb = gtk_hbox_new (FALSE, FALSE);

	fv = gtk_vbox_new (0, 5);
	gtk_container_set_border_width (GTK_CONTAINER (fv), 5);

	menu_p_black = gtk_menu_new();
	for (str = player_str; *str; str++) {
		GtkWidget *menu_item = gtk_menu_item_new_with_label (*str);
		gtk_widget_show (menu_item);

		gtk_menu_shell_append (GTK_MENU_SHELL(menu_p_black), menu_item);
		g_signal_connect (G_OBJECT(menu_item), "activate", G_CALLBACK(player_black_cb), (char *)*str);
	}
	if(computer[0] == BLACK) {
		gtk_menu_set_active(GTK_MENU(menu_p_black), 0);
	}
	if(human[0] == BLACK) {
		gtk_menu_set_active(GTK_MENU(menu_p_black), 1);
	}

	option_p_black = gtk_option_menu_new();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (option_p_black), menu_p_black);
	
	hb = gtk_hbox_new (FALSE, FALSE);
	
	l = gtk_label_new (_("Black: "));
	    
	gtk_box_pack_start_defaults (GTK_BOX(hb), l);
	gtk_box_pack_start_defaults (GTK_BOX(hb), option_p_black);
	gtk_box_pack_start_defaults (GTK_BOX(fv), hb);


        /* white players */
	menu_p_white = gtk_menu_new();
	for (str = player_str; *str; str++) {
		GtkWidget *menu_item = gtk_menu_item_new_with_label(*str);
		gtk_widget_show (menu_item);

		gtk_menu_shell_append (GTK_MENU_SHELL(menu_p_white), menu_item);
		g_signal_connect (G_OBJECT(menu_item), "activate", G_CALLBACK(player_white_cb), (char *)*str);
	}
	if(computer[1] == WHITE) {
		gtk_menu_set_active(GTK_MENU(menu_p_white), 0);
	}
	if(human[1] == WHITE) {
		gtk_menu_set_active(GTK_MENU(menu_p_white), 1);
	}

	option_p_white = gtk_option_menu_new();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (option_p_white), menu_p_white);
	
	hb = gtk_hbox_new (FALSE, FALSE);
	
	l = gtk_label_new (_("White: "));
	    
	gtk_box_pack_start_defaults (GTK_BOX(hb), l);
	gtk_box_pack_start_defaults (GTK_BOX(hb), option_p_white);
	gtk_box_pack_start_defaults (GTK_BOX(fv), hb);
	gtk_container_add (GTK_CONTAINER (f), fv);


	f = gtk_frame_new (_("Support"));
	gtk_container_set_border_width (GTK_CONTAINER (f), 5);

	fv = gtk_vbox_new (0, 5);
	gtk_container_set_border_width (GTK_CONTAINER (fv), 5);

	/* Show legals */
	checkbox_legal = gtk_check_button_new_with_label(_("Show legal moves"));
	if(show_legal) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(checkbox_legal), TRUE);
	g_signal_connect (G_OBJECT(checkbox_legal), "toggled",
			  G_CALLBACK(show_legals_cb), NULL);

	gtk_box_pack_start (GTK_BOX(fv), checkbox_legal, FALSE, FALSE, 0);

	/* show last move */
	checkbox_last = gtk_check_button_new_with_label(_("Show last move"));
	if(show_last) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(checkbox_last), TRUE);
	g_signal_connect (G_OBJECT(checkbox_last), "toggled",
			  G_CALLBACK(show_last_cb), NULL);

	gtk_box_pack_start (GTK_BOX(fv), checkbox_last, FALSE, FALSE, 0);
	
	
	gtk_box_pack_start_defaults (GTK_BOX(col1), f);
	gtk_container_add (GTK_CONTAINER (f), fv);

	f = gtk_frame_new (_("Computer opponent"));
	gtk_container_set_border_width (GTK_CONTAINER (f), 5);

	hb = gtk_hbox_new (FALSE, FALSE);

	fv = gtk_vbox_new (0, 5);
	gtk_container_set_border_width (GTK_CONTAINER (fv), 5);

	menu_depth = gtk_menu_new();
	i = 0;
	for (str = depth_str; *str; str++) {
		int depth;

		GtkWidget *menu_item = gtk_menu_item_new_with_label(*str);
		gtk_widget_show(menu_item);

		gtk_menu_shell_append(GTK_MENU_SHELL(menu_depth), menu_item);
		g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(depth_cb), (char *)*str);

		sscanf((char *)*str, "%d ply", &depth);
		if((depth == fixdepth) || ((fixdepth == 0) && (strcmp((char *)*str, _("Depend on time")) == 0))) {
			gtk_menu_set_active(GTK_MENU(menu_depth), i);
		}

		depth = -1;
		i++;
	}
	option_depth = gtk_option_menu_new();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (option_depth), menu_depth);
	
	hb = gtk_hbox_new (FALSE, FALSE);
	
	l = gtk_label_new (_("Search depth: "));
	    
	gtk_box_pack_start_defaults (GTK_BOX(hb), l);
	gtk_box_pack_start_defaults (GTK_BOX(hb), option_depth);
	gtk_box_pack_start_defaults (GTK_BOX(fv), hb);

	
	/* use opening book */

	hb = gtk_hbox_new (FALSE, FALSE);
	checkbox_book = gtk_check_button_new_with_label(_("Use opening book"));
	if(use_book) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(checkbox_book), TRUE);
	g_signal_connect (G_OBJECT(checkbox_book), "toggled",
			  G_CALLBACK(use_book_cb), NULL);

	gtk_box_pack_start (GTK_BOX(fv), checkbox_book, FALSE, FALSE, 0);

	gtk_box_pack_start_defaults (GTK_BOX(fv), hb);
	gtk_box_pack_start_defaults (GTK_BOX(col2), f) ;
	gtk_container_add (GTK_CONTAINER (f), fv);

	
	f = gtk_frame_new (_("Time"));
	gtk_container_set_border_width (GTK_CONTAINER(f), 5);

	fv = gtk_vbox_new (0, 5);
	gtk_container_set_border_width (GTK_CONTAINER(fv), 5);
	gtk_container_add (GTK_CONTAINER (f), fv);

	menu_c_time = gtk_menu_new();
	i = 0;
	for (str = time_computer_str; *str; str++) {
		int time;
		GtkWidget *menu_item = gtk_menu_item_new_with_label (*str);
		gtk_widget_show (menu_item);

		gtk_menu_shell_append (GTK_MENU_SHELL(menu_c_time), menu_item);
		g_signal_connect (G_OBJECT(menu_item), "activate", G_CALLBACK(computer_time_cb), (char *)*str);

		sscanf((char *)*str,"%d min", &time);
		if(time == computer_time) {
			gtk_menu_set_active(GTK_MENU(menu_c_time), i);
		}
		time = -1;
		i++;
	}
	option_c_time = gtk_option_menu_new();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (option_c_time), menu_c_time);
	
	hb = gtk_hbox_new (FALSE, FALSE);
	
	l = gtk_label_new (_("Computer: "));
	    
	gtk_box_pack_start_defaults (GTK_BOX(hb), l);
	gtk_box_pack_start_defaults (GTK_BOX(hb), option_c_time);
	gtk_box_pack_start_defaults (GTK_BOX(fv), hb);
		
	menu_h_time = gtk_menu_new();
	i = 0;
	for (str = time_human_str; *str; str++) {
		int time;
		GtkWidget *menu_item = gtk_menu_item_new_with_label (*str);
		gtk_widget_show (menu_item);

		gtk_menu_shell_append (GTK_MENU_SHELL(menu_h_time), menu_item);
		g_signal_connect (G_OBJECT(menu_item), "activate", G_CALLBACK(human_time_cb), (char *)*str);

		sscanf((char *)*str,"%d min", &time);
		if(time == human_time) {
                        gtk_menu_set_active(GTK_MENU(menu_h_time), i);
		}
		time = -1;
                i++;
	}
	option_h_time = gtk_option_menu_new();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (option_h_time), menu_h_time);
	
	hb = gtk_hbox_new (FALSE, FALSE);
	
	l = gtk_label_new (_("Human: "));
	    
	gtk_box_pack_start_defaults (GTK_BOX(hb), l);
	gtk_box_pack_start_defaults (GTK_BOX(hb), option_h_time);
	gtk_box_pack_start_defaults (GTK_BOX(fv), hb);
	gtk_box_pack_start_defaults (GTK_BOX(col2), f);
	gtk_box_pack_start_defaults (GTK_BOX(cols), col1);
        gtk_box_pack_start_defaults (GTK_BOX(cols), col2);
        gtk_box_pack_start_defaults (GTK_BOX(GTK_DIALOG(pref_dialog)->vbox), cols);

        gtk_widget_show_all (pref_dialog);
}

void about_cb(GtkWidget *widget, gpointer data) {
        static GtkWidget *about;
	GdkPixbuf *pixbuf = NULL;
	char *filename = NULL;
	char *tmp;
	
        const gchar *authors[] = {
		_("Code: Henrik Ohman <henrik@bitvis.nu>"),
		_("Graphics: Geoff Parker <geoff@lorem-ipsum.net>"),
                NULL
        };

        const gchar *documenters[] = {
                NULL
        };

	tmp = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_APP_DATADIR, "pixmaps/sirius.png", FALSE, NULL);
	filename = gnome_unconditional_pixmap_file(tmp);
	g_free(tmp);
	
	if (filename != NULL) {
		pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
		g_free (filename);
	}
	
        if (about) {
                gdk_window_raise(about->window);
                gdk_window_show(about->window);
                return;
        }
	
        about = gnome_about_new (_("Sirius"), VERSION,
                                 _("(C) 2002-2003 Henrik Ohman"),
				 _("Send comments and bug reports to:\nhenrik@bitvis.nu"),
                                 (const char **)authors,
				 (const char **)documenters,
                                 NULL,pixbuf);

        gtk_signal_connect (GTK_OBJECT (about), "destroy", GTK_SIGNAL_FUNC
			    (gtk_widget_destroyed), &about);

	gtk_window_set_transient_for(GTK_WINDOW(about), GTK_WINDOW(window));
        gtk_widget_show (about);
}



static int canvas_event(GnomeCanvas *c, GdkEvent *event, tile *t) {

	if((event->type != GDK_BUTTON_PRESS) || (game == NULL) || (lock == 1) || (paus == 1)) {
		return (FALSE);
	} else {
		int move = (t->y + 1) + t->x * 8;

		if(legal(game, move) && (game->game_over == 0) && ((human[0] == game->color_to_move) || (human[1] == game->color_to_move))) {
			
			stop_timer(game);

			/* save away the board for the undo */
			undo_struct = add_to_history(undo_struct, game);
			free_history(redo_struct);
			redo_struct = NULL;
			
			gtk_widget_set_sensitive(gamemenu[2].widget, TRUE);
			gtk_widget_set_sensitive(gamemenu[3].widget, FALSE);			
			gtk_widget_set_sensitive(gamemenu[5].widget, FALSE);			
			
			game = do_move(game, move, &ui);

			last = move;
			legal_moves(game, 0);

			update_board(game);

			update_time_and_score((gpointer)(game));
			
			update_appbar();

			if((game->pass == 1) && (game->game_over == 0)) {
				do_pass(game, &ui);
				legal_moves(game,0);

				update_board(game);
				if(game->game_over == 0) {
					if((human[0] > -1 && human[1] > -1) || (computer[0] > -1 && computer[1] > -1)) {
						if(game->color_to_move == WHITE) {
							message_box(_("Black has to pass!"));
						} else {
							message_box(_("White has to pass!"));
						}
					} else {
						message_box(_("Computer pass!\n\nYour turn again."));
					}				
				}
			}

			if(game->color_to_move == human[0] || game->color_to_move == human[1]) {
				start_timer(0, game->color_to_move);
			}
			
			return (TRUE);
		}

	}	

	return (FALSE);
}

gint check_for_computer_move(gpointer data) {
	if((game != NULL) && (game->game_over == 1)) {
		game_over();
		deinit_board(game);
		game = NULL;
		return (FALSE);
	}

	
	if((game != NULL) && (lock == 0) && (paus == 0) && (game->game_over == 0) && ((computer[0] == game->color_to_move) || (computer[1] == game->color_to_move))) {
		int move = -42;

		paus = 1;
		lock = 1;
			
		pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
		pthread_create(&thread, NULL, (void *)computer_thread, &move);
			
		do {
			g_main_context_iteration(NULL, FALSE);

		} while(lock == 1);

		if(move == -42) {
			paus = 0;
			return (TRUE);
		}			

		if(game == NULL) {
			/* the game has been aborted (user pressed "new game") */
			paus = 0;
			return (FALSE);
		}
		
		if(move > 0) {
			clean_transpositiontable(game);
			game = do_move(game, move, &ui);

			last = move;
		}

		legal_moves(game, 0);
		if((game->pass == 1) && (game->game_over == 0)) {
			do_pass(game, &ui);
			update_board(game);
			legal_moves(game,0);
			if(game->game_over == 0) {
				if((human[0] > -1 && human[1] > -1) || (computer[0] > -1 && computer[1] > -1)) {
					if(game->color_to_move == WHITE) {
						message_box(_("Black has to pass!"));
					} else {
						message_box(_("White has to pass!"));
					}
				} else {
					message_box(_("You have to pass!"));
				}
			}
		}

		update_board(game);

		update_time_and_score((gpointer)(game));

		update_appbar();

		if(game->color_to_move == human[0] || game->color_to_move == human[1]) {
			gtk_widget_set_sensitive(gamemenu[5].widget, TRUE);
			start_timer(0, game->color_to_move);
		}
		paus = 0;
	}
	
	return (TRUE);
}


void computer_thread(int *m) {
	
	*m = computer_move(game, fixdepth, 0, use_book);
	lock = 0;
}

history *add_to_history(history *h, board *g) {
	history *tmp;

	tmp = (history *) malloc(sizeof(history));
	tmp->game = (board *) malloc(sizeof(board));
	memcpy(tmp->game, g, sizeof(board));
	tmp->next = h;
	h = tmp;

	return (h);
}


void free_history(history *h) {
	while(h != NULL) {
		board *bo = h->game;
		history *hi = h;
		
		h = h->next;
		
		free(bo);
		free(hi);
	}
}
	

char *get_time_str(board *game, int color) {
	char *str;
	char *dub;
	long tmp;
	int h,m,s;

	if(game == NULL) {
		return (NULL);
	}
	
	tmp = get_time_left(game, color);

	if(tmp < 0) {
		return (NULL);
	}

	str = (char *) malloc(10);
	dub = (char *) malloc(3);
	
	h = (int) tmp / 3600000;
	tmp -= h * 3600000;

	m = (int) tmp / 60000;
	tmp -= m * 60000;

	s = (int) tmp / 1000;

	if(h > 9) {
		sprintf(dub, "%d", h);
	} else {
		sprintf(dub, "0%d", h);
	}
	strcpy(str, dub);
	if(m > 9) {
		sprintf(dub, "%d", m);
	} else {
		sprintf(dub, "0%d", m);
	}
	strcat(str, ":");
	strcat(str, dub);
	if(s > 9) {
		sprintf(dub, "%d", s);
	} else {
		sprintf(dub, "0%d", s);
	}
	strcat(str, ":");
	strcat(str, dub);
	
	return (str);
}


void game_over() {
	char message[1024];
	int num;

	message[0] = 0;

	gtk_timeout_remove(time_timer);
	update_time_and_score((gpointer)(game));

	gtk_widget_set_sensitive(gamemenu[2].widget, FALSE);
	gtk_widget_set_sensitive(gamemenu[5].widget, FALSE);
	
	if(time_up == 1) {
		if(get_time_left(game, BLACK) < 0) {
			sprintf(message, _("Time is up, white wins!\nAnother game?"));
		} else {
			sprintf(message, _("Time is up, black wins!\nAnother game?"));
		}
	} else {
	
		if(numbits(game->black) == numbits(game->white)) {
			sprintf(message, _("Draw! Another game?"));
		} else {
			num = numbits(game->black) - numbits(game->white);
			
			if((human[0] == BLACK) && (computer[1] == WHITE) && (num > 0)) {
				sprintf(message, _("You won with %d - %d! Another game?"), numbits(game->black), numbits(game->white));
			}
			if((human[1] == WHITE) && (computer[0] == BLACK) && (num < 0)) {
				sprintf(message, _("You won with %d - %d! Another game?"), numbits(game->white), numbits(game->black));
			}
			if((human[0] == BLACK) && (computer[1] == WHITE) && (num < 0)) {
				sprintf(message, _("You lost with %d - %d! Another try?"), numbits(game->black), numbits(game->white));
			}
			if((human[1] == WHITE) && (computer[0] == BLACK) && (num > 0)) {
				sprintf(message, _("You lost with %d - %d! Another try?"), numbits(game->white), numbits(game->black));
			}
			if(message[0] == 0) {
				if(num > 0) {
					sprintf(message, _("Black wins with %d - %d! Another game?"), numbits(game->black), numbits(game->white));
				} else {
					sprintf(message, _("White wins with %d - %d! Another game?"), numbits(game->white), numbits(game->black));
				}
			}
		}
	}
	gnome_app_question_modal(GNOME_APP(window), message, new_game_reply_cb, NULL);
}



void update_appbar() {

	gnome_appbar_pop(GNOME_APPBAR (appbar));

	if((computer[0] == game->color_to_move) || (computer[1] == game->color_to_move)) {
		gnome_appbar_push (GNOME_APPBAR(appbar), _("Computer is thinking"));
	} else {
		if((human[0] == game->color_to_move) || (human[1] == game->color_to_move)) {
			gnome_appbar_push (GNOME_APPBAR(appbar), _("Your move"));
		} else {
			if(game->color_to_move == BLACK) {
				gnome_appbar_push (GNOME_APPBAR(appbar), _("Black's move"));
			} else {
				gnome_appbar_push (GNOME_APPBAR(appbar), _("White's move"));
			}
		}
	}
}

	

	


gint update_time_and_score(gpointer data) {
	int black_discs;
	int white_discs;
	char *black_time;
	char *white_time;
	board *g = (board *)data;
	
	if(g == NULL) {
		deinit_timer();
		return (FALSE);
	}
	
	if(lock != 1) {
		black_discs = numbits(g->black);
		white_discs = numbits(g->white);
		disc_count[0] = black_discs;
		disc_count[1] = white_discs;
	} else {
		black_discs = disc_count[0];
		white_discs = disc_count[1];
	}

	black_time = get_time_str(g, BLACK);
	white_time = get_time_str(g, WHITE);

	if(black_time == NULL) {
		black_time = "00:00:00";

		if(!(human_time == -1 && (human[0] == BLACK || human[1] == BLACK))) {
			g->game_over = 1;
			time_up = 1;
		}
	}
	
	if(white_time == NULL) {
		white_time = "00:00:00";

		if(!(human_time == -1 && (human[0] == WHITE || human[1] == WHITE))) {
			g->game_over = 1;
			time_up = 1;
		}
	}	

	gtk_label_set_text(GTK_LABEL(black_label), g_strdup_printf(_("%d discs  %s"), black_discs, black_time));
	gtk_label_set_text(GTK_LABEL(white_label), g_strdup_printf(_("%d discs  %s"), white_discs, white_time));

	return (TRUE);
}


void update_board(board *game) {
	int x,y;
	u64 mask;

	mask = 1;

	for(x=0; x<BOARD_SIZE; x++) {
		for(y=0; y<BOARD_SIZE; y++) {
			int col = pos(game, mask);
			int xx,yy;

			if(tiles[x][y].image != col) {
				gdk_pixbuf_copy_area (image, CELL_SIZE * col, 0,
						      CELL_SIZE, CELL_SIZE,
						      tiles[x][y].img, 0, 0);

				tiles[x][y].image = col;
				gnome_canvas_item_set (tiles[x][y].item, "image", tiles[x][y].img, NULL);				
			}
			if((tiles[x][y].image == EMPTY) && show_legal && legal(game,((y + 1) + x * 8))) {
				gnome_canvas_item_show(legals[(y + 1) + x * 8]);
			} else {
				gnome_canvas_item_hide(legals[(y + 1) + x * 8]);
			}

			yy = (int)((last % 8) - 1);
			xx = (int)(last / 8);
			if(yy == -1) {
				yy = 7;
				xx -= 1;
			}
			if((tiles[x][y].image != EMPTY) && show_last && (x == xx) && (y == yy)) {
				gnome_canvas_item_show(lasts[(y + 1) + x * 8]);
			} else {
				gnome_canvas_item_hide(lasts[(y + 1) + x * 8]);
			}

			mask++;
		}
	}

}

int get_gconf_int(char *key, int def) {
	GConfValue *value = NULL;
        gint retval = def;

        value = gconf_client_get (client, key, NULL);

        if (value == NULL) {
                return (def);
        }
        if (value->type == GCONF_VALUE_INT) {
                retval = gconf_value_get_int (value);
                gconf_value_free (value);
        }
	return (retval);
}

		  
void get_prefs() {	
	computer[0] =   get_gconf_int("/apps/sirius/computer_1", -1);
	computer[1] =   get_gconf_int("/apps/sirius/computer_2", WHITE);
	human[0] =      get_gconf_int("/apps/sirius/human_1", BLACK);
	human[1] =      get_gconf_int("/apps/sirius/human_2", -1);
	show_legal =    get_gconf_int("/apps/sirius/show_legals", DEFAULT_SHOW_LEGALS);
	show_last  =    get_gconf_int("/apps/sirius/show_last", DEFAULT_SHOW_LAST);
	use_book =      get_gconf_int("/apps/sirius/use_book", DEFAULT_USE_BOOK);
	fixdepth =      get_gconf_int("/apps/sirius/fixdepth", DEFAULT_FIXDEPTH);
	computer_time = get_gconf_int("/apps/sirius/computer_time", DEFAULT_COMPUTER_TIME);
	human_time =    get_gconf_int("/apps/sirius/human_time", DEFAULT_HUMAN_TIME);
}

void set_prefs() {
	gconf_client_set_int(client,"/apps/sirius/computer_1",computer[0],NULL);
	gconf_client_set_int(client,"/apps/sirius/computer_2",computer[1],NULL);
	gconf_client_set_int(client,"/apps/sirius/human_1",human[0],NULL);
	gconf_client_set_int(client,"/apps/sirius/human_2",human[1],NULL);
	gconf_client_set_int(client,"/apps/sirius/show_legals",show_legal,NULL);
	gconf_client_set_int(client,"/apps/sirius/show_last",show_last,NULL);
	gconf_client_set_int(client,"/apps/sirius/use_book",use_book,NULL);
	gconf_client_set_int(client,"/apps/sirius/fixdepth",fixdepth,NULL);
	gconf_client_set_int(client,"/apps/sirius/computer_time",computer_time,NULL);
	gconf_client_set_int(client,"/apps/sirius/human_time",human_time,NULL);
}
	


/* MAIN */

int main (int argc, char *argv[]) {

#ifndef DEBUG
	
	gnome_program_init(APPNAME, VERSION, LIBGNOMEUI_MODULE, argc, argv, GNOME_PARAM_APP_DATADIR, DATADIR, NULL);
	
	client = gconf_client_get_default();

	get_prefs();

	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	
	textdomain(GETTEXT_PACKAGE);
	
	create_window();	

	setup_pixmaps(); 
	
	create_board();

	init_evaluation(gnome_program_locate_file(NULL, GNOME_FILE_DOMAIN_APP_DATADIR, "sirius/corner", FALSE, NULL),
			gnome_program_locate_file(NULL, GNOME_FILE_DOMAIN_APP_DATADIR, "sirius/edge", FALSE, NULL));
	init_openingbook(gnome_program_locate_file(NULL, GNOME_FILE_DOMAIN_APP_DATADIR, "sirius/opening_book", FALSE, NULL));

	gnome_app_flash(GNOME_APP(window), _("Welcome to Sirius!"));

	gtk_main();

#else  
	board *   b = NULL;
	int       move,i;
	char *    res[5];


	res[0] = "a2 (+38)";
	res[1] = "h4 (+0)";
	res[2] = "g2 (+6)";
	res[3] = "c7 (-12)";
	res[4] = "d2 (-14)";

	printf("tt size: %d\n", sizeof(transpositiontable));
	printf("board size: %d\n", sizeof(board));
	
	init_evaluation("../data/corner","../data/edge");
	init_openingbook("../data/opening_book");

	for(i=2 ; i<TESTSUITESIZE; i++) {
		b = load_board(b, ffotest[i]);
		dump(b);
		move = computer_move(b,10,0,0);
	       	printf("OPTIONAL: %s\n", res[i-1]); 
		deinit_board(b);
	}

#endif	
	
	return (0);
}


void create_window() {
	GtkWidget *status_box;
	GdkPixbuf *icon;
	char *filename = NULL;
	char *tmp;
	
	tmp = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_APP_DATADIR, "pixmaps/sirius.png", FALSE, NULL);
	filename = gnome_unconditional_pixmap_file(tmp);
	g_free(tmp);

	window = gnome_app_new("sirius", _("Sirius"));
        gnome_app_create_menus(GNOME_APP(window), mainmenu);
        gtk_window_set_policy(GTK_WINDOW (window), TRUE, TRUE, TRUE);	

	if (filename != NULL) {
		icon = gdk_pixbuf_new_from_file(filename, NULL);
		g_free (filename);
		gtk_window_set_icon ( GTK_WINDOW ( window ), icon );
		g_object_unref ( G_OBJECT (icon) );
	}
	
	
	status_box = gtk_hbox_new(FALSE, 10);

	black_label = gtk_label_new(_("Black:"));
	gtk_box_pack_start (GTK_BOX (status_box), black_label, TRUE, TRUE, 0);
	black_label = gtk_label_new(_("0 discs  00:00:00"));
	gtk_widget_set_sensitive(black_label, FALSE);
	gtk_box_pack_start (GTK_BOX (status_box), black_label, TRUE, TRUE, 0);	

	white_label = gtk_label_new(_("White:"));
	gtk_box_pack_start (GTK_BOX (status_box), white_label, TRUE, TRUE, 0);
	white_label = gtk_label_new(_("0 discs  00:00:00"));
	gtk_widget_set_sensitive(white_label, FALSE);
	gtk_box_pack_start (GTK_BOX (status_box), white_label, TRUE, TRUE, 0);	
	
	gtk_widget_show_all (status_box);

	gtk_widget_set_sensitive(gamemenu[2].widget, FALSE);
	gtk_widget_set_sensitive(gamemenu[3].widget, FALSE);
	gtk_widget_set_sensitive(gamemenu[5].widget, FALSE);
	
	appbar = GNOME_APPBAR(gnome_appbar_new(FALSE, TRUE, GNOME_PREFERENCES_USER));
	gtk_box_pack_end(GTK_BOX(appbar), status_box, FALSE, FALSE, 0);
	
        gnome_app_set_statusbar(GNOME_APP (window), GTK_WIDGET(appbar));	
	
	gnome_app_install_menu_hints(GNOME_APP(window), mainmenu);

	canvas = gnome_canvas_new();

	gnome_canvas_set_scroll_region(GNOME_CANVAS(canvas), 0.0, 0.0, 
				       BOARD_SIZE * CELL_SIZE + 2 * BOARD_BORDER + GRID_SIZE,
				       BOARD_SIZE * CELL_SIZE + 2 * BOARD_BORDER + GRID_SIZE);

	gnome_canvas_set_pixels_per_unit(GNOME_CANVAS(canvas), 1);
	
	gtk_widget_set_usize(canvas,
			     BOARD_SIZE * CELL_SIZE + 2 * BOARD_BORDER + GRID_SIZE + 10,
			     BOARD_SIZE * CELL_SIZE + 2 * BOARD_BORDER + GRID_SIZE + 10);
	
	gnome_app_set_contents(GNOME_APP(window), canvas);

	gtk_widget_show (canvas);
	
	gtk_signal_connect(GTK_OBJECT(window), "delete_event",
                           GTK_SIGNAL_FUNC(quit_game_cb), NULL);

	gtk_widget_show(window);
}

void create_board() {
	char *letters[8] = {"a","b","c","d","e","f","g","h"};
	char *numbers[8] = {"1","2","3","4","5","6","7","8"};	
	int i;
	int x,y;
	GdkColor *darkgreen = g_malloc(sizeof(GdkColor));
	darkgreen->pixel = 0;
	darkgreen->red   = 6000;
	darkgreen->green = 13000;
	darkgreen->blue  = 6000;

	gdk_color_alloc(gdk_colormap_get_system(), darkgreen);

	legals = g_malloc(sizeof(GnomeCanvasItem) * 64);
	lasts  = g_malloc(sizeof(GnomeCanvasItem) * 64);
	
	gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas)), gnome_canvas_rect_get_type(),
			      "x1", (double) 5,
			      "y1", (double) 5,
			      "x2", (double) (BOARD_SIZE * CELL_SIZE + 2 * BOARD_BORDER + 2),
			      "y2", (double) (BOARD_SIZE * CELL_SIZE + 2 * BOARD_BORDER + 2), 
			      "fill_color_gdk", darkgreen,
			      NULL);

	for(i=0; i<BOARD_SIZE; i++) {
		gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas)), gnome_canvas_text_get_type(),
				      "text", letters[i],
				      "x", (double)(BOARD_BORDER + (CELL_SIZE / 2) + CELL_SIZE * i + CELL_PAD * i - 2),
				      "y", (double)((BOARD_BORDER / 2) + 2),
				      "anchor", GTK_ANCHOR_WEST,
				      "size_points",(double)(8),
				      "fill_color", "white",
				      NULL);
		gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas)), gnome_canvas_text_get_type(),
				      "text", letters[i],
				      "x", (double)(BOARD_BORDER + (CELL_SIZE / 2) + CELL_SIZE * i + CELL_PAD * i - 2),
				      "y", (double)(BOARD_BORDER + (BOARD_BORDER / 2) + BOARD_SIZE * CELL_SIZE + 2),
				      "anchor", GTK_ANCHOR_WEST,
				      "size_points",(double)(8),   
				      "fill_color", "white",
				      NULL);
		gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas)), gnome_canvas_text_get_type(),
				      "text", numbers[i],
				      "x", (double)(BOARD_BORDER / 2),
				      "y", (double)(BOARD_BORDER + (CELL_SIZE / 2) + CELL_SIZE * i + CELL_PAD * i - 2),
				      "anchor", GTK_ANCHOR_WEST,
				      "size_points",(double)(8),
				      "fill_color", "white",
				      NULL);
		gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas)), gnome_canvas_text_get_type(),
				      "text", numbers[i],
				      "x", (double)(BOARD_BORDER + (BOARD_BORDER / 2) + BOARD_SIZE * CELL_SIZE + 1),
				      "y", (double)(BOARD_BORDER + (CELL_SIZE / 2) + CELL_SIZE * i + CELL_PAD * i - 2),
				      "anchor", GTK_ANCHOR_WEST,
				      "size_points",(double)(8),
				      "fill_color", "white",
				      NULL);
	}

	for(y=0; y<BOARD_SIZE; y++) {
		for(x=0; x<BOARD_SIZE; x++) {
			tiles[x][y].item = gnome_canvas_item_new (gnome_canvas_root(GNOME_CANVAS(canvas)),
								  gnome_canvas_group_get_type(),
								  NULL);
			
			tiles[x][y].img = gdk_pixbuf_new(gdk_pixbuf_get_colorspace(image),
							 TRUE, gdk_pixbuf_get_bits_per_sample(image),
							 CELL_SIZE, CELL_SIZE);

			gdk_pixbuf_copy_area(image, CELL_SIZE * 2, 0,
					     CELL_SIZE, CELL_SIZE,
					     tiles[x][y].img, 0, 0);

			tiles[x][y].item = gnome_canvas_item_new(GNOME_CANVAS_GROUP(tiles[x][y].item),
								 gnome_canvas_pimage_get_type(),
								 "image", tiles[x][y].img,
								 "x",(double)(BOARD_BORDER + x * CELL_SIZE + x * CELL_PAD),
								 "y",(double)(BOARD_BORDER + y * CELL_SIZE + y * CELL_PAD),
								 "width",(double)CELL_SIZE,
								 "height",(double)CELL_SIZE,
								 NULL);
			
			tiles[x][y].image = EMPTY;
			tiles[x][y].x     = x;
			tiles[x][y].y     = y;
			
			gtk_signal_connect (GTK_OBJECT(tiles[x][y].item), "event",
					    (GtkSignalFunc) canvas_event, &tiles[x][y]);

			legals[(y + 1) + x * 8] = gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas)),
					      gnome_canvas_ellipse_get_type(),
					      "x1",(double)(BOARD_BORDER + x * CELL_SIZE + x * CELL_PAD + CELL_SIZE / 2 - 2),
					      "y1",(double)(BOARD_BORDER + y * CELL_SIZE + y * CELL_PAD + CELL_SIZE / 2 - 2),
					      "x2",(double)(BOARD_BORDER + x * CELL_SIZE + x * CELL_PAD + CELL_SIZE / 2 + 2),
					      "y2",(double)(BOARD_BORDER + y * CELL_SIZE + y * CELL_PAD + CELL_SIZE / 2 + 2),
					      "fill_color", "darkred",
					      NULL);

			gtk_signal_connect (GTK_OBJECT(legals[(y + 1) + x * 8]), "event",
					    (GtkSignalFunc) canvas_event, &tiles[x][y]);

			lasts[(y + 1) + x * 8] = gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas)),
					      gnome_canvas_ellipse_get_type(),
					      "x1",(double)(BOARD_BORDER + x * CELL_SIZE + x * CELL_PAD + CELL_SIZE - 6),
					      "y1",(double)(BOARD_BORDER + y * CELL_SIZE + y * CELL_PAD + 6),
					      "x2",(double)(BOARD_BORDER + x * CELL_SIZE + x * CELL_PAD + CELL_SIZE - 3),
					      "y2",(double)(BOARD_BORDER + y * CELL_SIZE + y * CELL_PAD + 3),
					      "fill_color", "darkblue",
					      NULL);

			gnome_canvas_item_hide(legals[(y + 1) + x * 8]);
			gnome_canvas_item_hide(lasts[(y + 1) + x * 8]);		
		}
	}

	gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas)),
			      gnome_canvas_ellipse_get_type(),
			      "x1",(double)(BOARD_BORDER + CELL_SIZE*2),
			      "y1",(double)(BOARD_BORDER + CELL_SIZE*2),
			      "x2",(double)(BOARD_BORDER + CELL_SIZE*2 + 3),
			      "y2",(double)(BOARD_BORDER + CELL_SIZE*2 + 3),
			      "fill_color_gdk", darkgreen,
			      NULL);
	gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas)),
			      gnome_canvas_ellipse_get_type(),
			      "x1",(double)(BOARD_BORDER + CELL_SIZE*6 + 4),
			      "y1",(double)(BOARD_BORDER + CELL_SIZE*2),
			      "x2",(double)(BOARD_BORDER + CELL_SIZE*6 + 7),
			      "y2",(double)(BOARD_BORDER + CELL_SIZE*2 + 3),
			      "fill_color_gdk", darkgreen,
			      NULL);
	gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas)),
			      gnome_canvas_ellipse_get_type(),
			      "x1",(double)(BOARD_BORDER + CELL_SIZE*2),
			      "y1",(double)(BOARD_BORDER + CELL_SIZE*6 + 4),
			      "x2",(double)(BOARD_BORDER + CELL_SIZE*2 + 3),
			      "y2",(double)(BOARD_BORDER + CELL_SIZE*6 + 7),
			      "fill_color_gdk", darkgreen,
			      NULL);
	gnome_canvas_item_new(gnome_canvas_root(GNOME_CANVAS(canvas)),
			      gnome_canvas_ellipse_get_type(),
			      "x1",(double)(BOARD_BORDER + CELL_SIZE*6 + 4),
			      "y1",(double)(BOARD_BORDER + CELL_SIZE*6 + 4),
			      "x2",(double)(BOARD_BORDER + CELL_SIZE*6 + 7),
			      "y2",(double)(BOARD_BORDER + CELL_SIZE*6 + 7),
			      "fill_color_gdk", darkgreen,
			      NULL);
}


void setup_pixmaps() {
	char *fn;

        fn = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_APP_DATADIR, "pixmaps/sirius/marker_classic.png", FALSE, NULL);

        if (!g_file_exists (fn)) {
                char *s = g_strdup_printf(_("Could not find file %s"), fn);
                GtkWidget *box;

                box = gnome_message_box_new(s, GNOME_MESSAGE_BOX_ERROR, GNOME_STOCK_BUTTON_OK, NULL);
                gnome_dialog_run(GNOME_DIALOG (box));
                exit (1);
        }

        image = gdk_pixbuf_new_from_file(fn, NULL);

        g_free(fn);
}
