/* $Id: dialog.hpp 50956 2011-08-30 19:41:22Z mordante $ */
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

#ifndef GUI_DIALOGS_DIALOG_HPP_INCLUDED
#define GUI_DIALOGS_DIALOG_HPP_INCLUDED

#include "SDL_rect.h"
#include "gui/widgets/widget.hpp"

#include <string>
#include <vector>

class CVideo;

namespace gui2 {

/**
 * Registers a window.
 *
 * This function registers a window. The registration is used to validate
 * whether the config for the window exists when starting Wesnoth.
 *
 * @note Most of the time you want to call @ref REGISTER_DIALOG instead of this
 * function. It also directly adds the code for the dialog's id function.
 *
 * @param id                      Id of the window, multiple dialogs can use
 *                                the same window so the id doesn't need to be
 *                                unique.
 */
#define REGISTER_WINDOW(                                                   \
		  id)                                                              \
namespace {                                                                \
                                                                           \
	namespace ns_##id {                                                    \
                                                                           \
		struct tregister_helper {                                          \
			tregister_helper()                                             \
			{                                                              \
				register_window(#id);                                      \
			}                                                              \
		};                                                                 \
                                                                           \
		tregister_helper register_helper;                                  \
	}                                                                      \
}

/**
 * Registers a window for a dialog.
 *
 * Call this function to register a window. In the header of the class it adds
 * the following code:
 *@code
 *  // Inherited from tdialog, implemented by REGISTER_DIALOG.
 *	virtual const std::string& id() const;
 *@endcode
 * Then use this macro in the implementation, inside the gui2 namespace.
 *
 * @note When the @p id is "foo" and the type tfoo it's easier to use
 * REGISTER_DIALOG(foo).
 *
 * @param type                    Class type of the window to register.
 * @param id                      Id of the window, multiple dialogs can use
 *                                the same window so the id doesn't need to be
 *                                unique.
 */
#define REGISTER_DIALOG2(                                                  \
		  type                                                             \
		, id)                                                              \
                                                                           \
REGISTER_WINDOW(id)                                                        \
                                                                           \
const std::string&                                                         \
type::window_id() const                                                    \
{                                                                          \
	static const std::string result(#id);                                  \
	return result;                                                         \
}

/**
 * Wrapper for REGISTER_DIALOG2.
 *
 * "Calls" REGISTER_DIALOG2(twindow_id, window_id)
 */
#define REGISTER_DIALOG(window_id) REGISTER_DIALOG2(t##window_id, window_id)

/**
 * Abstract base class for all dialogs.
 *
 * A dialog shows a certain window instance to the user. The subclasses of this
 * class will hold the parameters used for a certain window, eg a server
 * connection dialog will hold the name of the selected server as parameter that
 * way the caller doesn't need to know about the 'contents' of the window.
 *
 * @par Usage
 *
 * Simple dialogs that are shown to query user information it is recommended to
 * add a static member called @p execute. The parameters to the function are:
 * - references to in + out parameters by reference
 * - references to the in parameters
 * - the parameters for @ref tdialog::show.
 *
 * The 'in + out parameters' are used as initial value and final value when the
 * OK button is pressed. The 'in parameters' are just extra parameters for
 * showing.
 *
 * When a function only has 'in parameters' it should return a void value and
 * the function should be called @p display, if it has 'in + out parameters' it
 * must return a bool value. This value indicates whether or not the OK button
 * was pressed to close the dialog. See @ref teditor_new_map::execute for an
 * example.
 */
class tdialog
{
	/**
	 * Special helper function to get the id of the window.
	 *
	 * This is used in the unit tests, but these implementation details
	 * shouldn't be used in the normal code.
	 */
	friend std::string unit_test_mark_as_tested(const tdialog& dialog);

public:
	tdialog() :
		retval_(0),
		focus_(),
		restore_(true),
		async_window_(NULL)
	{}

	virtual ~tdialog();

	/**
	 * Shows the window.
	 *
	 * @param video               The video which contains the surface to draw
	 *                            upon.
	 * @param auto_close_time     The time in ms after which the dialog will
	 *                            automatically close, if 0 it doesn't close.
	 *                            @note the timeout is a minimum time and
	 *                            there's no quarantee about how fast it closes
	 *                            after the minimum.
	 *
	 * @returns                   Whether the final retval_ == twindow::OK
	 */
	bool show(CVideo& video, const unsigned auto_close_time = 0, const unsigned explicit_x = 0, const unsigned explicit_y = 0);

	void asyn_show(CVideo& video, const SDL_Rect& map_area);
	void async_draw();
	const std::vector<gui2::twidget*>& volatiles() const { return volatiles_; }


	/***** ***** ***** setters / getters for members ***** ****** *****/

	int get_retval() const { return retval_; }

	void set_restore(const bool restore) { restore_ = restore; }

	virtual bool pre_toggle_tabbar(twidget* widget, twidget* previous) { return true; }
	virtual void toggle_report(twidget* widget);
	virtual bool click_report(twidget* widget) { return false; }

	virtual void destruct_widget(const twidget* widget) {}
	virtual void first_drawn(twindow& window) {};
	virtual void draw_layer(surface& frame_buffer, surface& bg_surf, surface& owner_surf) {}

protected:
	twindow* async_window_;
	std::vector<twidget*> volatiles_;

private:
	/** Returns the window exit status, 0 means not shown. */
	int retval_;

	/**
	 * Contains the widget that should get the focus when the window is shown.
	 */
	std::string focus_;

	/**
	 * Restore the screen after showing?
	 *
	 * Most windows should restore the display after showing so this value
	 * defaults to true. Toplevel windows (like the titlescreen don't want this
	 * behaviour so they can change it in pre_show().
	 */
	bool restore_;

	/** The id of the window to build. */
	virtual const std::string& window_id() const = 0;

	/**
	 * Builds the window.
	 *
	 * Every dialog shows it's own kind of window, this function should return
	 * the window to show.
	 *
	 * @param video               The video which contains the surface to draw
	 *                            upon.
	 * @returns                   The window to show.
	 */
	twindow* build_window(CVideo& video, const unsigned explicit_x, const unsigned explicit_y) const;

	/**
	 * Actions to be taken directly after the window is build.
	 *
	 * At this point the registered fields are not yet registered.
	 *
	 * @param video               The video which contains the surface to draw
	 *                            upon.
	 * @param window              The window just created.
	 */
	virtual void post_build(CVideo& /*video*/, twindow& /*window*/) {}

	/**
	 * Actions to be taken before showing the window.
	 *
	 * At this point the registered fields are registered and initialized with
	 * their initial values.
	 *
	 * @param video               The video which contains the surface to draw
	 *                            upon.
	 * @param window              The window to be shown.
	 */
	virtual void pre_show(CVideo& /*video*/, twindow& /*window*/) {}

	/**
	 * Actions to be taken after the window has been shown.
	 *
	 * At this point the registered fields already stored their values (if the
	 * OK has been pressed).
	 *
	 * @param window              The window which has been shown.
	 */
	virtual void post_show(twindow& /*window*/) {}

	/**
	 * Initializes all fields in the dialog and set the keyboard focus.
	 *
	 * @param window              The window which has been shown.
	 */
	virtual void init_fields(twindow& window);
};

class button_action
{
public:
	virtual ~button_action() {}

	virtual int button_pressed(int menu_selection) = 0;
};

struct tval_str {
	tval_str(int v, const std::string& s)
		: val(v), str(s)
	{}
	int val;
	std::string str;
};

} // namespace gui2

#endif

