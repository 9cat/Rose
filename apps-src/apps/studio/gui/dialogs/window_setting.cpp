/* $Id: campaign_difficulty.cpp 49602 2011-05-22 17:56:13Z mordante $ */
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

#define GETTEXT_DOMAIN "studio-lib"

#include "gui/dialogs/window_setting.hpp"

#include "display.hpp"
#include "mkwin_controller.hpp"
#include "formula_string_utils.hpp"
#include "gettext.hpp"

#include "gui/dialogs/helper.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/window.hpp"
#include "gui/widgets/label.hpp"
#include "gui/widgets/button.hpp"
#include "gui/widgets/text_box.hpp"
#include "gui/widgets/scroll_text_box.hpp"
#include "gui/widgets/toggle_button.hpp"
#include "gui/widgets/stacked_widget.hpp"
#include "gui/widgets/listbox.hpp"
#include "gui/widgets/report.hpp"
#include "gui/dialogs/combo_box.hpp"
#include "gui/dialogs/message.hpp"
#include "unit.hpp"

#include <boost/bind.hpp>

namespace gui2 {

/*WIKI
 * @page = GUIWindowDefinitionWML
 * @order = 2_campaign_difficulty
 *
 * == Campaign difficulty ==
 *
 * The campaign mode difficulty menu.
 *
 * @begin{table}{dialog_widgets}
 *
 * title & & label & m &
 *         Dialog title label. $
 *
 * message & & scroll_label & o &
 *         Text label displaying a description or instructions. $
 *
 * listbox & & listbox & m &
 *         Listbox displaying user choices, defined by WML for each campaign. $
 *
 * -icon & & control & m &
 *         Widget which shows a listbox item icon, first item markup column. $
 *
 * -label & & control & m &
 *         Widget which shows a listbox item label, second item markup column. $
 *
 * -description & & control & m &
 *         Widget which shows a listbox item description, third item markup column. $
 *
 * @end{table}
 */

REGISTER_DIALOG(window_setting)

twindow_setting::twindow_setting(display& disp, mkwin_controller& controller, const unit& u, const std::vector<std::string>& textdomains)
	: tsetting_dialog(u.cell())
	, tmode_navigate(controller, disp)
	, disp_(disp)
	, controller_(controller)
	, u_(u)
	, textdomains_(textdomains)
	, current_page_(0)
	, menus_(controller.context_menus())
	, bar_(NULL)
	, menu_navigate_(NULL)
	, submenu_navigate_(NULL)
	, patch_bar_(NULL)
	, patch_current_tab_(0)
{
	VALIDATE(!menus_.empty(), null_str);
}

void twindow_setting::pre_show(CVideo& /*video*/, twindow& window)
{
	window.set_canvas_variable("border", variant("default-border"));

	std::stringstream ss;
	const std::pair<std::string, gui2::tcontrol_definition_ptr>& widget = u_.widget();

	ss.str("");
	ss << _("Window setting");
	tlabel* label = find_widget<tlabel>(&window, "title", false, true);
	label->set_label(ss.str());

	// prepare navigate bar.
	std::vector<std::string> labels;
	labels.push_back(_("Base"));
	labels.push_back(_("Linked group"));
	labels.push_back(_("Context menu"));
	labels.push_back(_("Patch"));

	bar_ = find_widget<treport>(&window, "bar", false, true);
	bar_->tabbar_init(true, "tab");
	bar_->set_boddy(find_widget<twidget>(&window, "_bar_panel", false, true));

	int index = 0;
	for (std::vector<std::string>::const_iterator it = labels.begin(); it != labels.end(); ++ it) {
		tcontrol* widget = bar_->create_child(null_str, null_str, reinterpret_cast<void*>(index ++));
		widget->set_label(*it);
		bar_->insert_child(*widget);
	}
	bar_->select(BASE_PAGE);
	bar_->replacement_children();

	bar_panel_ = find_widget<tstacked_widget>(&window, "bar_panel", false, true);
	bar_panel_->set_radio_layer(BASE_PAGE);

	pre_base(window);
	pre_linked_group(window);

	if (controller_.theme()) {
		pre_context_menu(window);
		pre_patch(window);

	} else {
		bar_->set_child_visible(CONTEXT_MENU_PAGE, false);
		bar_->set_child_visible(PATCH_PAGE, false);
	}
}

void twindow_setting::pre_base(twindow& window)
{
	ttext_box* text_box = find_widget<ttext_box>(&window, "id", false, true);
	text_box->set_label(cell_.id);

	text_box = find_widget<ttext_box>(&window, "description", false, true);
	text_box->set_value(cell_.window.description);

	set_textdomain_label(window);
	connect_signal_mouse_left_click(
			  find_widget<tbutton>(&window, "textdomain", false)
			, boost::bind(
				&twindow_setting::set_textdomain
				, this
				, boost::ref(window)));

	set_orientation_label(window);
	connect_signal_mouse_left_click(
			  find_widget<tbutton>(&window, "orientation", false)
			, boost::bind(
				&twindow_setting::set_orientation
				, this
				, boost::ref(window)));

	if (controller_.theme()) {
		find_widget<tgrid>(&window, "grid_base_dialog", false).set_visible(twidget::INVISIBLE);

		set_tile_shape_label(window);
		connect_signal_mouse_left_click(
				  find_widget<tbutton>(&window, "_set_tile_shape", false)
				, boost::bind(
					&twindow_setting::set_tile_shape
					, this
					, boost::ref(window)));

	} else {
		find_widget<tgrid>(&window, "grid_base_theme", false).set_visible(twidget::INVISIBLE);

		set_definition_label(window);
		connect_signal_mouse_left_click(
				  find_widget<tbutton>(&window, "set_definition", false)
				, boost::bind(
					&twindow_setting::set_definition
					, this
					, boost::ref(window)));

		ttoggle_button* toggle = find_widget<ttoggle_button>(&window, "click_dismiss1", false, true);
		toggle->set_value(cell_.window.click_dismiss);

		toggle = find_widget<ttoggle_button>(&window, "automatic_placement", false, true);
		toggle->set_value(cell_.window.automatic_placement);
		toggle->set_callback_state_change(boost::bind(&twindow_setting::automatic_placement_toggled, this, _1));

		layout_panel_ = find_widget<tstacked_widget>(&window, "layout", false, true);
		if (cell_.window.automatic_placement) {
			swap_page(window, AUTOMATIC_PAGE, false);
		} else {
			swap_page(window, MANUAL_PAGE, false);
		}
	}

	connect_signal_mouse_left_click(
			  find_widget<tbutton>(&window, "save", false)
			, boost::bind(
				&twindow_setting::save
				, this
				, boost::ref(window)
				, _3, _4));
}

void twindow_setting::pre_linked_group(twindow& window)
{
	treport& report = find_widget<treport>(&window, "test_report", false);
	report.multiline_init(false, "portrait_icon");

	tcontrol* widget = report.create_child(null_str, null_str, NULL);
	widget->set_canvas_variable("image", variant("buttons/copy.png"));
	widget->set_label("buttons/copy.png");
	report.insert_child(*widget);

	widget = report.create_child(null_str, null_str, NULL);
	widget->set_canvas_variable("image", variant("buttons/cut.png"));
	widget->set_label("buttons/cut.png");
	report.insert_child(*widget);

	widget = report.create_child(null_str, null_str, NULL);
	widget->set_canvas_variable("image", variant("buttons/erase.png"));
	widget->set_label("buttons/erase.png");
	report.insert_child(*widget);

	widget = report.create_child(null_str, null_str, NULL);
	widget->set_canvas_variable("image", variant("buttons/erase_child.png"));
	widget->set_label("buttons/erase_child.png");
	report.insert_child(*widget);

	widget = report.create_child(null_str, null_str, NULL);
	widget->set_canvas_variable("image", variant("buttons/grid.png"));
	widget->set_label("buttons/grid.png");
	report.insert_child(*widget);

	report.replacement_children();
}

void twindow_setting::pre_context_menu(twindow& window)
{
	tbutton* button = find_widget<tbutton>(&window, "append_menu_item", false, true);
	connect_signal_mouse_left_click(
		  *button
		, boost::bind(
			&twindow_setting::append_menu_item
			, this
			, boost::ref(window)));

	connect_signal_mouse_left_click(
			  find_widget<tbutton>(&window, "erase_menu_item", false)
			, boost::bind(
				&twindow_setting::erase_menu_item
				, this
				, boost::ref(window)));

	connect_signal_mouse_left_click(
			  find_widget<tbutton>(&window, "modify_menu_item", false)
			, boost::bind(
				&twindow_setting::modify_menu_item
				, this
				, boost::ref(window)));


	tlistbox& list = find_widget<tlistbox>(&window, "menu", false);
	list.set_callback_value_change(dialog_callback3<twindow_setting, tlistbox, &twindow_setting::item_selected>);

	submenu_navigate_ = find_widget<treport>(&window, "submenu_navigate", false, true);
	submenu_navigate_->tabbar_init(true, "tab");
	submenu_navigate_->set_boddy(find_widget<twidget>(&window, "submenu_panel", false, true));

	reload_submenu_navigate(menus_[0], window, NULL);
	reload_menu_table(menus_[0], window, 0);
}

void twindow_setting::pre_patch(twindow& window)
{
	patch_bar_ = tmode_navigate::pre_show(window, "navigate");
	patch_bar_->set_boddy(find_widget<twidget>(&window, "navigate_panel", false, true));

	tbutton* button = find_widget<tbutton>(&window, "_append_patch", false, true);
	connect_signal_mouse_left_click(
		  *button
		, boost::bind(
			&twindow_setting::append_patch
			, this
			, boost::ref(window)));

	button = find_widget<tbutton>(&window, "_erase_patch", false, true);
	connect_signal_mouse_left_click(
		  *button
		, boost::bind(
			&twindow_setting::erase_patch
			, this
			, boost::ref(window)));

	button = find_widget<tbutton>(&window, "_rename_patch", false, true);
	connect_signal_mouse_left_click(
		  *button
		, boost::bind(
			&twindow_setting::rename_patch
			, this
			, boost::ref(window)));

	patch_current_tab_ = 1;
	patch_bar_->select(patch_current_tab_);
	patch_bar_->set_child_visible(0, false);

	switch_patch_cfg(window);
}

bool twindow_setting::save_base(twindow& window)
{
	ttext_box* text_box = find_widget<ttext_box>(&window, "id", false, true);
	cell_.id = text_box->label();
	if (!utils::isvalid_id(cell_.id, false, min_id_len, max_id_len)) {
		show_id_error(disp_, "id", utils::errstr);
		return false;
	}
	text_box = find_widget<ttext_box>(&window, "description", false, true);
	cell_.window.description = text_box->get_value();
	if (cell_.window.description.empty()) {
		show_id_error(disp_, "description", _("Can not empty!"));
		return false;
	}

	if (controller_.theme()) {

	} else {
		cell_.window.click_dismiss = find_widget<ttoggle_button>(&window, "click_dismiss1", false, true)->get_value();
		cell_.window.automatic_placement = find_widget<ttoggle_button>(&window, "automatic_placement", false, true)->get_value();
		if (!cell_.window.automatic_placement) {
			text_box = find_widget<ttext_box>(&window, "x", false, true);
			cell_.window.x = text_box->get_value();
			text_box = find_widget<ttext_box>(&window, "y", false, true);
			cell_.window.y = text_box->get_value();
			text_box = find_widget<ttext_box>(&window, "width", false, true);
			cell_.window.width = text_box->get_value();
			text_box = find_widget<ttext_box>(&window, "height", false, true);
			cell_.window.height = text_box->get_value();
		}
	}
	return true;
}

bool twindow_setting::save_linked_group(twindow& window)
{
	return true;
}

bool twindow_setting::save_context_menu(twindow& window)
{
	return true;
}

bool twindow_setting::save_patch(twindow& window)
{
	return true;
}

bool twindow_setting::pre_toggle_tabbar(twidget* widget, twidget* previous)
{
	treport* report = treport::get_report(widget);

	if (report == menu_navigate_) {
		return menu_pre_toggle_tabbar(widget, previous);

	} else if (report == submenu_navigate_) {
		return submenu_pre_toggle_tabbar(widget, previous);
	}

	twindow& window = *widget->get_window();
	bool ret = true;
	int previous_page = (int)reinterpret_cast<long>(previous->cookie());
	if (previous_page == BASE_PAGE) {
		ret = save_base(window);
	} else if (previous_page == LINKED_GROUP_PAGE) {
		ret = save_linked_group(window);
	} else if (previous_page == CONTEXT_MENU_PAGE) {
		ret = save_context_menu(window);
	}
	return ret;
}

void twindow_setting::toggle_report(twidget* widget)
{
	treport* report = treport::get_report(widget);

	if (report == menu_navigate_) {
		menu_toggle_tabbar(widget);
		return;
	} else if (report == submenu_navigate_) {
		submenu_toggle_tabbar(widget);
		return;
	} else if (report == patch_bar_) {
		patch_toggle_tabbar(widget);
		return;
	}

	int page = (int)reinterpret_cast<long>(widget->cookie());
	bar_panel_->set_radio_layer(page);

	tdialog::toggle_report(widget);
}

bool twindow_setting::click_report(twidget* widget)
{
	return false;
}

void twindow_setting::set_tile_shape(twindow& window)
{
	std::stringstream ss;
	std::vector<tval_str> items;
	std::vector<std::string> ids;

	const config& core_config = controller_.core_config();

	int index = 0;
	int def = 0;
	BOOST_FOREACH (const config &c, core_config.child_range("tb")) {
		const std::string& id = c["id"].str();
		ss.str("");
		ss << tintegrate::generate_format(id, "blue");
		if (id == cell_.window.tile_shape) {
			def = index;
		}
		items.push_back(tval_str(index ++, ss.str()));
		ids.push_back(id);
	}

	if (items.empty()) {
		return;
	}

	gui2::tcombo_box dlg(items, def);
	dlg.show(disp_.video());

	cell_.window.tile_shape = ids[dlg.selected_val()];

	set_tile_shape_label(window);
}

void twindow_setting::set_tile_shape_label(twindow& window)
{
	tlabel* label = find_widget<tlabel>(&window, "tile_shape", false, true);
	label->set_label(cell_.window.tile_shape);
}

void twindow_setting::set_definition(twindow& window)
{
	std::stringstream ss;
	std::vector<tval_str> items;

	const gui2::tgui_definition::tcontrol_definition_map& controls = gui2::current_gui->second.control_definition;
	const std::map<std::string, gui2::tcontrol_definition_ptr>& windows = controls.find("window")->second;

	int index = 0;
	int def = 0;
	for (std::map<std::string, gui2::tcontrol_definition_ptr>::const_iterator it = windows.begin(); it != windows.end(); ++ it) {
		ss.str("");
		ss << tintegrate::generate_format(it->first, "blue") << "(" << it->second->description << ")";
		if (it->first == cell_.window.definition) {
			def = index;
		}
		items.push_back(tval_str(index ++, ss.str()));
	}

	gui2::tcombo_box dlg(items, def);
	dlg.show(disp_.video());

	std::map<std::string, gui2::tcontrol_definition_ptr>::const_iterator it = windows.begin();
	std::advance(it, dlg.selected_val());
	cell_.window.definition = it->first;

	set_definition_label(window);
}

void twindow_setting::set_definition_label(twindow& window)
{
	tlabel* label = find_widget<tlabel>(&window, "definition", false, true);
	label->set_label(cell_.window.definition);
}

void twindow_setting::set_horizontal_layout(twindow& window)
{
	std::stringstream ss;
	std::vector<tval_str> items;

	for (std::map<int, tlayout>::const_iterator it = horizontal_layout.begin(); it != horizontal_layout.end(); ++ it) {
		if (it->second.val == gui2::tgrid::HORIZONTAL_GROW_SEND_TO_CLIENT) {
			continue;
		}
		ss.str("");
		ss << tintegrate::generate_img(it->second.icon + "~SCALE(24, 24)") << it->second.description;
		items.push_back(tval_str(it->first, ss.str()));
	}

	gui2::tcombo_box dlg(items, cell_.window.horizontal_placement);
	dlg.show(disp_.video());

	cell_.window.horizontal_placement = dlg.selected_val();

	set_horizontal_layout_label(window);
}

void twindow_setting::set_vertical_layout(twindow& window)
{
	std::stringstream ss;
	std::vector<tval_str> items;

	for (std::map<int, tlayout>::const_iterator it = vertical_layout.begin(); it != vertical_layout.end(); ++ it) {
		if (it->second.val == gui2::tgrid::VERTICAL_GROW_SEND_TO_CLIENT) {
			continue;
		}
		ss.str("");
		ss << tintegrate::generate_img(it->second.icon + "~SCALE(24, 24)") << it->second.description;
		items.push_back(tval_str(it->first, ss.str()));
	}

	gui2::tcombo_box dlg(items, cell_.window.vertical_placement);
	dlg.show(disp_.video());

	cell_.window.vertical_placement = dlg.selected_val();

	set_vertical_layout_label(window);
}

void twindow_setting::set_horizontal_layout_label(twindow& window)
{
	std::stringstream ss;

	ss << horizontal_layout.find(cell_.window.horizontal_placement)->second.description;
	tlabel* label = find_widget<tlabel>(&window, "_horizontal_layout", true, true);
	label->set_label(ss.str());
}

void twindow_setting::set_vertical_layout_label(twindow& window)
{
	std::stringstream ss;

	ss << vertical_layout.find(cell_.window.vertical_placement)->second.description;
	tlabel* label = find_widget<tlabel>(&window, "_vertical_layout", true, true);
	label->set_label(ss.str());
}

void twindow_setting::automatic_placement_toggled(twidget* widget)
{
	ttoggle_button* toggle = dynamic_cast<ttoggle_button*>(widget);
	twindow* window = toggle->get_window();
	if (toggle->get_value()) {
		swap_page(*window, AUTOMATIC_PAGE, true);
	} else {
		swap_page(*window, MANUAL_PAGE, true);
	}
}

void twindow_setting::set_textdomain(twindow& window)
{
	std::stringstream ss;
	std::vector<tval_str> items;

	int index = 0;
	int def = 0;
	for (std::vector<std::string>::const_iterator it = textdomains_.begin(); it != textdomains_.end(); ++ it) {
		ss.str("");
		ss << *it;
		if (*it == cell_.window.textdomain) {
			def = index;
		}
		items.push_back(tval_str(index ++, ss.str()));
	}

	gui2::tcombo_box dlg(items, def);
	dlg.show(disp_.video());

	index = dlg.selected_val();
	cell_.window.textdomain = textdomains_[index];

	set_textdomain_label(window);
}

void twindow_setting::set_textdomain_label(twindow& window)
{
	std::stringstream ss;
	ss << cell_.window.textdomain;

	tbutton* button = find_widget<tbutton>(&window, "textdomain", false, true);
	button->set_label(ss.str());
}

void twindow_setting::set_orientation(twindow& window)
{
	std::stringstream ss;
	std::vector<tval_str> items;

	int def = 0;
	for (std::map<int, tparam3>::const_iterator it = orientations.begin(); it != orientations.end(); ++ it) {
		const tparam3& param = it->second;
		ss.str("");
		ss << param.description;
		if (param.val == cell_.window.orientation) {
			def = param.val;
		}
		items.push_back(tval_str(param.val, ss.str()));
	}

	gui2::tcombo_box dlg(items, def);
	dlg.show(disp_.video());

	cell_.window.orientation = (twidget::torientation)dlg.selected_val();

	set_orientation_label(window);
}

void twindow_setting::set_orientation_label(twindow& window)
{
	const tparam3& param = orientations.find(cell_.window.orientation)->second;

	tbutton* button = find_widget<tbutton>(&window, "orientation", false, true);
	button->set_label(param.description);
}

void twindow_setting::save(twindow& window, bool& handled, bool& halt)
{
	bool ret = true;
	int current_page = (int)reinterpret_cast<long>(bar_->cursel()->cookie());
	if (current_page == BASE_PAGE) {
		ret = save_base(window);
	} else if (current_page == CONTEXT_MENU_PAGE) {
		ret = save_context_menu(window);
	} else if (current_page == PATCH_PAGE) {
		ret = save_patch(window);
	}
	if (!ret) {
		handled = true;
		halt = true;
		return;
	}
	window.set_retval(twindow::OK);
}

void twindow_setting::fill_automatic_page(twindow& window)
{
	// horizontal layout
	set_horizontal_layout_label(window);
	connect_signal_mouse_left_click(
			  find_widget<tbutton>(&window, "_set_horizontal_layout", false)
			, boost::bind(
				&twindow_setting::set_horizontal_layout
				, this
				, boost::ref(window)));

	// vertical layout
	set_vertical_layout_label(window);
	connect_signal_mouse_left_click(
			  find_widget<tbutton>(&window, "_set_vertical_layout", false)
			, boost::bind(
				&twindow_setting::set_vertical_layout
				, this
				, boost::ref(window)));
}

void twindow_setting::fill_manual_page(twindow& window)
{
	ttext_box* text_box = find_widget<ttext_box>(&window, "x", false, true);
	text_box->set_value(cell_.window.x);

	text_box = find_widget<ttext_box>(&window, "y", false, true);
	text_box->set_value(cell_.window.y);

	text_box = find_widget<ttext_box>(&window, "width", false, true);
	text_box->set_value(cell_.window.width);

	text_box = find_widget<ttext_box>(&window, "height", false, true);
	text_box->set_value(cell_.window.height);
}

void twindow_setting::swap_page(twindow& window, int page, bool swap)
{
	if (!layout_panel_) {
		return;
	}
	if (!swap) {
		fill_automatic_page(window);
		fill_manual_page(window);
	}
	layout_panel_->set_radio_layer(page);
}

//
// context menu page
//
void twindow_setting::reload_menu_table(tmenu2& menu, twindow& window, int cursel)
{
	tlistbox* list = find_widget<tlistbox>(&window, "menu", false, true);
	list->clear();

	int index = 0;
	for (std::vector<tmenu2::titem>::const_iterator it = menu.items.begin(); it != menu.items.end(); ++ it) {
		const tmenu2::titem& item = *it;

		string_map list_item;
		std::map<std::string, string_map> list_item_item;

		list_item["label"] = str_cast(index ++);
		list_item_item.insert(std::make_pair("number", list_item));

		list_item["label"] = item.id;
		list_item_item.insert(std::make_pair("id", list_item));

		list_item["label"] = item.submenu? _("Yes"): "-";
		list_item_item.insert(std::make_pair("submenu", list_item));

		list_item["label"] = item.hide? _("Yes"): "-";
		list_item_item.insert(std::make_pair("hide", list_item));

		list_item["label"] = item.param? _("Yes"): "-";
		list_item_item.insert(std::make_pair("param", list_item));

		list->add_row(list_item_item);
	}
	if (cursel >= (int)menu.items.size()) {
		cursel = (int)menu.items.size() - 1;
	}
	if (!menu.items.empty()) {
		list->select_row(cursel);
		map_menu_item_to(window, menu, cursel);
	}

	list->invalidate_layout(true);
}

class tmenu2_item_id_lock
{
public:
	tmenu2_item_id_lock(tmenu2& menu, int n)
		: menu_(menu)
		, n_(n)
		, original_id_(null_str)
	{
		if (n_ >= 0 && n_ < (int)menu_.items.size()) {
			tmenu2::titem& item = menu_.items[n_];
			original_id_ = item.id;
			item.id = null_str;
		}
	}
	~tmenu2_item_id_lock()
	{
		if (n_ >= 0 && n_ < (int)menu_.items.size()) {
			tmenu2::titem& item = menu_.items[n_];
			item.id = original_id_;
		}
	}

private:
	tmenu2& menu_;
	int n_;
	std::string original_id_;
};


std::string twindow_setting::get_menu_item_id(tmenu2& menu, twindow& window, int exclude)
{
	std::string id = find_widget<ttext_box>(&window, "_id", false, true)->label();
	if (!utils::isvalid_id(id, true, min_id_len, max_id_len)) {
		gui2::show_message(disp_.video(), "", utils::errstr);
		return null_str;
	}
	utils::transform_tolower2(id);

	tmenu2_item_id_lock lock(menu, exclude);

	if (menu.top_menu()->id_existed(id)) {
		std::stringstream err;
		err << _("String is using.");
		gui2::show_message(disp_.video(), "", err.str());
		return null_str;
	}
	return id;
}

void twindow_setting::reload_submenu_navigate(tmenu2& menu, twindow& window, const tmenu2* cursel)
{
	submenu_navigate_->erase_children();

	std::vector<tmenu2*> menus;
	menus.push_back(&menu);
	menu.submenus(menus);

	std::stringstream ss;
	int selected = 0;
	for (std::vector<tmenu2*>::iterator it = menus.begin(); it != menus.end(); ++ it) {
		tmenu2* m = *it;
		if (m == cursel) {
			selected = std::distance(menus.begin(), it);
		}
		tcontrol* widget = submenu_navigate_->create_child(null_str, null_str, NULL);
		widget->set_label(m->id);
		widget->set_cookie(reinterpret_cast<void*>(m));
		submenu_navigate_->insert_child(*widget);
	}
	submenu_navigate_->select(selected);
	submenu_navigate_->replacement_children();
	refresh_parent_desc(window, *menus[selected]);
}

tmenu2* twindow_setting::current_submenu() const
{
	tcontrol* widget = submenu_navigate_->cursel();
	return reinterpret_cast<tmenu2*>(widget->cookie());
}

bool twindow_setting::menu_pre_toggle_tabbar(twidget* widget, twidget* previous)
{
	return true;
}

void twindow_setting::menu_toggle_tabbar(twidget* widget)
{
	twindow* window = widget->get_window();
	tmenu2* menu = reinterpret_cast<tmenu2*>(widget->cookie());
	tdialog::toggle_report(widget);
}

bool twindow_setting::submenu_pre_toggle_tabbar(twidget* widget, twidget* previous)
{
	return true;
}

void twindow_setting::submenu_toggle_tabbar(twidget* widget)
{
	twindow* window = widget->get_window();
	tmenu2* menu = reinterpret_cast<tmenu2*>(widget->cookie());
	tdialog::toggle_report(widget);

	refresh_parent_desc(*window, *menu);
	reload_menu_table(*menu, *window, 0);
}

void twindow_setting::append_menu_item(twindow& window)
{
	tmenu2& menu = *current_submenu();

	std::string id = get_menu_item_id(menu, window, -1);
	if (id.empty()) {
		return;
	}
	
	tmenu2* submenu = NULL;
	if (find_widget<ttoggle_button>(&window, "_submenu", false, true)->get_value()) {
		submenu = new tmenu2(null_str, id, &menu);
	}
	bool hide = find_widget<ttoggle_button>(&window, "_hide", false, true)->get_value();
	bool param = find_widget<ttoggle_button>(&window, "_param", false, true)->get_value();
	menu.items.push_back(tmenu2::titem(id, submenu, hide, param));

	if (submenu) {
		reload_submenu_navigate(menus_[0], window, current_submenu());
	}

	reload_menu_table(menu, window, menus_[0].items.size() - 1);
	// find_widget<tlistbox>(&window, "menu", false).scroll_vertical_scrollbar(tscrollbar_::END);
}

void twindow_setting::modify_menu_item(twindow& window)
{
	tmenu2& menu = *current_submenu();
	tlistbox& list = find_widget<tlistbox>(&window, "menu", false);
	int row = list.get_selected_row();

	ttext_box* text_box = find_widget<ttext_box>(&window, "_id", false, true);
	std::string id = get_menu_item_id(menu, window, row);
	if (id.empty()) {
		return;
	}
	bool submenu = find_widget<ttoggle_button>(&window, "_submenu", false, true)->get_value();
	bool hide = find_widget<ttoggle_button>(&window, "_hide", false, true)->get_value();
	bool param = find_widget<ttoggle_button>(&window, "_param", false, true)->get_value();

	tmenu2::titem& item = menu.items[row];
	if (id == item.id && submenu == !!item.submenu && hide == item.hide && param == item.param) {
		return;
	}

	bool require_navigate = false;
	if (submenu != !!item.submenu) {
		if (item.submenu) {
			delete item.submenu;
			item.submenu = NULL;
		} else {
			item.submenu = new tmenu2(null_str, id, &menu);
		}
		require_navigate = true;
	}
	item.id = id;
	item.hide = hide;
	item.param = param;

	if (require_navigate) {
		reload_submenu_navigate(*menu.top_menu(), window, &menu);
	}
	reload_menu_table(menu, window, row);
}

void twindow_setting::erase_menu_item(twindow& window)
{
	tmenu2& menu = *current_submenu();
	tlistbox& list = find_widget<tlistbox>(&window, "menu", false);
	int row = list.get_selected_row();

	std::vector<tmenu2::titem>::iterator it = menu.items.begin();
	std::advance(it, row);
	bool require_navigate = it->submenu;
	menu.items.erase(it);

	if (require_navigate) {
		reload_submenu_navigate(*menu.top_menu(), window, &menu);
	}
	reload_menu_table(menu, window, row);
}

void twindow_setting::item_selected(twindow& window, tlistbox& list, const int type)
{
	tmenu2& menu = *current_submenu();
	int row = list.get_selected_row();
	map_menu_item_to(window, menu, row);
}

void twindow_setting::map_menu_item_to(twindow& window, tmenu2& menu, int row)
{
	tmenu2::titem& item = menu.items[row];

	find_widget<ttext_box>(&window, "_id", false, true)->set_value(item.id);
	find_widget<ttoggle_button>(&window, "_submenu", false, true)->set_value(item.submenu);
	find_widget<ttoggle_button>(&window, "_hide", false, true)->set_value(item.hide);
	find_widget<ttoggle_button>(&window, "_param", false, true)->set_value(item.param);
}

void twindow_setting::refresh_parent_desc(twindow& window, tmenu2& menu)
{
	tgrid& grid = find_widget<tgrid>(&window, "_grid_set_report", false);
	tlabel& label = find_widget<tlabel>(&window, "parent_menu", false);

	if (!menu.parent) {
		grid.set_visible(twidget::VISIBLE);
		label.set_visible(twidget::INVISIBLE);
		find_widget<ttext_box>(&window, "report_id", false).set_value(menu.report);

	} else {
		grid.set_visible(twidget::INVISIBLE);
		label.set_visible(twidget::VISIBLE);

		std::stringstream ss;
		utils::string_map symbols;
		symbols["menu"] = tintegrate::generate_format(menu.parent->id, "green");

		std::string msg = vgettext2("Parent menu $menu", symbols);
		label.set_label(msg);
	}
}

//
// patch page
//
void twindow_setting::switch_patch_cfg(twindow& window)
{
	fill_change_list(window);
	fill_remove_list(window);

	bool custom_patch = patch_current_tab_ >= tmode::res_count;
	find_widget<tbutton>(&window, "_erase_patch", false).set_active(custom_patch);
	find_widget<tbutton>(&window, "_rename_patch", false).set_active(custom_patch);
}

void twindow_setting::append_patch(twindow& window)
{
	tmode_navigate::append_patch(*patch_bar_, window);
}

void twindow_setting::erase_patch(twindow& window)
{
	tmode_navigate::erase_patch(*patch_bar_, window, patch_current_tab_);

	patch_current_tab_ = 1;
	patch_bar_->select(patch_current_tab_);
	switch_patch_cfg(window);
}

void twindow_setting::rename_patch(twindow& window)
{
	tmode_navigate::rename_patch(*patch_bar_, window, patch_current_tab_);
}

void twindow_setting::fill_change_list2(tlistbox& list, const tmode& mode, const unit::tchild& child)
{
	std::stringstream ss;
	for (std::vector<unit*>::const_iterator it = child.units.begin(); it != child.units.end(); ++ it) {
		unit* u = *it;
		tadjust adjust = u->adjust(mode, tadjust::CHANGE);
		
		if (adjust.valid()) {
			bool rect_changed = adjust.different_change_cfg(u->get_base_change_cfg(mode, true, u->cell().rect_cfg), t_true);
			bool misc_changed = adjust.different_change_cfg(u->get_base_change_cfg(mode, false, u->generate_change2_cfg()), t_false);
			if (rect_changed || misc_changed) {
				string_map list_item;
				std::map<std::string, string_map> list_item_item;

				ss.str("");
				ss << (list.get_item_count() + 1);
				list_item["label"] = ss.str();
				list_item_item.insert(std::make_pair("number", list_item));

				ss.str("");
				ss << u->cell().id;
				list_item["label"] = ss.str();
				list_item_item.insert(std::make_pair("id", list_item));

				list_item["label"] = rect_changed? _("Yes"): null_str;
				list_item_item.insert(std::make_pair("rect", list_item));

				list_item["label"] = misc_changed? _("Yes"): null_str;
				list_item_item.insert(std::make_pair("misc", list_item));

				list.add_row(list_item_item);
			}
		}

		const std::vector<unit::tchild>& children = u->children();
		for (std::vector<unit::tchild>::const_iterator it2 = children.begin(); it2 != children.end(); ++ it2) {
			fill_change_list2(list, mode, *it2); 
		}

	}
}

void twindow_setting::fill_change_list(twindow& window)
{
	std::stringstream ss;
	tlistbox& list = find_widget<tlistbox>(&window, "change_list", false);

	list.clear();
	const tmode& mode = controller_.mode(patch_current_tab_);

	mkwin_controller::ttheme_top_lock lock(controller_);
	const unit::tchild& top = controller_.top();

	fill_change_list2(list, mode, top);
	list.invalidate_layout(true);
}

int twindow_setting::calculate_change_count(const unit::tchild& child, int at) const
{
	int result = 0;
	const tmode& mode = controller_.mode(at);
	for (std::vector<unit*>::const_iterator it = child.units.begin(); it != child.units.end(); ++ it) {
		unit* u = *it;
		tadjust adjust = u->adjust(mode, tadjust::CHANGE);
		
		if (adjust.valid()) {
			config cfg = u->get_base_change_cfg(mode, true, u->cell().rect_cfg);
			cfg.merge_attributes(u->get_base_change_cfg(mode, false, u->generate_change2_cfg()));
			if (adjust.different_change_cfg(cfg, t_unset)) {
				result ++;
			}
		}

		const std::vector<unit::tchild>& children = u->children();
		for (std::vector<unit::tchild>::const_iterator it2 = children.begin(); it2 != children.end(); ++ it2) {
			result += calculate_change_count(*it2, at); 
		}
	}
	return result;
}

void twindow_setting::fill_remove_list2(tlistbox& list, const tmode& mode, const unit::tchild& child)
{
	std::stringstream ss;
	for (std::vector<unit*>::const_iterator it = child.units.begin(); it != child.units.end(); ++ it) {
		unit* u = *it;
		tadjust adjust = u->adjust(mode, tadjust::REMOVE);
		
		bool remove = u->get_base_remove(mode);
		if (!remove && adjust.valid()) {
			string_map list_item;
			std::map<std::string, string_map> list_item_item;

			ss.str("");
			ss << (list.get_item_count() + 1);
			list_item["label"] = ss.str();
			list_item_item.insert(std::make_pair("number", list_item));

			ss.str("");
			ss << u->cell().id;
			list_item["label"] = ss.str();
			list_item_item.insert(std::make_pair("id", list_item));

			list_item["label"] = _("Unit");
			list_item_item.insert(std::make_pair("type", list_item));

			list.add_row(list_item_item);
		}

		const std::vector<unit::tchild>& children = u->children();
		for (std::vector<unit::tchild>::const_iterator it2 = children.begin(); it2 != children.end(); ++ it2) {
			fill_remove_list2(list, mode, *it2); 
		}

	}
}

void twindow_setting::fill_remove2_list2(tlistbox& list, const tmode& mode, const unit::tchild& child)
{
	std::stringstream ss;
	for (std::vector<unit*>::const_iterator it = child.units.begin(); it != child.units.end(); ++ it) {
		unit* u = *it;
		tadjust adjust = u->adjust(mode, tadjust::REMOVE2);
		if (adjust.valid()) {
			config cfg = u->get_base_remove2_cfg(mode);
			std::set<int> ret = adjust.newed_remove2_cfg(cfg);
			for (std::set<int>::const_iterator it2 = ret.begin(); it2 != ret.end(); ++ it2) {
				string_map list_item;
				std::map<std::string, string_map> list_item_item;

				ss.str("");
				ss << (list.get_item_count() + 1);
				list_item["label"] = ss.str();
				list_item_item.insert(std::make_pair("number", list_item));

				ss.str("");
				ss << u->child(*it2).window->cell().id;
				list_item["label"] = ss.str();
				list_item_item.insert(std::make_pair("id", list_item));

				list_item["label"] = _("Layer");
				list_item_item.insert(std::make_pair("type", list_item));

				list.add_row(list_item_item);
			}
		}

		const std::vector<unit::tchild>& children = u->children();
		for (std::vector<unit::tchild>::const_iterator it2 = children.begin(); it2 != children.end(); ++ it2) {
			fill_remove2_list2(list, mode, *it2); 
		}

	}
}

void twindow_setting::fill_remove_list(twindow& window)
{
	std::stringstream ss;
	tlistbox& list = find_widget<tlistbox>(&window, "remove_list", false);

	list.clear();
	const tmode& mode = controller_.mode(patch_current_tab_);
	mkwin_controller::ttheme_top_lock lock(controller_);
	const unit::tchild& top = controller_.top();

	fill_remove_list2(list, mode, top);
	fill_remove2_list2(list, mode, top);
	list.invalidate_layout(true);
}

int twindow_setting::calculate_remove_count(const unit::tchild& child, int at) const
{
	int result = 0;
	const tmode& mode = controller_.mode(at);
	for (std::vector<unit*>::const_iterator it = child.units.begin(); it != child.units.end(); ++ it) {
		unit* u = *it;
		tadjust adjust = u->adjust(mode, tadjust::REMOVE);
		
		bool remove = u->get_base_remove(mode);
		if (!remove && adjust.valid()) {
			result ++;
		}

		const std::vector<unit::tchild>& children = u->children();
		for (std::vector<unit::tchild>::const_iterator it2 = children.begin(); it2 != children.end(); ++ it2) {
			result += calculate_remove_count(*it2, at);
		}
	}
	return result;
}

int twindow_setting::calculate_remove2_count(const unit::tchild& child, int at) const
{
	int result = 0;
	const tmode& mode = controller_.mode(at);
	for (std::vector<unit*>::const_iterator it = child.units.begin(); it != child.units.end(); ++ it) {
		unit* u = *it;
		tadjust adjust = u->adjust(mode, tadjust::REMOVE2);
		
		if (adjust.valid()) {
			config cfg = u->get_base_remove2_cfg(mode);
			std::set<int> ret = adjust.newed_remove2_cfg(cfg);
			result += (int)ret.size();
		}

		const std::vector<unit::tchild>& children = u->children();
		for (std::vector<unit::tchild>::const_iterator it2 = children.begin(); it2 != children.end(); ++ it2) {
			result += calculate_remove2_count(*it2, at);
		}
	}
	return result;
}

void twindow_setting::patch_toggle_tabbar(twidget* widget)
{
	patch_current_tab_ = (int)reinterpret_cast<long>(widget->cookie());
	tdialog::toggle_report(widget);

	twindow* window = widget->get_window();
	switch_patch_cfg(*window);
}

std::string twindow_setting::form_tab_label(treport& navigate, int at) const
{
	const tmode& mode = controller_.mode(at);

	std::stringstream ss;
	ss << mode.id;

	if (!at) {
		return ss.str();
	}

	mkwin_controller::ttheme_top_lock lock(controller_);
	const unit::tchild& top = controller_.top();

	int changes = calculate_change_count(top, at);
	int removes = calculate_remove_count(top, at);
	removes += calculate_remove2_count(top, at);
	if (changes || removes) {
		ss << "(";
		if (changes) {
			ss << tintegrate::generate_format(changes, "red");
		}
		ss << ",";
		if (removes) {
			ss << tintegrate::generate_format(removes, "red");
		}
		ss << ")";
	}
	ss << "\n";
	ss << mode.res.width << "x" << mode.res.height;

	return ss.str();
}

}
