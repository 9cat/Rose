/* $Id: button.hpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
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

#ifndef GUI_WIDGETS_BUTTON_HPP_INCLUDED
#define GUI_WIDGETS_BUTTON_HPP_INCLUDED

#include "gui/widgets/control.hpp"
#include "gui/widgets/clickable.hpp"

namespace gui2 {

/**
 * Simple push button.
 */
class tbutton
	: public tcontrol
	, public tclickable_
{
public:
	tbutton();

	/**
	 * Possible sort mode of the widget.
	 *
	 * Note the order of the states must be the same as defined in settings.hpp.
	 */
	enum tsort { NONE, ASCEND, DESCEND, TOGGLE };

	/***** ***** ***** ***** Inherited ***** ***** ***** *****/

	/** Inherited from tcontrol. */
	void load_config_extra();

	void set_active(const bool active);
		

	/** Inherited from tcontrol. */
	bool get_active() const { return state_ != DISABLED; }

	/** Inherited from tcontrol. */
	unsigned get_state() const { return state_; }

	/** Inherited from tclickable. */
	void connect_click_handler(const event::tsignal_function& signal)
	{
		connect_signal_mouse_left_click(*this, signal);
	}

	/** Inherited from tclickable. */
	void disconnect_click_handler(const event::tsignal_function& signal)
	{
		disconnect_signal_mouse_left_click(*this, signal);
	}

	/***** ***** ***** setters / getters for members ***** ****** *****/

	void set_retval(const int retval) { retval_ = retval; }

	void set_sort(tsort sort);
	int get_sort() const;
	void set_sound_button_click(const std::string& sound) { sound_button_click_ = sound; }

	void set_surface(const surface& surf, int w, int h);
private:

	/**
	 * Possible states of the widget.
	 *
	 * Note the order of the states must be the same as defined in settings.hpp.
	 */
	enum tstate { ENABLED, DISABLED, PRESSED, FOCUSSED, COUNT };

	void set_state(const tstate state);
	/**
	 * Current state of the widget.
	 *
	 * The state of the widget determines what to render and how the widget
	 * reacts to certain 'events'.
	 */
	tstate state_;

	/**
	 * Current sort mode of the widget.
	 *
	 * The sort mode of the widget determines what to sort and how the widget
	 * reacts to certain 'events'.
	 */
	tsort sort_;

	/**
	 * The return value of the button.
	 *
	 * If this value is not 0 and the button is clicked it sets the retval of
	 * the window and the window closes itself.
	 */
	int retval_;

	std::string sound_button_click_;

	/** Inherited from tcontrol. */
	const std::string& get_control_type() const;

	/***** ***** ***** signal handlers ***** ****** *****/

	void signal_handler_mouse_enter(const event::tevent event, bool& handled);

	void signal_handler_mouse_leave(const event::tevent event, bool& handled);

	void signal_handler_left_button_down(
			const event::tevent event, bool& handled);

	void signal_handler_left_button_up(
			const event::tevent event, bool& handled);

	void signal_handler_left_button_click(
			const event::tevent event, bool& handled);
};

tbutton* create_button(const config& cfg);
tbutton* create_button(const std::string& id, const std::string& definition, void* cookie);
tbutton* create_surface_button(const std::string& id, void* cookie);
} // namespace gui2

#endif

