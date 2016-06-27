/* $Id: transient_message.cpp 48961 2011-03-20 18:26:19Z mordante $ */
/*
   Copyright (C) 2009 - 2011 by Mark de Wever <koraq@xs4all.nl>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#define GETTEXT_DOMAIN "rose-lib"

#include "gui/dialogs/transient_message.hpp"

#include "gettext.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/window.hpp"
#include "log.hpp"

namespace gui2 {

REGISTER_DIALOG(transient_message)

ttransient_message::ttransient_message(const std::string& title
		, const bool title_use_markup
		, const std::string& message
		, const bool message_use_markup
		, const std::string& image)
		: title_(title)
		, message_(message)
		, image_(image)
{
}

void ttransient_message::pre_show(CVideo& /*video*/, twindow& window)
{
	window.set_canvas_variable("border", variant("default-border"));

	find_widget<tcontrol>(&window, "title", false).set_label(title_);
	find_widget<tcontrol>(&window, "message", false).set_label(message_);
	find_widget<tcontrol>(&window, "image", false).set_label(image_);
}

void show_transient_message(CVideo& video
		, const std::string& title
		, const std::string& message
		, const std::string& image
		, const bool message_use_markup
		, const bool title_use_markup)
{
	ttransient_message dlg(title
			, title_use_markup
			, message
			, message_use_markup
			, image);

	dlg.show(video);
}

void show_transient_error_message(CVideo& video
		, const std::string& message
		, const std::string& image
		, const bool message_use_markup)
{
	LOG_STREAM(err, lg::general) << message << '\n';
	show_transient_message(video
			, _("Error")
			, message
			, image
			, message_use_markup);
}

} // namespace gui2

