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

#ifndef GUI_DIALOGS_MODE_NAVIGATE_HPP_INCLUDED
#define GUI_DIALOGS_MODE_NAVIGATE_HPP_INCLUDED

#include "gui/dialogs/dialog.hpp"
#include "util.hpp"
#include "serialization/string_utils.hpp"
#include <vector>
#include <set>

class display;
class mkwin_controller;

namespace gui2 {

class treport;

class tmode_navigate
{
public:
	tmode_navigate(mkwin_controller& controller, display& disp);

protected:
	treport* pre_show(twindow& window, const std::string& id);

	void append_patch(treport& navigate, twindow& window);
	void rename_patch(treport& navigate, twindow& window, int at);
	void erase_patch(treport& navigate, twindow& window, int at);

	void reload_navigate(treport& navigate, twindow& window);

	virtual std::string form_tab_label(treport& navigate, int at) const = 0;
	void reload_tab_label(treport& navigate) const;

protected:
	display& disp_;
	mkwin_controller& controller_;
};


}


#endif /* ! GUI_DIALOGS_CAMPAIGN_DIFFICULTY_HPP_INCLUDED */
