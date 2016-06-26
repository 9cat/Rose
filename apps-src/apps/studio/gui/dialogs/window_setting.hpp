/* $Id: campaign_difficulty.hpp 49603 2011-05-22 17:56:17Z mordante $ */
/*
   Copyright (C) 2010 - 2011 by Ignacio Riquelme Morelle <shadowm2006@gmail.com>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef GUI_DIALOGS_WINDOW_SETTING_HPP_INCLUDED
#define GUI_DIALOGS_WINDOW_SETTING_HPP_INCLUDED

#include "gui/dialogs/cell_setting.hpp"
#include "gui/dialogs/mode_navigate.hpp"
#include "mkwin_controller.hpp"
#include <vector>

class display;
class mkwin_controller;
class unit;

namespace gui2 {

class tstacked_widget;
class tlistbox;
class treport;

class twindow_setting : public tsetting_dialog, public tmode_navigate
{
public:
	enum {BASE_PAGE, LINKED_GROUP_PAGE, CONTEXT_MENU_PAGE, PATCH_PAGE};
	enum {AUTOMATIC_PAGE, MANUAL_PAGE};

	explicit twindow_setting(display& disp, mkwin_controller& controller, const unit& u, const std::vector<std::string>& textdomains);

private:
	/** Inherited from tdialog, implemented by REGISTER_DIALOG. */
	virtual const std::string& window_id() const;

	/** Inherited from tdialog. */
	void pre_show(CVideo& video, twindow& window);

	bool pre_toggle_tabbar(twidget* widget, twidget* previous);
	void toggle_report(twidget* widget);

	bool click_report(twidget* widget);

	void pre_base(twindow& window);
	void pre_linked_group(twindow& window);
	void pre_context_menu(twindow& window);
	void pre_patch(twindow& window);

	bool save_base(twindow& window);
	bool save_linked_group(twindow& window);
	bool save_context_menu(twindow& window);
	bool save_patch(twindow& window);

	void fill_automatic_page(twindow& window);
	void fill_manual_page(twindow& window);

	void set_textdomain(twindow& window);
	void set_textdomain_label(twindow& window);

	void set_orientation(twindow& window);
	void set_orientation_label(twindow& window);

	void set_tile_shape(twindow& window);
	void set_tile_shape_label(twindow& window);

	void set_definition(twindow& window);
	void set_definition_label(twindow& window);
	void set_horizontal_layout(twindow& window);
	void set_vertical_layout(twindow& window);
	void set_horizontal_layout_label(twindow& window);
	void set_vertical_layout_label(twindow& window);

	void automatic_placement_toggled(twidget* widget);
	void save(twindow& window, bool& handled, bool& halt);
	void swap_page(twindow& window, int page, bool swap);

	//
	// context menu page
	//
	void reload_menu_table(tmenu2& menu, twindow& window, int cursel);
	std::string get_menu_item_id(tmenu2& menu, twindow& window, int exclude);
	void reload_submenu_navigate(tmenu2& menu, twindow& window, const tmenu2* cursel);
	tmenu2* current_submenu() const;

	bool menu_pre_toggle_tabbar(twidget* widget, twidget* previous);
	void menu_toggle_tabbar(twidget* widget);

	bool submenu_pre_toggle_tabbar(twidget* widget, twidget* previous);
	void submenu_toggle_tabbar(twidget* widget);

	void append_menu_item(twindow& window);
	void modify_menu_item(twindow& window);
	void erase_menu_item(twindow& window);

	void map_menu_item_to(twindow& window, tmenu2& menu, int row);
	void item_selected(twindow& window, tlistbox& list, const int type);

	void refresh_parent_desc(twindow& window, tmenu2& menu);

	//
	// patch page
	//
	void patch_toggle_tabbar(twidget* widget);
	std::string form_tab_label(treport& navigate, int at) const;

	void fill_change_list2(tlistbox& list, const tmode& mode, const unit::tchild& child);
	void fill_change_list(twindow& window);
	int calculate_change_count(const unit::tchild& child, int at) const;

	void fill_remove_list2(tlistbox& list, const tmode& mode, const unit::tchild& child);
	void fill_remove2_list2(tlistbox& list, const tmode& mode, const unit::tchild& child);
	void fill_remove_list(twindow& window);
	int calculate_remove_count(const unit::tchild& child, int at) const;
	int calculate_remove2_count(const unit::tchild& child, int at) const;

	void switch_patch_cfg(twindow& window);

	void append_patch(twindow& window);
	void erase_patch(twindow& window);
	void rename_patch(twindow& window);

private:
	display& disp_;
	mkwin_controller& controller_;
	const unit& u_;

	const std::vector<std::string>& textdomains_;
	tstacked_widget* layout_panel_;

	tstacked_widget* bar_panel_;
	treport* bar_;
	int current_page_;
	std::vector<tmenu2>& menus_;

	treport* menu_navigate_;
	treport* submenu_navigate_;
	treport* patch_bar_;
	int patch_current_tab_;
};


}


#endif /* ! GUI_DIALOGS_CAMPAIGN_DIFFICULTY_HPP_INCLUDED */
