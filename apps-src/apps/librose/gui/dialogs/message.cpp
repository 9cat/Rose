/* $Id: message.cpp 48440 2011-02-07 20:57:31Z mordante $ */
/*
   Copyright (C) 2008 - 2011 by Mark de Wever <koraq@xs4all.nl>
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

#include "gui/dialogs/message.hpp"

#include "gettext.hpp"
#include "gui/widgets/button.hpp"
#include "gui/widgets/image.hpp"
#include "gui/widgets/label.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/window.hpp"
#include "log.hpp"
#include "rose_config.hpp"

#include <boost/foreach.hpp>

namespace gui2 {

REGISTER_DIALOG(simple_message)
REGISTER_DIALOG(portrait_message)

/**
 * Helper to implement private functions without modifying the header.
 *
 * The class is a helper to avoid recompilation and only has static
 * functions.
 */
struct tmessage_implementation
{
	/**
	 * Initialiazes a button.
	 *
	 * @param window              The window that contains the button.
	 * @param button_status       The button status to modify.
	 * @param id                  The id of the button.
	 */
	static void
	init_button(twindow& window, tmessage::tbutton_status& button_status,
			const std::string& id)
	{
		button_status.button = find_widget<tbutton>(
				&window, id, false, true);
		button_status.button->set_visible(button_status.visible);

		if(!button_status.caption.empty()) {
			button_status.button->set_label(button_status.caption);
		}

		if(button_status.retval != twindow::NONE) {
			button_status.button->set_retval(button_status.retval);
		}
	}
};

void tmessage::pre_show(CVideo& /*video*/, twindow& window)
{
	window.set_canvas_variable("border", variant("default-border"));

	// ***** Validate the required buttons ***** ***** ***** *****
	tmessage_implementation::
			init_button(window, buttons_[cancel], "cancel");
	tmessage_implementation::
			init_button(window, buttons_[ok] ,"ok");

	// ***** ***** ***** ***** Set up the widgets ***** ***** ***** *****
	if(!title_.empty()) {
		find_widget<tlabel>(&window, "title", false).set_label(title_);
	}

	tcontrol& label = find_widget<tcontrol>(&window, "label", false);
	label.set_label(message_);

	// The label might not always be a scroll_label but the capturing
	// shouldn't hurt.
	window.keyboard_capture(&label);

	// Override the user value, to make sure it's set properly.
	window.set_click_dismiss(auto_close_);
}

void tmessage::post_show(twindow& /*window*/)
{
	BOOST_FOREACH (tbutton_status& button_status, buttons_) {
		button_status.button = NULL;
	}
}

void tmessage::set_button_caption(const tbutton_id button,
		const std::string& caption)
{
	buttons_[button].caption = caption;
	if(buttons_[button].button) {
		buttons_[button].button->set_label(caption);
	}
}

void tmessage::set_button_visible(const tbutton_id button,
		const twidget::tvisible visible)
{
	buttons_[button].visible = visible;
	if(buttons_[button].button) {
		buttons_[button].button->set_visible(visible);
	}
}

void tmessage::set_button_retval(const tbutton_id button,
		const int retval)
{
	buttons_[button].retval = retval;
	if(buttons_[button].button) {
		buttons_[button].button->set_retval(retval);
	}
}

tmessage::tbutton_status::tbutton_status()
	: button(NULL)
	, caption()
	, visible(twidget::INVISIBLE)
	, retval(twindow::NONE)
{
}

void tportrait_message::pre_show(CVideo& video, gui2::twindow& window)
{
	tmessage::pre_show(video, window);

	window.canvas(1).set_variable("portrait_image", variant(portrait_));
	window.canvas(1).set_variable("incident_image", variant(incident_));
}

void show_message(CVideo& video, const std::string& title,
	const std::string& message, const std::string& button_caption,
	const bool auto_close)
{
	tsimple_message dlg(title, message, auto_close);
	dlg.set_button_caption(tmessage::ok, button_caption);
	dlg.show(video);
}

int show_message(CVideo& video, const std::string& title,
	const std::string& message, const tmessage::tbutton_style button_style,
	const std::string& portrait, const std::string& incident)
{
	if (game_config::no_messagebox && button_style == tmessage::auto_close) {
		return twindow::OK;
	}

	tmessage* dlg;
	
	if (portrait.empty()) {
		dlg = new tsimple_message(title, message, button_style == tmessage::auto_close);
	} else {
		dlg = new tportrait_message(title, message, portrait, incident, button_style == tmessage::auto_close);
	}

	switch (button_style) {
		case tmessage::auto_close :
			break;
		case tmessage::ok_button :
			dlg->set_button_visible(tmessage::ok, twidget::VISIBLE);
			dlg->set_button_caption(tmessage::ok, _("OK"));
			break;
		case tmessage::close_button :
			dlg->set_button_visible(tmessage::ok, twidget::VISIBLE);
			break;
		case tmessage::ok_cancel_buttons :
			dlg->set_button_visible(tmessage::ok, twidget::VISIBLE);
			dlg->set_button_caption(tmessage::ok, _("OK"));
			/* FALL DOWN */
		case tmessage::cancel_button :
			dlg->set_button_visible(tmessage::cancel, twidget::VISIBLE);
			break;
		case tmessage::yes_no_buttons :
			dlg->set_button_visible(tmessage::ok, twidget::VISIBLE);
			dlg->set_button_caption(tmessage::ok, _("Yes"));
			dlg->set_button_visible(tmessage::cancel, twidget::VISIBLE);
			dlg->set_button_caption(tmessage::cancel, _("No"));
			break;
	}

	dlg->show(video);
	int retval = dlg->get_retval();
	delete dlg;

	return retval;
}

void show_error_message(CVideo& video, const std::string& message,
	bool message_use_markup)
{
	LOG_STREAM(err, lg::general) << message << '\n';
	show_message(video, _("Error"), message,
			tmessage::ok_button, "hero-256/0.png", "");
}

} // namespace gui2

