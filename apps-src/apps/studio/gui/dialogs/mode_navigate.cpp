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

#include "gui/dialogs/mode_navigate.hpp"

#include "display.hpp"
#include "formula_string_utils.hpp"
#include "gettext.hpp"
#include "filesystem.hpp"
#include "game_config.hpp"
#include "mkwin_controller.hpp"

#include "gui/dialogs/helper.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/window.hpp"
#include "gui/widgets/label.hpp"
#include "gui/widgets/button.hpp"
#include "gui/widgets/toggle_button.hpp"
#include "gui/widgets/report.hpp"
#include "gui/dialogs/edit_box.hpp"
#include "gui/dialogs/message.hpp"

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

tmode_navigate::tmode_navigate(mkwin_controller& controller, display& disp)
	: controller_(controller)
	, disp_(disp)
{
}

treport* tmode_navigate::pre_show(twindow& window, const std::string& id)
{
	treport* report = find_widget<treport>(&window, id, false, true);
	report->tabbar_init(true, "tab");
	reload_navigate(*report, window);

	return report;
}

void tmode_navigate::reload_tab_label(treport& navigate) const
{
	const tgrid::tchild* children = navigate.child_begin();
	int childs = navigate.childs();
	for (int i = 0; i < childs; i ++) {
		tcontrol* widget = dynamic_cast<tcontrol*>(children[i].widget_);
		widget->set_label(form_tab_label(navigate, i));
	}
}

void tmode_navigate::reload_navigate(treport& navigate, twindow& window)
{
	navigate.erase_children();

	const std::vector<tmode>& modes = controller_.modes();

	std::stringstream ss;
	int index = 0;
	for (std::vector<tmode>::const_iterator it = modes.begin(); it != modes.end(); ++ it) {
		tcontrol* widget = navigate.create_child(null_str, null_str, NULL);
		widget->set_label(form_tab_label(navigate, index));
		widget->set_cookie(reinterpret_cast<void*>(index ++));
		navigate.insert_child(*widget);
	}
	navigate.select(0);
	navigate.replacement_children();
}

void tmode_navigate::append_patch(treport& navigate, twindow& window)
{
	gui2::tedit_box::tparam param(_("New patch"), null_str, _("ID"), "_untitled");
	{
		param.verify = boost::bind(&mkwin_controller::verify_new_mode, &controller_, _1);
		gui2::tedit_box dlg(disp_, param);
		try {
			dlg.show(disp_.video());
		} catch(twml_exception& e) {
			e.show(disp_);
		}
		int res = dlg.get_retval();
		if (res != gui2::twindow::OK) {
			return;
		}
	}

	const std::vector<tmode>& modes = controller_.modes();
	int original_size = (int)modes.size();
	controller_.insert_mode(param.result);

	std::vector<tmode>::const_iterator it = modes.begin();
	std::advance(it, original_size);
	for (int index = original_size; it != modes.end(); ++ it) {
		tcontrol* widget = navigate.create_child(null_str, null_str, NULL);
		widget->set_label(form_tab_label(navigate, index));
		widget->set_cookie(reinterpret_cast<void*>(index ++));
		navigate.insert_child(*widget);
	}
	navigate.replacement_children();
}

void tmode_navigate::rename_patch(treport& navigate, twindow& window, int at)
{
	const std::vector<tmode>& modes = controller_.modes();
	const tmode& mode = modes[at];
	const std::string original_id = mode.id;

	std::stringstream ss;
	ss << _("Original ID") << " " << tintegrate::generate_format(original_id, "green");
	ss << "\n";

	gui2::tedit_box::tparam param(_("Rename patch"), ss.str(), _("ID"), mode.id);
	{
		param.verify = boost::bind(&mkwin_controller::verify_new_mode, &controller_, _1);
		gui2::tedit_box dlg(disp_, param);
		try {
			dlg.show(disp_.video());
		} catch(twml_exception& e) {
			e.show(disp_);
		}
		int res = dlg.get_retval();
		if (res != gui2::twindow::OK) {
			return;
		}
		if (original_id == param.result) {
			return;
		}
	}

	controller_.rename_patch(original_id, param.result);
	navigate.replacement_children();

	reload_tab_label(navigate);
}

void tmode_navigate::erase_patch(treport& navigate, twindow& window, int at)
{
	const std::vector<tmode>& modes = controller_.modes();
	at -= at % tmode::res_count;
	const tmode& mode = modes[at];
	controller_.erase_patch(mode.id);

	for (int index = 0; index < tmode::res_count; index ++) {
		navigate.erase_child(at);
	}

	for (int index = at; at < (int)modes.size(); at ++) {
		const tgrid::tchild& child = navigate.get_child(at);
		child.widget_->set_cookie(reinterpret_cast<void*>(index));
	}

	navigate.replacement_children();
	reload_tab_label(navigate);
}

}
