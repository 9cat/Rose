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

#include "gui/dialogs/grid_setting.hpp"

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
#include "gui/widgets/report.hpp"
#include "gui/dialogs/combo_box.hpp"
#include "gui/dialogs/theme.hpp"
#include "gui/dialogs/message.hpp"
#include "unit_map.hpp"
#include "mkwin_controller.hpp"

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

REGISTER_DIALOG(grid_setting)

tgrid_setting::tgrid_setting(mkwin_controller& controller, display& disp, unit_map& units, unit& u, const std::string& control)
	: tsetting_dialog(u.cell())
	, tmode_navigate(controller, disp)
	, disp_(disp)
	, units_(units)
	, u_(u)
	, control_(control)
	, current_cfg_()
	, navigate_(NULL)
	, current_tab_(0)
{
}

void tgrid_setting::pre_show(CVideo& /*video*/, twindow& window)
{
	window.set_canvas_variable("border", variant("default-border"));

	std::stringstream ss;
	const std::pair<std::string, gui2::tcontrol_definition_ptr>& widget = u_.widget();

	ss << "grid setting";
	ss << "(";
	ss << tintegrate::generate_format(control_, "blue");
	ss << ")";
	tlabel* label = find_widget<tlabel>(&window, "title", false, true);
	label->set_label(ss.str());

	if (!controller_.theme()) {
		tgrid* grid = find_widget<tgrid>(&window, "resolution_grid", false, true);
		grid->set_visible(twidget::INVISIBLE);
	}

	navigate_ = tmode_navigate::pre_show(window, "navigate");
	navigate_->set_boddy(find_widget<twidget>(&window, "bar_panel", false, true));

	navigate_->set_child_visible(0, false);
	navigate_->select(1);
	navigate_->replacement_children();
	current_tab_ = 1;

	ttext_box* text_box = find_widget<ttext_box>(&window, "_id", false, true);
	text_box->set_maximum_length(max_id_len);
	text_box->set_label(cell_.id);

	connect_signal_mouse_left_click(
			  find_widget<tbutton>(&window, "save", false)
			, boost::bind(
				&tgrid_setting::save
				, this
				, boost::ref(window)
				, true));

	switch_cfg(window);
}

void tgrid_setting::switch_cfg(twindow& window)
{
	const tmode& mode = controller_.mode(current_tab_);
	const unit::tparent& parent = u_.parent();

	bool active = true;
	bool remove = false;
	config base_remove2_cfg;
	if (current_tab_) {
		base_remove2_cfg = parent.u->get_base_remove2_cfg(mode);
	}
	if (!base_remove2_cfg.has_attribute(str_cast(parent.number))) {
		tadjust adjust = parent.u->adjust(mode, tadjust::REMOVE2);
		if (adjust.valid()) {
			std::set<int> ret = adjust.newed_remove2_cfg(base_remove2_cfg);
			if (ret.find(parent.number) != ret.end()) {
				remove = true;	
			}
		}
	} else {
		remove = true;
		active = false;
	}
	ttoggle_button* toggle = find_widget<ttoggle_button>(&window, "remove", false, true);
	toggle->set_value(remove);
	toggle->set_active(current_tab_ && active);
}

void tgrid_setting::save(twindow& window, bool exit)
{
	ttext_box* text_box = find_widget<ttext_box>(&window, "_id", false, true);
	cell_.id = text_box->label();
	// id maybe empty.
	if (!cell_.id.empty() && !utils::isvalid_id(cell_.id, false, min_id_len, max_id_len)) {
		gui2::show_message(disp_.video(), "", utils::errstr);
		return ;
	}
	utils::transform_tolower2(cell_.id);

	const unit::tparent& parent = u_.parent();
	const tmode& mode = controller_.mode(current_tab_);

	if (current_tab_) {
		ttoggle_button* toggle = find_widget<ttoggle_button>(&window, "remove", false, true);
		if (toggle->get_active()) {
			config adjust_cfg;
			adjust_cfg[str_cast(parent.number)] = toggle->get_value();
			parent.u->insert_adjust(tadjust(tadjust::REMOVE2, mode, adjust_cfg));
		}
	}

	if (exit) {
		window.set_retval(twindow::OK);
	} else {
		reload_tab_label(*navigate_);
	}
}

void tgrid_setting::toggle_report(twidget* widget)
{
	twindow* window = widget->get_window();
	save(*window, false);

	current_tab_ = (int)reinterpret_cast<long>(widget->cookie());
	tdialog::toggle_report(widget);

	switch_cfg(*window);
}

std::string tgrid_setting::form_tab_label(treport& navigate, int at) const
{
	const tmode& mode = controller_.mode(at);
	const unit::tparent& parent = u_.parent();

	std::stringstream ss;
	ss << mode.id << "\n";
	ss << mode.res.width << "x" << mode.res.height;

	tadjust adjust = parent.u->adjust(mode, tadjust::REMOVE2);
	if (adjust.valid()) {
		config cfg = parent.u->get_base_remove2_cfg(mode);
		std::set<int> ret = adjust.newed_remove2_cfg(cfg);
		if (ret.find(parent.number) != ret.end()) {
			ss << tintegrate::generate_img("misc/delete.png~SCALE(12,12)");
		}
	}
	
	return ss.str();
}

}
