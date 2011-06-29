/*
 *   Copyright (C) 2007-2011 Tristan Heaven <tristanheaven@gmail.com>
 *
 *   This file is part of GtkHash.
 *
 *   GtkHash is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   GtkHash is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with GtkHash. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GTKHASH_GUI_H
#define GTKHASH_GUI_H

#include <stdbool.h>
#include <gtk/gtk.h>

#include "hash.h"

#define GUI_VIEW_IS_VALID(X) (((X) >= 0) && ((X) <= GUI_VIEW_FILE_LIST))
#define GUI_STATE_IS_VALID(X) (((X) >= 0) && ((X) <= GUI_STATE_BUSY))

enum gui_view_e {
	GUI_VIEW_INVALID = -1,
	GUI_VIEW_FILE,
	GUI_VIEW_TEXT,
	GUI_VIEW_FILE_LIST,
};

enum gui_state_e {
	GUI_STATE_INVALID = -1,
	GUI_STATE_IDLE,
	GUI_STATE_BUSY,
};

struct {
	GtkWindow *window;
	GtkMenuItem *menuitem_file, *menuitem_save_as, *menuitem_quit;
	GtkMenuItem *menuitem_edit;
	GtkMenuItem *menuitem_cut, *menuitem_copy, *menuitem_paste;
	GtkMenuItem *menuitem_delete, *menuitem_select_all, *menuitem_prefs;
	GtkMenuItem *menuitem_about;
	GtkRadioMenuItem *radiomenuitem_file, *radiomenuitem_text, *radiomenuitem_file_list;
	GtkToolbar *toolbar;
	GtkToolButton *toolbutton_add, *toolbutton_remove, *toolbutton_clear;
	GtkVBox *vbox_single, *vbox_list;
	GtkHBox *hbox_input, *hbox_output;
	GtkVBox *vbox_outputlabels, *vbox_digests_file, *vbox_digests_text;
	GtkEntry *entry;
	GtkFileChooserButton *filechooserbutton;
	GtkLabel *label_text, *label_file;
	GtkTreeView *treeview;
	GtkTreeSelection *treeselection;
	GtkTreeModel *treemodel;
	GtkListStore *liststore;
	GtkMenu *menu_treeview;
	GtkMenuItem *menuitem_treeview_add, *menuitem_treeview_remove;
	GtkMenuItem *menuitem_treeview_clear;
	GtkMenu *menu_treeview_copy;
	GtkMenuItem *menuitem_treeview_copy;
	GtkMenuItem *menuitem_treeview_show_toolbar;
	GtkHSeparator *hseparator_buttons;
	GtkProgressBar *progressbar;
	GtkButton *button_hash, *button_stop;
	GtkDialog *dialog;
	GtkTable *dialog_table;
	GtkComboBox *dialog_combobox;
	GtkButton *dialog_button_close;
	struct {
		GtkToggleButton *button;
		GtkLabel *label;
		GtkEntry *entry_file, *entry_text;
		GtkMenuItem *menuitem_treeview_copy;
	} hash_widgets[HASH_FUNCS_N];
} gui;

void gui_init(const char *datadir);
unsigned int gui_add_uris(GSList *uris, enum gui_view_e view);
void gui_run(void);
void gui_deinit(void);
void gui_set_view(const enum gui_view_e view);
enum gui_view_e gui_get_view(void);
void gui_set_digest_format(const enum digest_format_e format);
enum digest_format_e gui_get_digest_format(void);
void gui_update(void);
void gui_clear_digests(void);
void gui_set_state(const enum gui_state_e state);
enum gui_state_e gui_get_state(void);
bool gui_is_maximised(void);

#endif
