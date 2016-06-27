/* $Id: toggle_button.cpp 54604 2012-07-07 00:49:45Z loonycyborg $ */
/*
   Copyright (C) 2008 - 2012 by Mark de Wever <koraq@xs4all.nl>
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

#include "gui/widgets/toggle_button.hpp"

#include "gui/auxiliary/log.hpp"
#include "gui/auxiliary/widget_definition/toggle_button.hpp"
#include "gui/auxiliary/window_builder/toggle_button.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/window.hpp"
#include "sound.hpp"

#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#define LOG_SCOPE_HEADER get_control_type() + " [" + id() + "] " + __func__
#define LOG_HEADER LOG_SCOPE_HEADER + ':'

namespace gui2 {

ttoggle_button* create_toggle_button(const config& cfg)
{
	implementation::tbuilder_toggle_button builder(cfg);
	return dynamic_cast<ttoggle_button*>(builder.build());
}

ttoggle_button* create_toggle_button(const std::string& id, const std::string& definition, void* cookie)
{
	config cfg;

	if (!id.empty()) {
		cfg["id"] = id;
	}
	cfg["definition"] = definition;

	ttoggle_button* widget = create_toggle_button(cfg);
	widget->set_cookie(cookie);
	return widget;
}

REGISTER_WIDGET(toggle_button)

ttoggle_button::ttoggle_button()
	: tcontrol(COUNT)
	, state_(ENABLED)
	, radio_(false)
	, retval_(0)
	, callback_state_pre_change_()
	, callback_state_change_()
	, icon_name_()
{
	connect_signal<event::MOUSE_ENTER>(boost::bind(
				&ttoggle_button::signal_handler_mouse_enter, this, _2, _3));
	connect_signal<event::MOUSE_LEAVE>(boost::bind(
				&ttoggle_button::signal_handler_mouse_leave, this, _2, _3));

	connect_signal<event::LEFT_BUTTON_CLICK>(boost::bind(
				&ttoggle_button::signal_handler_left_button_click
					, this, _2, _3, _4));
	connect_signal<event::LEFT_BUTTON_DOUBLE_CLICK>(boost::bind(
				&ttoggle_button::signal_handler_left_button_double_click
					, this, _2, _3));
}

void ttoggle_button::set_members(const string_map& data)
{
	// Inherit
	tcontrol::set_members(data);

	string_map::const_iterator itor = data.find("icon");
	if(itor != data.end()) {
		set_icon_name(itor->second);
	}
}

void ttoggle_button::set_active(const bool active)
{
	if(active) {
		if(get_value()) {
			set_state(ENABLED_SELECTED);
		} else {
			set_state(ENABLED);
		}
	} else {
		if(get_value()) {
			set_state(DISABLED_SELECTED);
		} else {
			set_state(DISABLED);
		}
	}
}

void ttoggle_button::update_canvas()
{
	// Inherit.
	tcontrol::update_canvas();

	// set icon in canvases
	std::vector<tcanvas>& canvases = tcontrol::canvas();
	BOOST_FOREACH(tcanvas& canvas, canvases) {
		canvas.set_variable("icon", variant(icon_name_));
	}

	set_dirty();
}

void ttoggle_button::set_value(const bool selected)
{
	if (selected == get_value()) {
		return;
	}

	if (selected) {
		set_state(static_cast<tstate>(state_ + ENABLED_SELECTED));
	} else {
		set_state(static_cast<tstate>(state_ - ENABLED_SELECTED));
	}
}

void ttoggle_button::set_retval(const int retval)
{
	if (retval == retval_) {
		return;
	}

	retval_ = retval;
	set_wants_mouse_left_double_click(retval_ != 0);
}

void ttoggle_button::set_state(const tstate state)
{
	if(state != state_) {
		state_ = state;
		set_dirty(true);
	}
}

bool ttoggle_button::can_selectable() const
{
	return get_active() && get_visible() == twidget::VISIBLE;
}

const std::string& ttoggle_button::get_control_type() const
{
	static const std::string type = "toggle_button";
	return type;
}

void ttoggle_button::signal_handler_mouse_enter(
		const event::tevent event, bool& handled)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";
	if (!get_active()) {
		handled = true;
		return;
	}

	if(get_value()) {
		set_state(FOCUSSED_SELECTED);
	} else {
		set_state(FOCUSSED);
	}
	handled = true;
}

void ttoggle_button::signal_handler_mouse_leave(
		const event::tevent event, bool& handled)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";
	if (!get_active()) {
		handled = true;
		return;
	}

	if(get_value()) {
		set_state(ENABLED_SELECTED);
	} else {
		set_state(ENABLED);
	}
	handled = true;
}

void ttoggle_button::signal_handler_left_button_click(
		const event::tevent event, bool& handled, bool& halt)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";

	if (radio_ && get_value()) {
		return;
	}

	// sound::play_UI_sound(settings::sound_toggle_button_click);

	if (callback_state_pre_change_) {
		if (!callback_state_pre_change_(this)) {
			handled = true;
			halt = true;
			return;
		}
	}

	if (get_value()) {
		set_state(ENABLED);
	} else {
		set_state(ENABLED_SELECTED);
	}

	if (callback_state_change_) {
		callback_state_change_(this);
	}
	handled = true;
}

void ttoggle_button::signal_handler_left_button_double_click(
		const event::tevent event, bool& handled)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";

	if (retval_ == 0) {
		return;
	}

	twindow* window = get_window();
	assert(window);

	window->set_retval(retval_);

	handled = true;
}
} // namespace gui2

