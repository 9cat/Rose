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

#include "gui/dialogs/control_setting.hpp"

#include "display.hpp"
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
#include "gui/widgets/report.hpp"
#include "gui/dialogs/combo_box.hpp"
#include "gui/dialogs/message.hpp"
#include "gui/dialogs/theme.hpp"
#include "gui/auxiliary/window_builder.hpp"
#include "unit.hpp"
#include "mkwin_controller.hpp"
#include <cctype>

#include <boost/bind.hpp>

namespace gui2 {

REGISTER_DIALOG(control_setting)

const std::string untitled = "_untitled";

void show_id_error(display& disp, const std::string& id, const std::string& errstr)
{
	std::stringstream err;
	utils::string_map symbols;

	symbols["id"] = tintegrate::generate_format(id, "red");
	err << vgettext2("Invalid '$id' value!", symbols);
	err << "\n\n" << errstr;
	gui2::show_message(disp.video(), "", err.str());
}

std::map<int, tlayout> horizontal_layout;
std::map<int, tlayout> vertical_layout;

std::map<int, tscroll_mode> horizontal_mode;
std::map<int, tscroll_mode> vertical_mode;

tanchor::tanchor(int val, const std::string& description, bool horizontal)
	: val(val)
	, id()
	, description(description)
{
	if (val == theme::TOP_ANCHORED) {
		if (horizontal) {
			id = "left";
		} else {
			id = "top";
		}
	} else if (val == theme::BOTTOM_ANCHORED) {
		if (horizontal) {
			id = "right";
		} else {
			id = "bottom";
		}
	} else if (val == theme::FIXED) {
		id = "fixed"; // default
	}
}
std::map<int, tanchor> horizontal_anchor;
std::map<int, tanchor> vertical_anchor;

std::map<int, tparam3> orientations;

void init_layout_mode()
{
	if (horizontal_layout.empty()) {
		horizontal_layout.insert(std::make_pair(tgrid::HORIZONTAL_GROW_SEND_TO_CLIENT, 
			tlayout(tgrid::HORIZONTAL_GROW_SEND_TO_CLIENT, _("Align left. Maybe stretch"))));
		horizontal_layout.insert(std::make_pair(tgrid::HORIZONTAL_ALIGN_LEFT, 
			tlayout(tgrid::HORIZONTAL_ALIGN_LEFT, _("Align left"))));
		horizontal_layout.insert(std::make_pair(tgrid::HORIZONTAL_ALIGN_CENTER, 
			tlayout(tgrid::HORIZONTAL_ALIGN_CENTER, _("Align center"))));
		horizontal_layout.insert(std::make_pair(tgrid::HORIZONTAL_ALIGN_RIGHT, 
			tlayout(tgrid::HORIZONTAL_ALIGN_RIGHT, _("Align right"))));
	}

	if (vertical_layout.empty()) {
		vertical_layout.insert(std::make_pair(tgrid::VERTICAL_GROW_SEND_TO_CLIENT, 
			tlayout(tgrid::VERTICAL_GROW_SEND_TO_CLIENT, _("Align top. Maybe stretch"))));
		vertical_layout.insert(std::make_pair(tgrid::VERTICAL_ALIGN_TOP, 
			tlayout(tgrid::VERTICAL_ALIGN_TOP, _("Align top"))));
		vertical_layout.insert(std::make_pair(tgrid::VERTICAL_ALIGN_CENTER, 
			tlayout(tgrid::VERTICAL_ALIGN_CENTER, _("Align center"))));
		vertical_layout.insert(std::make_pair(tgrid::VERTICAL_ALIGN_BOTTOM, 
			tlayout(tgrid::VERTICAL_ALIGN_BOTTOM, _("Align bottom"))));
	}

	if (horizontal_mode.empty()) {
		horizontal_mode.insert(std::make_pair(tscrollbar_container::always_invisible, 
			tscroll_mode(tscrollbar_container::always_invisible, _("Always invisible"))));
		horizontal_mode.insert(std::make_pair(tscrollbar_container::auto_visible, 
			tscroll_mode(tscrollbar_container::auto_visible, _("Auto visible"))));
	}

	if (vertical_mode.empty()) {
		vertical_mode.insert(std::make_pair(tscrollbar_container::always_invisible, 
			tscroll_mode(tscrollbar_container::always_invisible, _("Always invisible"))));
		vertical_mode.insert(std::make_pair(tscrollbar_container::auto_visible, 
			tscroll_mode(tscrollbar_container::auto_visible, _("Auto visible"))));
	}

	std::stringstream err;
	utils::string_map symbols;
	
	if (horizontal_anchor.empty()) {
		symbols["field1"] = tintegrate::generate_format("x1", "red");
		symbols["field2"] = tintegrate::generate_format(_("Width"), "red");
		horizontal_anchor.insert(std::make_pair(theme::FIXED, tanchor(theme::FIXED, vgettext2("$field1 fixed, $field2 fixed", symbols), true)));
		horizontal_anchor.insert(std::make_pair(theme::TOP_ANCHORED, tanchor(theme::TOP_ANCHORED, vgettext2("$field1 fixed, $field2 scalable", symbols), true)));
		horizontal_anchor.insert(std::make_pair(theme::BOTTOM_ANCHORED, tanchor(theme::BOTTOM_ANCHORED, vgettext2("$field1 scalable, $field2 fixed", symbols), true)));
	}

	if (vertical_anchor.empty()) {
		symbols["field1"] = tintegrate::generate_format("y1", "red");
		symbols["field2"] = tintegrate::generate_format(_("Height"), "red");
		vertical_anchor.insert(std::make_pair(theme::FIXED, tanchor(theme::FIXED, vgettext2("$field1 fixed, $field2 fixed", symbols), false)));
		vertical_anchor.insert(std::make_pair(theme::TOP_ANCHORED, tanchor(theme::TOP_ANCHORED, vgettext2("$field1 fixed, $field2 scalable", symbols), false)));
		vertical_anchor.insert(std::make_pair(theme::BOTTOM_ANCHORED, tanchor(theme::BOTTOM_ANCHORED, vgettext2("$field1 scalable, $field2 fixed", symbols), false)));
	}

	if (orientations.empty()) {
		orientations.insert(std::make_pair(twidget::auto_orientation, tparam3(twidget::auto_orientation, "auto", _("Auto"))));
		orientations.insert(std::make_pair(twidget::landscape_orientation, tparam3(twidget::landscape_orientation, "landscape", _("Landscape"))));
		orientations.insert(std::make_pair(twidget::portrait_orientation, tparam3(twidget::portrait_orientation, "portrait", _("Portrait"))));
	}
}

tcontrol_setting::tcontrol_setting(display& disp, mkwin_controller& controller, unit& u, const std::vector<std::string>& textdomains, const std::vector<tlinked_group>& linkeds)
	: tsetting_dialog(u.cell())
	, tmode_navigate(controller, disp)
	, disp_(disp)
	, controller_(controller)
	, u_(u)
	, textdomains_(textdomains)
	, linkeds_(linkeds)
	, bar_(NULL)
	, rect_navigate_(NULL)
	, rect_current_tab_(0)
	, advanced_navigate_(NULL)
	, advanced_current_tab_(0)
	, must_use_rect_(controller.in_theme_top())
	, current_rect_cfg_()
	, current_advanced_cfg_()
{
}

void tcontrol_setting::update_title(twindow& window)
{
	std::stringstream ss;
	const std::pair<std::string, gui2::tcontrol_definition_ptr>& widget = u_.widget();

	ss.str("");
	if (!u_.cell().id.empty()) {
		ss << tintegrate::generate_format(u_.cell().id, "green"); 
	} else {
		ss << tintegrate::generate_format("---", "green"); 
	}
	ss << "    ";

	ss << widget.first;
	if (widget.second.get()) {
		ss << "(" << tintegrate::generate_format(widget.second->id, "blue") << ")"; 
	}
	tlabel* label = find_widget<tlabel>(&window, "title", false, true);
	label->set_label(ss.str());
}

void tcontrol_setting::pre_show(CVideo& /*video*/, twindow& window)
{
	window.set_canvas_variable("border", variant("default-border"));

	update_title(window);

	// prepare navigate bar.
	std::vector<std::string> labels;
	labels.push_back(_("Base"));
	labels.push_back(_("Rectangle"));
	labels.push_back(_("Size"));
	labels.push_back(_("Advanced"));

	bar_ = find_widget<treport>(&window, "bar", false, true);
	bar_->tabbar_init(true, "tab");
	bar_->set_boddy(find_widget<twidget>(&window, "bar_panel", false, true));
	int index = 0;
	for (std::vector<std::string>::const_iterator it = labels.begin(); it != labels.end(); ++ it) {
		tcontrol* widget = bar_->create_child(null_str, null_str, reinterpret_cast<void*>(index ++));
		widget->set_label(*it);
		bar_->insert_child(*widget);
	}
	bar_->select(BASE_PAGE);
	bar_->replacement_children();

	page_panel_ = find_widget<tstacked_widget>(&window, "panel", false, true);
	page_panel_->set_radio_layer(BASE_PAGE);
	
	pre_base(window);

	if (controller_.theme()) {
		pre_rect(window);
	} else {
		bar_->set_child_visible(RECT_PAGE, false);
		page_panel_->layer(RECT_PAGE)->set_visible(twidget::INVISIBLE);
	}

	if (u_.has_size()) {
		pre_size(window);
	} else {
		bar_->set_child_visible(SIZE_PAGE, false);
		page_panel_->layer(SIZE_PAGE)->set_visible(twidget::INVISIBLE);
	}

	if (u_.widget().second.get()) {
		pre_advanced(window);
	} else {
		bar_->set_child_visible(ADVANCED_PAGE, false);
		page_panel_->layer(ADVANCED_PAGE)->set_visible(twidget::INVISIBLE);
	}

	connect_signal_mouse_left_click(
		 find_widget<tbutton>(&window, "save", false)
		, boost::bind(
			&tcontrol_setting::save
			, this
			, boost::ref(window)
			, _3, _4));
}

void tcontrol_setting::pre_base(twindow& window)
{
	// horizontal layout
	set_horizontal_layout_label(window);
	connect_signal_mouse_left_click(
			  find_widget<tbutton>(&window, "_set_horizontal_layout", false)
			, boost::bind(
				&tcontrol_setting::set_horizontal_layout
				, this
				, boost::ref(window)));

	// vertical layout
	set_vertical_layout_label(window);
	connect_signal_mouse_left_click(
			  find_widget<tbutton>(&window, "_set_vertical_layout", false)
			, boost::bind(
				&tcontrol_setting::set_vertical_layout
				, this
				, boost::ref(window)));

	// linked_group
	set_linked_group_label(window);
	connect_signal_mouse_left_click(
			  find_widget<tbutton>(&window, "_set_linked_group", false)
			, boost::bind(
				&tcontrol_setting::set_linked_group
				, this
				, boost::ref(window)));

	// linked_group
	set_textdomain_label(window, true);
	set_textdomain_label(window, false);
	connect_signal_mouse_left_click(
			  find_widget<tbutton>(&window, "textdomain_label", false)
			, boost::bind(
				&tcontrol_setting::set_textdomain
				, this
				, boost::ref(window)
				, true));
	connect_signal_mouse_left_click(
			  find_widget<tbutton>(&window, "textdomain_tooltip", false)
			, boost::bind(
				&tcontrol_setting::set_textdomain
				, this
				, boost::ref(window)
				, false));

	// border size
	ttext_box* text_box = find_widget<ttext_box>(&window, "_border", false, true);
	text_box->set_label(str_cast(cell_.widget.cell.border_size_));
	if (cell_.widget.cell.flags_ & tgrid::BORDER_LEFT) {
		find_widget<ttoggle_button>(&window, "_border_left", false, true)->set_value(true);
	}
	if (cell_.widget.cell.flags_ & tgrid::BORDER_RIGHT) {
		find_widget<ttoggle_button>(&window, "_border_right", false, true)->set_value(true);
	}
	if (cell_.widget.cell.flags_ & tgrid::BORDER_TOP) {
		find_widget<ttoggle_button>(&window, "_border_top", false, true)->set_value(true);
	}
	if (cell_.widget.cell.flags_ & tgrid::BORDER_BOTTOM) {
		find_widget<ttoggle_button>(&window, "_border_bottom", false, true)->set_value(true);
	}

	text_box = find_widget<ttext_box>(&window, "_id", false, true);
	text_box->set_maximum_length(max_id_len);
	text_box->set_label(cell_.id);

	find_widget<tscroll_text_box>(&window, "_label", false, true)->set_label(cell_.widget.label);
	find_widget<tscroll_text_box>(&window, "_tooltip", false, true)->set_label(cell_.widget.tooltip);

	if (controller_.in_theme_top()) {
		find_widget<tbutton>(&window, "_set_horizontal_layout", false).set_active(false);
		find_widget<tbutton>(&window, "_set_vertical_layout", false).set_active(false);
		find_widget<ttext_box>(&window, "_border", false).set_active(false);
		find_widget<ttoggle_button>(&window, "_border_left", false).set_active(false);
		find_widget<ttoggle_button>(&window, "_border_right", false).set_active(false);
		find_widget<ttoggle_button>(&window, "_border_top", false).set_active(false);
		find_widget<ttoggle_button>(&window, "_border_bottom", false).set_active(false);
		find_widget<tbutton>(&window, "_set_linked_group", false).set_active(false);

		bool readonly_id = u_.is_main_map();
		find_widget<ttext_box>(&window, "_id", false).set_active(!readonly_id);

		find_widget<tbutton>(&window, "textdomain_label", false).set_active(false);
		find_widget<tscroll_text_box>(&window, "_label", false).set_active(false);
		find_widget<tbutton>(&window, "textdomain_tooltip", false).set_active(false);
		find_widget<tscroll_text_box>(&window, "_tooltip", false).set_active(false);

	}
}

void tcontrol_setting::pre_rect(twindow& window)
{
	std::stringstream err;
	utils::string_map symbols;

	std::vector<std::string> vstr;
	vstr.push_back(_("Absolute"));
	symbols["field"] = tintegrate::generate_format("x1", "red");
	vstr.push_back(vgettext2("ref's $field", symbols));
	symbols["field"] = tintegrate::generate_format("x2", "red");
	vstr.push_back(vgettext2("ref's $field", symbols));
	candidate_rect_coor_ops_.insert(std::make_pair(0, vstr));

	vstr.clear();
	vstr.push_back(_("Absolute"));
	symbols["field"] = tintegrate::generate_format("y1", "red");
	vstr.push_back(vgettext2("ref's $field", symbols));
	symbols["field"] = tintegrate::generate_format("y2", "red");
	vstr.push_back(vgettext2("ref's $field", symbols));
	candidate_rect_coor_ops_.insert(std::make_pair(1, vstr));

	vstr.clear();
	vstr.push_back(_("Absolute"));
	symbols["field"] = tintegrate::generate_format("x2", "red");
	vstr.push_back(vgettext2("ref's $field", symbols));
	symbols["field"] = tintegrate::generate_format("x1", "red");
	vstr.push_back(vgettext2("My $field", symbols));
	candidate_rect_coor_ops_.insert(std::make_pair(2, vstr));

	vstr.clear();
	vstr.push_back(_("Absolute"));
	symbols["field"] = tintegrate::generate_format("y2", "red");
	vstr.push_back(vgettext2("ref's $field", symbols));
	symbols["field"] = tintegrate::generate_format("y1", "red");
	vstr.push_back(vgettext2("My $field", symbols));
	candidate_rect_coor_ops_.insert(std::make_pair(3, vstr));

	rect_coor_ops_.resize(4);

	// require_show_flag require rect_navigate_ valid.
	rect_navigate_ = find_widget<treport>(&window, "rect_navigate", false, true);
	tmode_navigate::pre_show(window, "rect_navigate");
	rect_navigate_->set_boddy(find_widget<twidget>(&window, "rect_panel", false, true));

	bool toggled = must_use_rect_ || u_.fix_rect();
	ttoggle_button* toggle = find_widget<ttoggle_button>(&window, "rect", false, true);
	toggle->set_value(toggled);
	if (must_use_rect_) {
		toggle->set_active(false);
	}
	toggle->set_callback_state_change(boost::bind(&tcontrol_setting::rect_toggled, this, _1));
	if (!toggled) {
		rect_toggled(toggle);
	}

	toggle = find_widget<ttoggle_button>(&window, "custom_rect", false, true);
	toggle->set_visible(twidget::INVISIBLE);
	toggle->set_callback_state_change(boost::bind(&tcontrol_setting::custom_rect_toggled, this, _1));

	connect_signal_mouse_left_click(
			  find_widget<tbutton>(&window, "set_ref", false)
			, boost::bind(
				&tcontrol_setting::set_ref
				, this
				, boost::ref(window)));

	std::vector<std::string> set_coors;
	set_coors.push_back("set_x1");
	set_coors.push_back("set_y1");
	set_coors.push_back("set_x2");
	set_coors.push_back("set_y2");
	for (int n = 0; n < (int)set_coors.size(); n ++) {
		connect_signal_mouse_left_click(
			  find_widget<tbutton>(&window, set_coors[n], false)
			, boost::bind(
				&tcontrol_setting::set_rect_coor
				, this
				, boost::ref(window)
				, n));
	}

	connect_signal_mouse_left_click(
			  find_widget<tbutton>(&window, "set_xanchor", false)
			, boost::bind(
				&tcontrol_setting::set_xanchor
				, this
				, boost::ref(window)));

	connect_signal_mouse_left_click(
			  find_widget<tbutton>(&window, "set_yanchor", false)
			, boost::bind(
				&tcontrol_setting::set_yanchor
				, this
				, boost::ref(window)));

	VALIDATE(cell_.rect_cfg.empty() || tadjust::cfg_is_rect_fields(cell_.rect_cfg), null_str);
	current_rect_cfg_ = cell_.rect_cfg;
	switch_rect_cfg(window, current_rect_cfg_);
}

void tcontrol_setting::pre_size(twindow& window)
{
	find_widget<tscroll_text_box>(&window, "_width", false, true)->set_label(cell_.widget.width);
	find_widget<tscroll_text_box>(&window, "_height", false, true)->set_label(cell_.widget.height);
}

void tcontrol_setting::pre_advanced(twindow& window)
{
	advanced_navigate_ = tmode_navigate::pre_show(window, "advanced_navigate");
	advanced_navigate_->set_boddy(find_widget<twidget>(&window, "advanced_panel", false, true));
	if (!controller_.theme()) {
		advanced_navigate_->set_visible(twidget::INVISIBLE);

		ttoggle_button* remove = find_widget<ttoggle_button>(&window, "remove", false, true);
		remove->set_visible(twidget::INVISIBLE);
	}

	std::stringstream ss;
	const std::pair<std::string, gui2::tcontrol_definition_ptr>& widget = u_.widget();

	tlabel* label;
	ttext_box* text_box;
	if (!u_.is_tree_view()) {
		tgrid* grid = find_widget<tgrid>(&window, "_grid_tree_view", false, true);
		grid->set_visible(twidget::INVISIBLE);

	} else {
		text_box = find_widget<ttext_box>(&window, "indention_step_size", false, true);
		text_box->set_label(str_cast(cell_.widget.tree_view.indention_step_size));
		text_box = find_widget<ttext_box>(&window, "node_id", false, true);
		text_box->set_label(cell_.widget.tree_view.node_id);
	}

	if (!u_.is_slider()) {
		tgrid* grid = find_widget<tgrid>(&window, "_grid_slider", false, true);
		grid->set_visible(twidget::INVISIBLE);

	} else {
		text_box = find_widget<ttext_box>(&window, "minimum_value", false, true);
		text_box->set_label(str_cast(cell_.widget.slider.minimum_value));
		text_box = find_widget<ttext_box>(&window, "maximum_value", false, true);
		text_box->set_label(str_cast(cell_.widget.slider.maximum_value));
		text_box = find_widget<ttext_box>(&window, "step_size", false, true);
		text_box->set_label(str_cast(cell_.widget.slider.step_size));
	}

	if (!u_.is_text_box2()) {
		tgrid* grid = find_widget<tgrid>(&window, "_grid_text_box2", false, true);
		grid->set_visible(twidget::INVISIBLE);

	} else {
		connect_signal_mouse_left_click(
			  find_widget<tbutton>(&window, "set_text_box2_text_box", false)
			, boost::bind(
				&tcontrol_setting::set_text_box2_text_box
				, this
				, boost::ref(window)));
		label = find_widget<tlabel>(&window, "text_box2_text_box", false, true);
		label->set_label(cell_.widget.text_box2.text_box.empty()? "default": cell_.widget.text_box2.text_box);
	}

	if (!u_.is_report()) {
		tgrid* grid = find_widget<tgrid>(&window, "_grid_report", false, true);
		grid->set_visible(twidget::INVISIBLE);

	} else {
		connect_signal_mouse_left_click(
			  find_widget<tbutton>(&window, "multi_line", false)
			, boost::bind(
				&tcontrol_setting::set_multi_line
				, this
				, boost::ref(window)));
	}

	if (u_.is_scroll()) {
		// horizontal mode
		set_horizontal_mode_label(window);
		connect_signal_mouse_left_click(
				  find_widget<tbutton>(&window, "_set_horizontal_mode", false)
				, boost::bind(
					&tcontrol_setting::set_horizontal_mode
					, this
					, boost::ref(window)));

		// vertical layout
		set_vertical_mode_label(window);
		connect_signal_mouse_left_click(
				  find_widget<tbutton>(&window, "_set_vertical_mode", false)
				, boost::bind(
					&tcontrol_setting::set_vertical_mode
					, this
					, boost::ref(window)));
	} else {
		find_widget<tgrid>(&window, "_grid_scrollbar", false).set_visible(twidget::INVISIBLE);
	}

	if (u_.has_drag()) {
		ttoggle_button* toggle = find_widget<ttoggle_button>(&window, "_drag_left", false, true);
		toggle->set_value(cell_.widget.drag & twidget::drag_left);
		toggle = find_widget<ttoggle_button>(&window, "_drag_right", false, true);
		toggle->set_value(cell_.widget.drag & twidget::drag_right);
		toggle = find_widget<ttoggle_button>(&window, "_drag_up", false, true);
		toggle->set_value(cell_.widget.drag & twidget::drag_up);
		toggle = find_widget<ttoggle_button>(&window, "_drag_down", false, true);
		toggle->set_value(cell_.widget.drag & twidget::drag_down);

	} else {
		find_widget<tgrid>(&window, "_grid_drag", false).set_visible(twidget::INVISIBLE);
	}

	connect_signal_mouse_left_click(
			  find_widget<tbutton>(&window, "_set_definition", false)
			, boost::bind(
				&tcontrol_setting::set_definition
				, this
				, boost::ref(window)));

	current_advanced_cfg_ = u_.generate2_change2_cfg(cell_);
	switch_advanced_cfg(window, current_advanced_cfg_, true);
}

bool tcontrol_setting::pre_toggle_tabbar(twidget* widget, twidget* previous)
{
	treport* report = treport::get_report(widget);
	if (report == rect_navigate_) {
		return rect_pre_toggle_tabbar(widget, previous);

	} else if (report == advanced_navigate_) {
		return advanced_pre_toggle_tabbar(widget, previous);
	} 

	twindow& window = *widget->get_window();
	bool ret = true;
	int previous_page = (int)reinterpret_cast<long>(previous->cookie());
	if (previous_page == BASE_PAGE) {
		ret = save_base(window);
	} else if (previous_page == RECT_PAGE) {
		ret = save_rect(window);
	} else if (previous_page == SIZE_PAGE) {
		ret = save_size(window);
	} else if (previous_page == ADVANCED_PAGE) {
		ret = save_advanced(window);
	}
	return ret;
}

void tcontrol_setting::toggle_report(twidget* widget)
{
	treport* report = treport::get_report(widget);
	if (report == rect_navigate_) {
		rect_toggle_tabbar(widget);
		return;
	} else if (report == advanced_navigate_) {
		advanced_toggle_tabbar(widget);
		return;
	} 
	int page = (int)reinterpret_cast<long>(widget->cookie());
	page_panel_->set_radio_layer(page);

	tdialog::toggle_report(widget);
}

void tcontrol_setting::save(twindow& window, bool& handled, bool& halt)
{
	bool ret = true;
	int current_page = (int)reinterpret_cast<long>(bar_->cursel()->cookie());
	if (current_page == BASE_PAGE) {
		ret = save_base(window);
	} else if (current_page == RECT_PAGE) {
		ret = save_rect(window);
	} else if (current_page == SIZE_PAGE) {
		ret = save_size(window);
	} else if (current_page == ADVANCED_PAGE) {
		ret = save_advanced(window);
	}
	if (!ret) {
		handled = true;
		halt = true;
		return;
	}
	window.set_retval(twindow::OK);
}

bool tcontrol_setting::save_base(twindow& window)
{
	ttext_box* text_box = find_widget<ttext_box>(&window, "_border", false, true);
	int border = lexical_cast_default<int>(text_box->label());
	if (border < 0 || border > 50) {
		return false;
	}
	cell_.widget.cell.border_size_ = border;

	ttoggle_button* toggle = find_widget<ttoggle_button>(&window, "_border_left", false, true);
	if (toggle->get_value()) {
		cell_.widget.cell.flags_ |= tgrid::BORDER_LEFT;
	} else {
		cell_.widget.cell.flags_ &= ~tgrid::BORDER_LEFT;
	}
	toggle = find_widget<ttoggle_button>(&window, "_border_right", false, true);
	if (toggle->get_value()) {
		cell_.widget.cell.flags_ |= tgrid::BORDER_RIGHT;
	} else {
		cell_.widget.cell.flags_ &= ~tgrid::BORDER_RIGHT;
	}
	toggle = find_widget<ttoggle_button>(&window, "_border_top", false, true);
	if (toggle->get_value()) {
		cell_.widget.cell.flags_ |= tgrid::BORDER_TOP;
	} else {
		cell_.widget.cell.flags_ &= ~tgrid::BORDER_TOP;
	}
	toggle = find_widget<ttoggle_button>(&window, "_border_bottom", false, true);
	if (toggle->get_value()) {
		cell_.widget.cell.flags_ |= tgrid::BORDER_BOTTOM;
	} else {
		cell_.widget.cell.flags_ &= ~tgrid::BORDER_BOTTOM;
	}

	text_box = find_widget<ttext_box>(&window, "_id", false, true);
	cell_.id = text_box->label();
	// id maybe empty.
	if (!cell_.id.empty() && !utils::isvalid_id(cell_.id, false, min_id_len, max_id_len)) {
		gui2::show_message(disp_.video(), "", utils::errstr);
		return false;
	}
	utils::transform_tolower2(cell_.id);

	tscroll_text_box* scroll_text_box = find_widget<tscroll_text_box>(&window, "_label", false, true);
	cell_.widget.label = scroll_text_box->label();
	scroll_text_box = find_widget<tscroll_text_box>(&window, "_tooltip", false, true);
	cell_.widget.tooltip = scroll_text_box->label();

	return true;
}

bool tcontrol_setting::save_rect(twindow& window)
{
	ttoggle_button* toggle = find_widget<ttoggle_button>(&window, "rect", false, true);
	if (!toggle->get_value()) {
		return true;
	}

	toggle = find_widget<ttoggle_button>(&window, "custom_rect", false, true);
	if (rect_current_tab_ && !toggle->get_value()) {
		return true;
	}

	std::stringstream ss;
	std::string val = calculate_rect_coor(window, 0);
	if (val.empty()) {
		return false;
	}
	ss << val;

	val = calculate_rect_coor(window, 1);
	if (val.empty()) {
		return false;
	}
	ss << "," << val;

	val = calculate_rect_coor(window, 2);
	if (val.empty()) {
		return false;
	}
	ss << "," << val;

	val = calculate_rect_coor(window, 3);
	if (val.empty()) {
		return false;
	}
	ss << "," << val;

	current_rect_cfg_["rect"] = ss.str();

	const tmode& current_mode = controller_.mode(rect_current_tab_);

	if (rect_current_tab_) {
		u_.insert_adjust(tadjust(tadjust::CHANGE, current_mode, current_rect_cfg_));
	} else {
		cell_.rect_cfg = current_rect_cfg_;
	}

	reload_tab_label(*rect_navigate_);
	return true;
}

bool tcontrol_setting::save_size(twindow& window)
{
	tscroll_text_box* scroll_text_box = find_widget<tscroll_text_box>(&window, "_width", false, true);
	cell_.widget.width = scroll_text_box->label();
	scroll_text_box = find_widget<tscroll_text_box>(&window, "_height", false, true);
	cell_.widget.height = scroll_text_box->label();
	return true;
}

bool tcontrol_setting::save_advanced(twindow& window)
{
	if (u_.is_tree_view()) {
		ttext_box* text_box = find_widget<ttext_box>(&window, "indention_step_size", false, true);
		cell_.widget.tree_view.indention_step_size = lexical_cast_default<int>(text_box->label());
		text_box = find_widget<ttext_box>(&window, "node_id", false, true);
		cell_.widget.tree_view.node_id = text_box->get_value();
		if (cell_.widget.tree_view.node_id.empty()) {
			show_id_error(disp_, "node's id", _("Can not empty!"));
			return false;
		}
	}

	if (u_.is_slider()) {
		ttext_box* text_box = find_widget<ttext_box>(&window, "minimum_value", false, true);
		cell_.widget.slider.minimum_value = lexical_cast_default<int>(text_box->label());
		text_box = find_widget<ttext_box>(&window, "maximum_value", false, true);
		cell_.widget.slider.maximum_value = lexical_cast_default<int>(text_box->label());
		if (cell_.widget.slider.minimum_value >= cell_.widget.slider.maximum_value) {
			show_id_error(disp_, "Minimum or Maximum value", _("Maximum value must large than minimum value!"));
			return false;
		}
		text_box = find_widget<ttext_box>(&window, "step_size", false, true);
		cell_.widget.slider.step_size = lexical_cast_default<int>(text_box->label());
		if (cell_.widget.slider.step_size <= 0) {
			show_id_error(disp_, "Step size", _("Step size must > 0!"));
			return false;
		}
		if (cell_.widget.slider.step_size >= cell_.widget.slider.maximum_value - cell_.widget.slider.minimum_value) {
			show_id_error(disp_, "Step size", _("Step size must < maximum_value - minimum_value!"));
			return false;
		}
	}

	const tmode& current_mode = controller_.mode(advanced_current_tab_);

	if (u_.is_report()) {
		ttext_box* text_box = find_widget<ttext_box>(&window, "unit_width", false, true);
		int width = lexical_cast_default<int>(text_box->label());
		if (width < 0) {
			show_id_error(disp_, _("Unit width"), _("must be larger than 0!"));
			return false;
		}
		
		text_box = find_widget<ttext_box>(&window, "unit_height", false, true);
		int height = lexical_cast_default<int>(text_box->label());
		if (height < 0) {
			show_id_error(disp_, _("Unit height"), _("must be larger than 0!"));
			return false;
		}
		text_box = find_widget<ttext_box>(&window, "gap", false, true);
		int gap = lexical_cast_default<int>(text_box->label());
		if (gap < 0) {
			show_id_error(disp_, _("Gap"), _("must not be less than 0!"));
			return false;
		}

		if (!advanced_current_tab_) {
			cell_.widget.report.multi_line = current_advanced_cfg_["multi_line"].to_bool();
			cell_.widget.report.unit_width = width;
			cell_.widget.report.unit_height = height;
			cell_.widget.report.gap = gap;
		} else {
			current_advanced_cfg_["unit_width"] = width;
			current_advanced_cfg_["unit_height"] = height;
			current_advanced_cfg_["gap"] = gap;
		}
	}

	if (u_.has_drag()) {
		cell_.widget.drag = 0;
		ttoggle_button* toggle = find_widget<ttoggle_button>(&window, "_drag_left", false, true);
		if (toggle->get_value()) {
			cell_.widget.drag |= twidget::drag_left;
		}
		toggle = find_widget<ttoggle_button>(&window, "_drag_right", false, true);
		if (toggle->get_value()) {
			cell_.widget.drag |= twidget::drag_right;
		}
		toggle = find_widget<ttoggle_button>(&window, "_drag_up", false, true);
		if (toggle->get_value()) {
			cell_.widget.drag |= twidget::drag_up;
		}
		toggle = find_widget<ttoggle_button>(&window, "_drag_down", false, true);
		if (toggle->get_value()) {
			cell_.widget.drag |= twidget::drag_down;
		}
	}

	if (advanced_current_tab_) {
		u_.insert_adjust(tadjust(tadjust::CHANGE, current_mode, current_advanced_cfg_));

		ttoggle_button* toggle = find_widget<ttoggle_button>(&window, "remove", false, true);
		if (toggle->get_active()) {
			if (toggle->get_value()) {
				u_.insert_adjust(tadjust(tadjust::REMOVE, current_mode, null_cfg));
			} else {
				u_.erase_adjust(current_mode, tadjust::REMOVE);
			}
		}
	}

	reload_tab_label(*advanced_navigate_);
	return true;
}

void tcontrol_setting::set_horizontal_layout(twindow& window)
{
	std::stringstream ss;
	std::vector<tval_str> items;

	for (std::map<int, tlayout>::const_iterator it = horizontal_layout.begin(); it != horizontal_layout.end(); ++ it) {
		ss.str("");
		ss << tintegrate::generate_img(it->second.icon + "~SCALE(24, 24)") << it->second.description;
		items.push_back(tval_str(it->first, ss.str()));
	}

	unsigned h_flag = cell_.widget.cell.flags_ & tgrid::HORIZONTAL_MASK;
	gui2::tcombo_box dlg(items, h_flag);
	dlg.show(disp_.video());

	h_flag = dlg.selected_val();
	cell_.widget.cell.flags_ = (cell_.widget.cell.flags_ & ~tgrid::HORIZONTAL_MASK) | h_flag;

	set_horizontal_layout_label(window);
}

void tcontrol_setting::set_vertical_layout(twindow& window)
{
	std::stringstream ss;
	std::vector<tval_str> items;

	for (std::map<int, tlayout>::const_iterator it = vertical_layout.begin(); it != vertical_layout.end(); ++ it) {
		ss.str("");
		ss << tintegrate::generate_img(it->second.icon + "~SCALE(24, 24)") << it->second.description;
		items.push_back(tval_str(it->first, ss.str()));
	}

	unsigned h_flag = cell_.widget.cell.flags_ & tgrid::VERTICAL_MASK;
	gui2::tcombo_box dlg(items, h_flag);
	dlg.show(disp_.video());

	h_flag = dlg.selected_val();
	cell_.widget.cell.flags_ = (cell_.widget.cell.flags_ & ~tgrid::VERTICAL_MASK) | h_flag;

	set_vertical_layout_label(window);
}

void tcontrol_setting::set_horizontal_layout_label(twindow& window)
{
	const unsigned h_flag = cell_.widget.cell.flags_ & tgrid::HORIZONTAL_MASK;
	std::stringstream ss;

	ss << horizontal_layout.find(h_flag)->second.description;
	tlabel* label = find_widget<tlabel>(&window, "_horizontal_layout", false, true);
	label->set_label(ss.str());
}

void tcontrol_setting::set_vertical_layout_label(twindow& window)
{
	const unsigned v_flag = cell_.widget.cell.flags_ & tgrid::VERTICAL_MASK;
	std::stringstream ss;

	ss << vertical_layout.find(v_flag)->second.description;
	tlabel* label = find_widget<tlabel>(&window, "_vertical_layout", false, true);
	label->set_label(ss.str());
}

void tcontrol_setting::set_linked_group(twindow& window)
{
	std::stringstream ss;
	std::vector<tval_str> items;

	int index = -1;
	int def = -1;
	items.push_back(tval_str(index ++, ""));
	for (std::vector<tlinked_group>::const_iterator it = linkeds_.begin(); it != linkeds_.end(); ++ it) {
		const tlinked_group& linked = *it;
		ss.str("");
		ss << linked.id;
		if (linked.fixed_width) {
			ss << "  " << tintegrate::generate_format("width", "blue");
		}
		if (linked.fixed_height) {
			ss << "  " << tintegrate::generate_format("height", "blue");
		}
		if (cell_.widget.linked_group == linked.id) {
			def = index;
		}
		items.push_back(tval_str(index ++, ss.str()));
	}

	gui2::tcombo_box dlg(items, def);
	dlg.show(disp_.video());

	index = dlg.selected_val();
	if (index >= 0) {
		cell_.widget.linked_group = linkeds_[index].id;
	} else {
		cell_.widget.linked_group.clear();
	}

	set_linked_group_label(window);
}

void tcontrol_setting::set_linked_group_label(twindow& window)
{
	std::stringstream ss;

	ss << cell_.widget.linked_group;
	ttext_box* text_box = find_widget<ttext_box>(&window, "_linked_group", false, true);
	text_box->set_label(ss.str());
	text_box->set_active(false);

	if (linkeds_.empty()) {
		find_widget<tbutton>(&window, "_set_linked_group", false, true)->set_active(false);
	}
}

void tcontrol_setting::set_textdomain(twindow& window, bool label)
{
	std::stringstream ss;
	std::vector<tval_str> items;

	std::string& textdomain = label? cell_.widget.label_textdomain: cell_.widget.tooltip_textdomain;
	
	int index = -1;
	int def = -1;
	items.push_back(tval_str(index ++, ""));
	for (std::vector<std::string>::const_iterator it = textdomains_.begin(); it != textdomains_.end(); ++ it) {
		ss.str("");
		ss << *it;
		if (*it == textdomain) {
			def = index;
		}
		items.push_back(tval_str(index ++, ss.str()));
	}

	gui2::tcombo_box dlg(items, def);
	dlg.show(disp_.video());

	index = dlg.selected_val();
	if (index >= 0) {
		textdomain = textdomains_[index];
	} else {
		textdomain.clear();
	}

	set_textdomain_label(window, label);
}

void tcontrol_setting::set_textdomain_label(twindow& window, bool label)
{
	std::stringstream ss;

	const std::string id = label? "textdomain_label": "textdomain_tooltip";
	std::string str = label? cell_.widget.label_textdomain: cell_.widget.tooltip_textdomain;
	if (str.empty()) {
		str = "---";
	}

	tbutton* button = find_widget<tbutton>(&window, id, false, true);
	button->set_label(str);

	if (textdomains_.empty()) {
		button->set_active(false);
	}
}


//
// rect page
//
const std::string tcontrol_setting::default_ref = "(default)";

void tcontrol_setting::switch_rect_cfg(twindow& window, const config& cfg)
{
	set_ref_label(window, cfg);
	set_rect_label(window, cfg, 0);
	set_rect_label(window, cfg, 1);
	set_rect_label(window, cfg, 2);
	set_rect_label(window, cfg, 3);
	set_xanchor_label(window, cfg);
	set_yanchor_label(window, cfg);
}

void tcontrol_setting::rect_toggled(twidget* widget)
{
	VALIDATE(!rect_current_tab_, null_str);

	const tmode& current_mode = controller_.mode(rect_current_tab_);

	ttoggle_button* toggle = dynamic_cast<ttoggle_button*>(widget);
	twindow* window = toggle->get_window();

	bool active = toggle->get_value();
	tgrid* grid = find_widget<tgrid>(window, "grid_set_rect", false, true);
	grid->set_visible(active? twidget::VISIBLE: twidget::INVISIBLE);

	if (active) {
		// ttoggle_button* toggle = find_widget<ttoggle_button>(window, "custom_rect", false, true);
		// toggle->set_visible(twidget::VISIBLE);
		// toggle->set_active(true);

		grid->set_visible(twidget::VISIBLE);

		current_rect_cfg_ = tadjust::generate_empty_rect_cfg();
		switch_rect_cfg(*window, current_rect_cfg_);

	} else {
		current_rect_cfg_.clear();
		cell_.rect_cfg = current_rect_cfg_;
		// clear all rect change.
		u_.adjust_clear_rect_cfg(NULL);
		reload_tab_label(*rect_navigate_);
	}
}

void tcontrol_setting::custom_rect_toggled(twidget* widget)
{
	VALIDATE(rect_current_tab_, null_str);

	ttoggle_button* toggle = dynamic_cast<ttoggle_button*>(widget);
	twindow* window = toggle->get_window();

	bool active = toggle->get_value();
	tgrid* grid = find_widget<tgrid>(window, "grid_set_rect2", false, true);
	grid->set_visible(active? twidget::VISIBLE: twidget::INVISIBLE);

	if (active) {
		current_rect_cfg_ = get_derived_res_change_cfg(rect_current_tab_, true);
		switch_rect_cfg(*window, current_rect_cfg_);

	} else {
		// current_rect_cfg_.clear();
		// cell_.rect_cfg = current_rect_cfg_;

		// clear current mode's rect change.
		const tmode& current_mode = controller_.mode(rect_current_tab_);
		u_.adjust_clear_rect_cfg(&current_mode);
		reload_tab_label(*rect_navigate_);
	}
}

bool tcontrol_setting::collect_ref(const unit::tchild& child, std::vector<std::string>& refs) const
{
	for (std::vector<unit*>::const_iterator it = child.units.begin(); it != child.units.end(); ++ it) {
		const unit* u = *it;
		if (u == &u_) {
			return false;
		}
		if (!u->cell().id.empty()) {
			refs.push_back(u->cell().id);
		}
		const std::vector<unit::tchild>& children = u->children();
		for (std::vector<unit::tchild>::const_iterator it2 = children.begin(); it2 != children.end(); ++ it2) {
			if (!collect_ref(*it2, refs)) {
				return false;
			}
		}
	}
	return true;
}

void tcontrol_setting::set_ref(twindow& window)
{
	std::stringstream ss;
	std::vector<tval_str> items;
	std::vector<std::string> refs;

	int index = 0;
	int def = index;
	
	refs.push_back("");
	if (controller_.in_theme_top()) {
		refs.push_back(theme::id_screen);
		std::vector<unit*> units = controller_.form_top_units();
		for (std::vector<unit*>::const_iterator it = units.begin(); it != units.end(); ++ it) {
			const unit* u = *it;
			if (u->type() != unit::WIDGET) {
				continue;
			}
			if (u->parent_at_top() == u_.parent_at_top()) {
				break;
			}
			if (!u->cell().id.empty()) {
				refs.push_back(u->cell().id);
			}
		}
	} else {
		unit* u = controller_.current_unit();
		refs.push_back(u->cell().id);
		const std::vector<unit::tchild>& children = u->children();
		for (std::vector<unit::tchild>::const_iterator it = children.begin(); it != children.end(); ++ it) {
			if (!collect_ref(*it, refs)) {
				break;
			}
		}
	}

	for (std::vector<std::string>::const_iterator it = refs.begin(); it != refs.end(); ++ it) {
		const std::string& ref = *it;
		if (ref == current_rect_cfg_["ref"].str()) {
			def = index;
		}
		ss.str("");
		if (ref.empty()) {
			ss << tintegrate::generate_format(default_ref, "red");
		} else if (ref == theme::id_screen) {
			ss << tintegrate::generate_format(ref, "green");
		} else if (ref == theme::id_main_map) {
			ss << tintegrate::generate_format(ref, "yellow");
		} else {
			ss << ref;
		}
		items.push_back(tval_str(index ++, ss.str()));
	}

	gui2::tcombo_box dlg(items, def);
	dlg.show(disp_.video());

	current_rect_cfg_["ref"] = refs[dlg.selected_index()];

	set_ref_label(window, current_rect_cfg_);
}

void tcontrol_setting::set_rect_coor(twindow& window, int n)
{
	std::vector<std::string> vstr = utils::split(current_rect_cfg_["rect"].str());
	std::string val = (int)vstr.size() > n? vstr[n]: "0";
	const std::vector<std::string>& op = candidate_rect_coor_ops_.find(n)->second;

	std::stringstream ss;
	std::vector<tval_str> items;

	int index = 0;
	for (std::vector<std::string>::const_iterator it = op.begin(); it != op.end(); ++ it) {
		ss.str("");
		ss << *it;
		items.push_back(tval_str(index ++, ss.str()));
	}

	gui2::tcombo_box dlg(items, rect_coor_ops_[n]);
	dlg.show(disp_.video());

	int selected = dlg.selected_val();
	std::string set_id;
	if (n == 0) {
		set_id = "set_x1";
	} else if (n == 1) {
		set_id = "set_y1";
	} else if (n == 2) {
		set_id = "set_x2";
	} else if (n == 3) {
		set_id = "set_y2";
	}
	find_widget<tbutton>(&window, set_id, false, true)->set_label(op[selected]);
	rect_coor_ops_[n] = selected;
}

void tcontrol_setting::set_xanchor_label(twindow& window, const config& cfg) const
{
	int xanchor = theme::read_anchor(cfg["xanchor"]);

	std::stringstream ss;
	ss << horizontal_anchor.find(xanchor)->second.description;
	tlabel* label = find_widget<tlabel>(&window, "xanchor", false, true);
	label->set_label(ss.str());
}

void tcontrol_setting::set_xanchor(twindow& window)
{
	std::stringstream ss;
	std::vector<tval_str> items;

	for (std::map<int, tanchor>::const_iterator it = horizontal_anchor.begin(); it != horizontal_anchor.end(); ++ it) {
		ss.str("");
		ss << it->second.description;
		items.push_back(tval_str(it->first, ss.str()));
	}

	int xanchor = theme::read_anchor(current_rect_cfg_["xanchor"]);
	gui2::tcombo_box dlg(items, xanchor);
	dlg.show(disp_.video());

	xanchor = dlg.selected_val();
	current_rect_cfg_["xanchor"] = horizontal_anchor.find(xanchor)->second.id;

	set_xanchor_label(window, current_rect_cfg_);
}

void tcontrol_setting::set_yanchor_label(twindow& window, const config& cfg) const
{
	int yanchor = theme::read_anchor(cfg["yanchor"]);

	std::stringstream ss;
	ss << vertical_anchor.find(yanchor)->second.description;
	tlabel* label = find_widget<tlabel>(&window, "yanchor", false, true);
	label->set_label(ss.str());
}

void tcontrol_setting::set_yanchor(twindow& window)
{
	std::stringstream ss;
	std::vector<tval_str> items;

	for (std::map<int, tanchor>::const_iterator it = vertical_anchor.begin(); it != vertical_anchor.end(); ++ it) {
		ss.str("");
		ss << it->second.description;
		items.push_back(tval_str(it->first, ss.str()));
	}

	int yanchor = theme::read_anchor(current_rect_cfg_["yanchor"]);
	gui2::tcombo_box dlg(items, yanchor);
	dlg.show(disp_.video());

	yanchor = dlg.selected_val();
	current_rect_cfg_["yanchor"] = vertical_anchor.find(yanchor)->second.id;

	set_yanchor_label(window, current_rect_cfg_);
}

void tcontrol_setting::set_ref_label(twindow& window, const config& cfg)
{
	std::string str = cfg["ref"].str();
	if (str.empty()) {
		str = default_ref;
	}

	tlabel* label = find_widget<tlabel>(&window, "ref", false, true);
	label->set_label(str);
}

int tcontrol_setting::calculate_rect_coor_op(const std::string& val) const
{
	char flag = val.at(0);
	int op = 0; // abs
	if (flag == '=') {
		op = 1;
	} else if (flag == '-' || flag == '+') {
		op = 2;
	}
	return op;
}

std::string tcontrol_setting::calculate_rect_coor(twindow& window, int n) const
{
	VALIDATE(n >= 0 && n < 4, null_str);

	std::string val_id;
	if (n == 0) {
		val_id = "x1";
	} else if (n == 1) {
		val_id = "y1";
	} else if (n == 2) {
		val_id = "x2";
	} else if (n == 3) {
		val_id = "y2";
	}
	ttext_box* text_box = find_widget<ttext_box>(&window, val_id, false, true);
	std::string val = text_box->get_value();

	std::stringstream err;
	utils::string_map symbols;
	symbols["id"] = tintegrate::generate_format(val_id, "yellow");

	if (utils::isinteger(val.c_str())) {
		int val_n = lexical_cast<int>(val);
		char ch = val[0];

		int op = rect_coor_ops_[n];
		if (!op) {
			if (ch == '+' || ch == '-') {
				err << vgettext2("$id is absolute type, must not be contain '+' or '-'!", symbols);
			}
		} else if (op == 1) {
			val = std::string("=") + val;

		} else if (op == 2) {
			if (val_n >= 0 && ch != '+' && ch != '-') {
				val = std::string("+") + val;
			}
		}
	} else {
		err << vgettext2("$id must be integer!", symbols);
	}
	if (!err.str().empty()) {
		gui2::show_message(disp_.video(), "", err.str());
		return null_str;
	}
	return val;
}

void tcontrol_setting::set_rect_label(twindow& window, const config& cfg, int n)
{
	VALIDATE(n >= 0 && n < 4, null_str);

	std::vector<std::string> vstr = utils::split(cfg["rect"].str());
	std::string val = (int)vstr.size() > n? vstr[n]: "0";
	if (val.empty()) {
		val = "0";
	}

	std::string set_id, val_id;
	if (n == 0) {
		set_id = "set_x1";
		val_id = "x1";
	} else if (n == 1) {
		set_id = "set_y1";
		val_id = "y1";
	} else if (n == 2) {
		set_id = "set_x2";
		val_id = "x2";
	} else if (n == 3) {
		set_id = "set_y2";
		val_id = "y2";
	}
	int op = calculate_rect_coor_op(val);
	rect_coor_ops_[n] = op;
	tbutton* button = find_widget<tbutton>(&window, set_id, false, true);
	button->set_label(candidate_rect_coor_ops_.find(n)->second[op]);

	ttext_box* text_box = find_widget<ttext_box>(&window, val_id, false, true);
	if (op == 1) {
		val = val.substr(1);
		if (val.empty()) {
			val = "+0";
		} else {
			char symbol = val.at(0);
			if (symbol != '+' && symbol != '-') {
				val = std::string("+") + val;
			}
		}

	} else if (op == 2) {
		char symbol = val.at(0);
		if (symbol != '+' && symbol != '-') {
			val = std::string("+") + val;
		}
	}
	text_box->set_label(val);
}

bool tcontrol_setting::rect_pre_toggle_tabbar(twidget* widget, twidget* previous)
{
	twindow& window = *widget->get_window();
	return save_rect(window);
}

void tcontrol_setting::rect_toggle_tabbar(twidget* widget)
{
	twindow* window = widget->get_window();
	rect_current_tab_ = (int)reinterpret_cast<long>(widget->cookie());
	tdialog::toggle_report(widget);

	const tmode& current_mode = controller_.mode(rect_current_tab_);

	ttoggle_button* toggle = find_widget<ttoggle_button>(window, "rect", false, true);
	toggle->set_active(!must_use_rect_ && !rect_current_tab_);
	if (!toggle->get_value()) {
		return;
	}

	bool has_rect = true;

	toggle = find_widget<ttoggle_button>(window, "custom_rect", false, true);
	toggle->set_visible(rect_current_tab_? twidget::VISIBLE: twidget::INVISIBLE);
	if (rect_current_tab_) {
		has_rect = tadjust::cfg_has_rect_fields(u_.adjust(current_mode, tadjust::CHANGE).cfg);
		toggle->set_value(has_rect);
	}

	tgrid* grid = find_widget<tgrid>(window, "grid_set_rect2", false, true);
	grid->set_visible(has_rect? twidget::VISIBLE: twidget::INVISIBLE);
	if (!has_rect) {
		return;
	}

	if (!rect_current_tab_) {
		current_rect_cfg_ = cell_.rect_cfg;

	} else {
		current_rect_cfg_ = get_derived_res_change_cfg(rect_current_tab_, true);
	}

	switch_rect_cfg(*window, current_rect_cfg_);
}

config tcontrol_setting::get_derived_res_change_cfg(int current_tab, bool rect) const
{
	VALIDATE(current_tab, "current_tab must not be 0!");

	config result;
	// shoud fill all fields.
	if (rect) {
		result = cell_.rect_cfg;
	} else {
		result = u_.generate2_change2_cfg(cell_);
	}

	const tmode& mode = controller_.mode(current_tab);
	// apply base adjust if has. If it is patch, should get from corresponding main's res.
	result = u_.get_base_change_cfg(mode, rect, result);
	
	// apply myself adjust if has.
	tadjust adjust = u_.adjust(mode, tadjust::CHANGE);
	if (adjust.valid()) {
		adjust.pure_change_fields(rect);
		result.merge_attributes(adjust.cfg);
	}
	return result;
}

std::string tcontrol_setting::form_tab_label(treport& navigate, int at) const
{
	const tmode& mode = controller_.mode(at);

	std::stringstream ss;
	ss << mode.id << "\n";
	ss << mode.res.width << "x" << mode.res.height;
	if (require_show_flag(navigate, at)) {
		ss << tintegrate::generate_img("misc/dot.png");
	}
	return ss.str();
}

bool tcontrol_setting::require_show_flag(treport& navigate, int index) const
{
	if (!index) {
		return false;
	}
	const tmode& mode = controller_.mode(index);
	tadjust adjust = u_.adjust(mode, tadjust::CHANGE);
	if (!adjust.valid()) {
		return false;
	}
	if (&navigate == rect_navigate_) {
		return adjust.different_change_cfg(u_.get_base_change_cfg(mode, true, cell_.rect_cfg), t_true);
	} else {
		return adjust.different_change_cfg(u_.get_base_change_cfg(mode, false, u_.generate2_change2_cfg(cell_)), t_false);
	}
}

//
// avcanced page
//
void tcontrol_setting::switch_advanced_cfg(twindow& window, const config& cfg, bool enable)
{
	set_advanced_misc(window, cfg, enable);
	if (u_.is_report()) {
		set_report_attribute(window, cfg, enable);
	}
}

bool tcontrol_setting::advanced_pre_toggle_tabbar(twidget* widget, twidget* previous)
{
	twindow& window = *widget->get_window();
	return save_advanced(window);
}

void tcontrol_setting::advanced_toggle_tabbar(twidget* widget)
{
	twindow* window = widget->get_window();
	advanced_current_tab_ = (int)reinterpret_cast<long>(widget->cookie());
	tdialog::toggle_report(widget);

	if (!advanced_current_tab_) {
		current_advanced_cfg_ = u_.generate2_change2_cfg(cell_);

	} else {
		current_advanced_cfg_ = get_derived_res_change_cfg(advanced_current_tab_, false);
	}

	switch_advanced_cfg(*window, current_advanced_cfg_, true);
}

void tcontrol_setting::set_horizontal_mode(twindow& window)
{
	std::stringstream ss;
	std::vector<tval_str> items;

	for (std::map<int, tscroll_mode>::const_iterator it = horizontal_mode.begin(); it != horizontal_mode.end(); ++ it) {
		ss.str("");
		ss << it->second.description;
		items.push_back(tval_str(it->first, ss.str()));
	}

	gui2::tcombo_box dlg(items, cell_.widget.horizontal_mode);
	dlg.show(disp_.video());

	cell_.widget.horizontal_mode = (tscrollbar_container::tscrollbar_mode)dlg.selected_val();

	set_horizontal_mode_label(window);
}

void tcontrol_setting::set_horizontal_mode_label(twindow& window)
{
	std::stringstream ss;

	ss << horizontal_mode.find(cell_.widget.horizontal_mode)->second.description;
	tlabel* label = find_widget<tlabel>(&window, "_horizontal_mode", false, true);
	label->set_label(ss.str());
}

void tcontrol_setting::set_vertical_mode(twindow& window)
{
	std::stringstream ss;
	std::vector<tval_str> items;

	for (std::map<int, tscroll_mode>::const_iterator it = vertical_mode.begin(); it != vertical_mode.end(); ++ it) {
		ss.str("");
		ss << it->second.description;
		items.push_back(tval_str(it->first, ss.str()));
	}

	gui2::tcombo_box dlg(items, cell_.widget.vertical_mode);
	dlg.show(disp_.video());

	cell_.widget.vertical_mode = (tscrollbar_container::tscrollbar_mode)dlg.selected_val();

	set_vertical_mode_label(window);
}

void tcontrol_setting::set_vertical_mode_label(twindow& window)
{
	std::stringstream ss;

	ss << vertical_mode.find(cell_.widget.vertical_mode)->second.description;
	tlabel* label = find_widget<tlabel>(&window, "_vertical_mode", false, true);
	label->set_label(ss.str());
}

void tcontrol_setting::set_definition(twindow& window)
{
	std::stringstream ss;
	std::vector<tval_str> items;

	const std::string current_definition = advanced_current_tab_? current_advanced_cfg_["definition"]: u_.widget().second->id;

	const gui2::tgui_definition::tcontrol_definition_map& controls = gui2::current_gui->second.control_definition;
	const std::map<std::string, gui2::tcontrol_definition_ptr>& definitions = controls.find(u_.widget().first)->second;

	int index = 0;
	int def = 0;
	for (std::map<std::string, gui2::tcontrol_definition_ptr>::const_iterator it = definitions.begin(); it != definitions.end(); ++ it) {
		ss.str("");
		ss << tintegrate::generate_format(it->first, "blue") << "(" << it->second->description << ")";
		if (it->first == current_definition) {
			def = index;
		}
		items.push_back(tval_str(index ++, ss.str()));
	}

	gui2::tcombo_box dlg(items, def);
	dlg.show(disp_.video());

	std::map<std::string, gui2::tcontrol_definition_ptr>::const_iterator it = definitions.begin();
	std::advance(it, dlg.selected_val());
	if (advanced_current_tab_) {
		current_advanced_cfg_["definition"] = it->first;
	} else {
		u_.set_widget_definition(it->second);
		update_title(window);
	}

	find_widget<tlabel>(&window, "_definition", false).set_label(it->first);
}

void tcontrol_setting::set_text_box2_text_box(twindow& window)
{
	std::stringstream ss;
	std::vector<tval_str> items;

	const std::string current_definition = cell_.widget.text_box2.text_box.empty()? "default": cell_.widget.text_box2.text_box;

	const gui2::tgui_definition::tcontrol_definition_map& controls = gui2::current_gui->second.control_definition;
	const std::map<std::string, gui2::tcontrol_definition_ptr>& definitions = controls.find("text_box")->second;

	int index = 0;
	int def = 0;
	for (std::map<std::string, gui2::tcontrol_definition_ptr>::const_iterator it = definitions.begin(); it != definitions.end(); ++ it) {
		ss.str("");
		ss << tintegrate::generate_format(it->first, "blue") << "(" << it->second->description << ")";
		if (it->first == current_definition) {
			def = index;
		}
		items.push_back(tval_str(index ++, ss.str()));
	}

	gui2::tcombo_box dlg(items, def);
	dlg.show(disp_.video());

	std::map<std::string, gui2::tcontrol_definition_ptr>::const_iterator it = definitions.begin();
	std::advance(it, dlg.selected_val());
	cell_.widget.text_box2.text_box = it->first;

	find_widget<tlabel>(&window, "text_box2_text_box", false).set_label(it->first);
}

void tcontrol_setting::set_advanced_misc(twindow& window, const config& cfg, bool enable)
{
	const gui2::tgui_definition::tcontrol_definition_map& controls = gui2::current_gui->second.control_definition;
	const std::map<std::string, gui2::tcontrol_definition_ptr>& definitions = controls.find(u_.widget().first)->second;

	tbutton* button = find_widget<tbutton>(&window, "_set_definition", false, true);
	button->set_active(enable && definitions.size() > 1);
		
	tlabel* label = find_widget<tlabel>(&window, "_definition", false, true);
	label->set_label(cfg["definition"].str());

	if (controller_.theme()) {
		ttoggle_button* toggle = find_widget<ttoggle_button>(&window, "remove", false, true);
		if (advanced_current_tab_) {
			const tmode& mode = controller_.mode(advanced_current_tab_);

			bool remove = u_.get_base_remove(mode);
			toggle->set_active(!remove);
			if (!remove) {
				tadjust adjust = u_.adjust(mode, tadjust::REMOVE);
				if (adjust.valid()) {
					remove = true;
				}
			}
			toggle->set_value(remove);

		} else {
			toggle->set_active(false);
			toggle->set_value(false);
		}
	}
}

void tcontrol_setting::set_multi_line(twindow& window)
{
	bool multi_line = current_advanced_cfg_["multi_line"].to_bool();
	tbutton* button = find_widget<tbutton>(&window, "multi_line", false, true);
	multi_line = !multi_line;

	button->set_label(multi_line? _("Multi line report"): _("Tabbar"));
	current_advanced_cfg_["multi_line"] = multi_line;
}

void tcontrol_setting::set_report_attribute(twindow& window, const config& cfg, bool enable)
{
	tbutton* button = find_widget<tbutton>(&window, "multi_line", false, true);
	button->set_label(cfg["multi_line"].to_bool()? _("Multi line report"): _("Tabbar"));

	ttext_box* text_box = find_widget<ttext_box>(&window, "unit_width", false, true);
	text_box->set_label(cfg["unit_width"]);
	text_box->set_active(enable);

	text_box = find_widget<ttext_box>(&window, "unit_height", false, true);
	text_box->set_label(cfg["unit_height"]);
	text_box->set_active(enable);

	text_box = find_widget<ttext_box>(&window, "gap", false, true);
	text_box->set_label(cfg["gap"]);
	text_box->set_active(enable);
}

}
