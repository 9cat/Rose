/* $Id: window.hpp 54906 2012-07-29 19:52:01Z mordante $ */
/*
   Copyright (C) 2007 - 2012 by Mark de Wever <koraq@xs4all.nl>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

/**
 *  @file
 *  This file contains the window object, this object is a top level container
 *  which has the event management as well.
 */

#ifndef GUI_WIDGETS_WINDOW_HPP_INCLUDED
#define GUI_WIDGETS_WINDOW_HPP_INCLUDED

#include "cursor.hpp"
#include "gui/auxiliary/formula.hpp"
#include "gui/widgets/helper.hpp"
#include "gui/widgets/panel.hpp"

#include "events.hpp"
#include "SDL.h"

#include <string>
#include <boost/function.hpp>

class CVideo;

namespace gui2{

class tdialog;

namespace event {
	class tdistributor;
} // namespace event

class ttransition
{
public:
	enum { fade_none, fade_left = 0x1, fade_right = 0x2, fade_up = 0x4, fade_down = 0x8};

	static const int normal_duration;
	ttransition()
		: transition_start_time_(twidget::npos)
		, transition_fade_(fade_none)
		, transition_duration_(0)
		// , transition_last_offset_(twidget::npos)
	{}

	void set_transition_fade(int val) { transition_fade_ = val; }
	void set_transition(surface& surf, int start, int duration = normal_duration);

protected:
	surface transition_surf_;
	surface transition_frame_buffer_;
	int transition_fade_;
	int transition_start_time_;
	int transition_duration_;
	// int transition_last_offset_;
};

/**
 * base class of top level items, the only item
 * which needs to store the final canvases to draw on
 */
class twindow
	: public tpanel
	, public cursor::setter
	, public ttransition
{
	friend twindow* build(CVideo&, const twindow_builder::tresolution*, const unsigned, const unsigned);
	friend class tinvalidate_layout_blocker;

public:

	static bool set_orientation_resolution();
	static void enter_orientation(torientation orientation);
	static void recover_landscape(bool original_landscape);

	struct tlayout_exception 
	{
		tlayout_exception(const twindow& target, const std::string& reason) 
			: target(target)
			, reason(reason)
		{}

		const twindow& target;
		const std::string reason;
	};

	twindow(CVideo& video,
		tformula<unsigned>x,
		tformula<unsigned>y,
		tformula<unsigned>w,
		tformula<unsigned>h,
		const bool automatic_placement,
		const unsigned horizontal_placement,
		const unsigned vertical_placement,
		const std::string& definition,
		const bool theme,
		const torientation orientation,
		const unsigned explicit_x,
		const unsigned explicit_y);

	~twindow();

	static const std::string visual_layout_id;
	static surface last_frame_buffer;

	/**
	 * Update the size of the screen variables in settings.
	 *
	 * Before a window gets build the screen sizes need to be updated. This
	 * function does that. It's only done when no other window is active, if
	 * another window is active it already updates the sizes with it's resize
	 * event.
	 */
	static void update_screen_size();

	/**
	 * Returns the instance of a window.
	 *
	 * @param handle              The instance id of the window.
	 *
	 * @returns                   The window or NULL.
	 */
	static twindow* window_instance(const unsigned handle);

	/**
	 * Default return values.
	 *
	 * These values are named return values and most are assigned to a widget
	 * automatically when using a certain id for that widget. The automatic
	 * return values are always a negative number.
	 *
	 * Note this might be moved somewhere else since it will force people to
	 * include the button, while it should be and implementation detail for most
	 * callers.
	 */
	enum tretval {
		NONE = 0,                      /**<
										* Dialog is closed with no return
										* value, should be rare but eg a
										* message popup can do it.
										*/
		OK = -1,                       /**< Dialog is closed with ok button. */
		CANCEL = -2,                   /**<
										* Dialog is closed with the cancel
										* button.
										*/
		AUTO_CLOSE = -3                /**<
		                                * The dialog is closed automatically
		                                * since it's timeout has been
		                                * triggered.
		                                */
		};

	/** Gets the retval for the default buttons. */
	static tretval get_retval_by_id(const std::string& id);

	/**
	 * @todo Clean up the show functions.
	 *
	 * the show functions are a bit messy and can use a proper cleanup.
	 */

	/**
	 * Shows the window.
	 *
	 * @param restore             Restore the screenarea the window was on
	 *                            after closing it?
	 * @param auto_close_timeout  The time in ms after which the window will
	 *                            automatically close, if 0 it doesn't close.
	 *                            @note the timeout is a minimum time and
	 *                            there's no quarantee about how fast it closes
	 *                            after the minimum.
	 *
	 * @returns                   The close code of the window, predefined
	 *                            values are listed in tretval.
	 */
	int show(const bool restore = true,
			const unsigned auto_close_timeout = 0);

	int asyn_show();

	/**
	 * Shows the window as a tooltip.
	 *
	 * A tooltip can't be interacted with and is just shown.
	 *
	 * @todo implement @p auto_close_timeout.
	 *
	 * @param auto_close_timeout  The time in ms after which the window will
	 *                            automatically close, if 0 it doesn't close.
	 *                            @note the timeout is a minimum time and
	 *                            there's no guarantee about how fast it closes
	 *                            after the minimum.
	 */
	void show_tooltip(/*const unsigned auto_close_timeout = 0*/);

	/**
	 * Shows the window non modal.
	 *
	 * A tooltip can be interacted with unlike the tooltip.
	 *
	 * @todo implement @p auto_close_timeout.
	 *
	 * @param auto_close_timeout  The time in ms after which the window will
	 *                            automatically close, if 0 it doesn't close.
	 *                            @note the timeout is a minimum time and
	 *                            there's no guarantee about how fast it closes
	 *                            after the minimum.
	 */
	void show_non_modal(/*const unsigned auto_close_timeout = 0*/);

	/**
	 * Draws the window.
	 *
	 * This routine draws the window if needed, it's called from the event
	 * handler. This is done by a drawing event. When a window is shown it
	 * manages an SDL timer which fires a drawing event every X milliseconds,
	 * that event calls this routine. Don't call it manually.
	 */
	void draw();

	/**
	 * Undraws the window.
	 */
	void undraw();

	/**
	 * Adds an item to the dirty_list_.
	 *
	 * @param call_stack          The list of widgets traversed to get to the
	 *                            dirty widget.
	 */
	void add_to_dirty_list(const std::vector<twidget*>& call_stack)
	{
		dirty_list_.push_back(call_stack);
	}

	/** The status of the window. */
	enum tstatus{
		NEW,                      /**< The window is new and not yet shown. */
		SHOWING,                  /**< The window is being shown. */
		REQUEST_CLOSE,            /**< The window has been requested to be
		                           *   closed but still needs to evaluate the
								   *   request.
								   */
		CLOSED                    /**< The window has been closed. */
		};

	/**
	 * Requests to close the window.
	 *
	 * At the moment the request is always honored but that might change in the
	 * future.
	 */
	void close() { status_ = REQUEST_CLOSE; }

	/**
	 * Helper class to block invalidate_layout.
	 *
	 * Some widgets can handling certain layout aspects without help. For
	 * example a listbox can handle hiding and showing rows without help but
	 * setting the visibility calls invalidate_layout(). When this blocker is
	 * Instantiated the call to invalidate_layout() becomes a nop.
	 *
	 * @note The class can't be used recursively.
	 */
	class tinvalidate_layout_blocker
	{
	public:
		tinvalidate_layout_blocker(twindow& window);
		~tinvalidate_layout_blocker();
	private:
		twindow& window_;
		bool invalidate_layout_blocked_;
	};

	/**
	 * Updates the size of the window.
	 *
	 * If the window has automatic placement set this function recalculates the
	 * window. To be used after creation and after modification or items which
	 * can have different sizes eg listboxes.
	 */
	void invalidate_layout();

	/** Inherited from tevent_handler. */
	twindow& get_window() { return *this; }

	/** Inherited from tevent_handler. */
	const twindow& get_window() const { return *this; }

	/** Inherited from tevent_handler. */
	twidget* find_at(const tpoint& coordinate, const bool must_be_active)
		{ return tpanel::find_at(coordinate, must_be_active); }

	/** Inherited from tevent_handler. */
	const twidget* find_at(const tpoint& coordinate,
			const bool must_be_active) const
		{ return tpanel::find_at(coordinate, must_be_active); }

	/** Inherited from twidget. */
	tdialog* dialog() { return owner_; }

	/** Inherited from tcontainer_. */
	twidget* find(const std::string& id, const bool must_be_active)
		{ return tcontainer_::find(id, must_be_active); }

	/** Inherited from tcontainer_. */
	const twidget* find(const std::string& id,
			const bool must_be_active) const
		{ return tcontainer_::find(id, must_be_active); }

	/**
	 * Does the window close easily?
	 *
	 * The behavior can change at run-time, but that might cause oddities
	 * with the easy close button (when one is needed).
	 *
	 * @returns                   Whether or not the window closes easily.
	 */
	bool does_click_dismiss() const
	{
		return click_dismiss_ && !disable_click_dismiss();
	}

	/**
	 * Disable the enter key.
	 *
	 * This is added to block dialogs from being closed automatically.
	 *
	 * @todo this function should be merged with the hotkey support once
	 * that has been added.
	 */
	void set_enter_disabled(const bool enter_disabled)
		{ enter_disabled_ = enter_disabled; }

	/**
	 * Disable the escape key.
	 *
	 * This is added to block dialogs from being closed automatically.
	 *
	 * @todo this function should be merged with the hotkey support once
	 * that has been added.
	 */
	void set_escape_disabled(const bool escape_disabled)
		{ escape_disabled_ = escape_disabled; }

	/**
	 * Initializes a linked size group.
	 *
	 * Note at least one of fixed_width or fixed_height must be true.
	 *
	 * @param id                  The id of the group.
	 * @param fixed_width         Does the group have a fixed width?
	 * @param fixed_height        Does the group have a fixed height?
	 */
	void init_linked_size_group(const std::string& id,
			const bool fixed_width, const bool fixed_height, bool radio = false);

	/**
	 * Is the linked size group defined for this window?
	 *
	 * @param id                  The id of the group.
	 *
	 * @returns                   True if defined, false otherwise.
	 */
	bool has_linked_size_group(const std::string& id);

	/**
	 * Adds a widget to a linked size group.
	 *
	 * The group needs to exist, which is done by calling
	 * init_linked_size_group. A widget may only be member of one group.
	 * @todo Untested if a new widget is added after showing the widgets.
	 *
	 * @param id                  The id of the group.
	 * @param widget              The widget to add to the group.
	 */
	void add_linked_widget(const std::string& id, twidget* widget);

	/**
	 * Removes a widget from a linked size group.
	 *
	 * The group needs to exist, which is done by calling
	 * init_linked_size_group. If the widget is no member of the group the
	 * function does nothing.
	 *
	 * @param id                  The id of the group.
	 * @param widget              The widget to remove from the group.
	 */
	void remove_linked_widget(const std::string& id, const twidget* widget);

	/***** ***** ***** setters / getters for members ***** ****** *****/

	CVideo& video() { return video_; }

	/**
	 * Sets there return value of the window.
	 *
	 * @param retval              The return value for the window.
	 * @param close_window        Close the window after setting the value.
	 */
	void set_retval(const int retval, const bool close_window = true)
		{ retval_ = retval; if(close_window) close(); }

	void set_owner(tdialog* owner) { owner_ = owner; }

	void set_click_dismiss(const bool click_dismiss)
	{
		click_dismiss_ = click_dismiss;
	}

	static void set_sunset(const unsigned interval)
		{ sunset_ = interval ? interval : 5; }

	bool get_need_layout() const { return need_layout_; }

	void set_variable(const std::string& key, const variant& value)
	{
		variables_.add(key, value);
		set_dirty();
	}
	const game_logic::map_formula_callable& variables() const { return variables_; }

	std::vector<std::vector<twidget*> >& dirty_list();
	std::vector<twidget*> set_fix_coordinate(const SDL_Rect& map_area);
	bool is_theme() const { return fix_coordinate_; }
	torientation get_orientation() const { return orientation_; }

	/**
	 * Layouts the linked widgets.
	 *
	 * @see layout_algorithm for more information.
	 */
	void layout_linked_widgets(const twidget* parent);

	/** Inherited from twidget. */
	tpoint calculate_best_size() const;

	/** Inherited from tcontrol. */
	void impl_draw_background(
			  surface& frame_buffer
			, int x_offset
			, int y_offset);

	/** Inherited from tcontrol. */
	void impl_draw_foreground(
			  surface& frame_buffer
			, int x_offset
			, int y_offset);

	/** Inherited from twidget. */
	void impl_draw_children(surface& frame_buffer, int x_offset, int y_offset);

	/**
	 * Layouts the window.
	 *
	 * This part does the pre and post processing for the actual layout
	 * algorithm.
	 *
	 * @see layout_algorithm for more information.
	 */
	void layout();

	void insert_tooltip(const std::string& msg, const twidget& widget);
	void draw_tooltip(surface& screen);
	void undraw_tooltip(surface& screen);
	void remove_tooltip();
	const SDL_Rect& tooltip_rect() const { return tooltip_rect_; }
	bool has_tooltip() const { return !!tooltip_surf_; }

	void set_layer_style(bool val) { layer_style_ = val; }
	bool layer_draging() const { return layer_draging_; }
	void set_layer_draging(bool val);
private:

	/** Needed so we can change what's drawn on the screen. */
	CVideo& video_;

	/** The status of the window. */
	tstatus status_;

	enum tshow_mode {
		  none
		, modal
		, tooltip
	};

	/**
	 * The mode in which the window is shown.
	 *
	 * This is used to determine whether or not to remove the tip.
	 */
	tshow_mode show_mode_;

	// return value of the window, 0 default.
	int retval_;

	/** The dialog that owns the window. */
	tdialog* owner_;

	/**
	 * When set the form needs a full layout redraw cycle.
	 *
	 * This happens when either a widget changes it's size or visibility or
	 * the window is resized.
	 */
	bool need_layout_;

	/** The variables of the canvas. */
	game_logic::map_formula_callable variables_;

	/** Is invalidate layout blocked see tinvalidate_layout_blocker. */
	bool invalidate_layout_blocked_;

	/** Avoid drawing the window.  */
	bool suspend_drawing_;

	/** When the window closes this surface is used to undraw the window. */
	surface restorer_;

	/** Do we wish to place the widget automatically? */
	const bool automatic_placement_;

	/**
	 * Sets the horizontal placement.
	 *
	 * Only used if automatic_placement_ is true.
	 * The value should be a tgrid placement flag.
	 */
	const unsigned horizontal_placement_;

	/**
	 * Sets the vertical placement.
	 *
	 * Only used if automatic_placement_ is true.
	 * The value should be a tgrid placement flag.
	 */
	const unsigned vertical_placement_;

	unsigned explicit_x_;
	unsigned explicit_y_;

	/** The formula to calulate the x value of the dialog. */
	tformula<unsigned>x_;

	/** The formula to calulate the y value of the dialog. */
	tformula<unsigned>y_;

	/** The formula to calulate the width of the dialog. */
	tformula<unsigned>w_;

	/** The formula to calulate the height of the dialog. */
	tformula<unsigned>h_;

	/**
	 * Do we want to have easy close behavior?
	 *
	 * Easy closing means that whenever a mouse click is done the dialog will
	 * be closed. The widgets in the window may override this behavior by
	 * registering themselves as blockers. This is tested by the function
	 * disable_click_dismiss().
	 *
	 * The handling of easy close is done in the window, in order to do so a
	 * window either needs a click_dismiss or an ok button. Both will be hidden
	 * when not needed and when needed first the ok is tried and then the
	 * click_dismiss button. this allows adding a click_dismiss button to the
	 * window definition and use the ok from the window instance.
	 *
	 * @todo After testing the click dismiss feature it should be documented in
	 * the wiki.
	 */
	bool click_dismiss_;

	/** Disable the enter key see our setter for more info. */
	bool enter_disabled_;

	/** Disable the escape key see our setter for more info. */
	bool escape_disabled_;

	/**
	 * Controls the sunset feature.
	 *
	 * If this value is not 0 it will darken the entire screen every
	 * sunset_th drawing request that nothing has been modified. It's a debug
	 * feature.
	 */
	static unsigned sunset_;

	/**
	 * Helper struct to force widgets the have the same size.
	 *
	 * Widget which are linked will get the same width and/or height. This
	 * can especially be useful for listboxes, but can also be used for other
	 * applications.
	 */
	struct tlinked_size
	{
		tlinked_size(const bool width = false, const bool height = false, bool radio = false)
			: widgets()
			, width(width)
			, height(height)
			, radio(radio)
		{
		}

		/** The widgets linked. */
		std::vector<twidget*> widgets;

		/** Link the widgets in the width? */
		bool width;

		/** Link the widgets in the height? */
		bool height;

		/** this id is from radio config*/
		bool radio;
	};

	/** List of the widgets, whose size are linked together. */
	std::map<std::string, tlinked_size> linked_size_;

	const twindow_builder::tresolution* definition_;

	/** Inherited from tevent_handler. */
	bool click_dismiss();

	/** Inherited from tcontrol. */
	const std::string& get_control_type() const;

	/**
	 * The list with dirty items in the window.
	 *
	 * When drawing only the widgets that are dirty are updated. The draw()
	 * function has more information about the dirty_list_.
	 */
	std::vector<std::vector<twidget*> > dirty_list_;

	tristate bg_opaque_;
	bool fix_coordinate_;
	torientation orientation_;
	bool original_landscape_;

	SDL_Rect tooltip_rect_;
	surface tooltip_surf_;
	surface tooltip_buf_;

	bool layer_style_;
	surface owner_surf_;
	surface layer_background_surf_;
	bool layer_draging_;

	bool drawn_; // true: first drawn
	/**
	 * Finishes the initialization of the grid.
	 *
	 * @param content_grid        The new contents for the content grid.
	 */
	void finalize(const boost::intrusive_ptr<tbuilder_grid>& content_grid);

	event::tdistributor* event_distributor_;

public:
	// mouse and keyboard_capture should be renamed and stored in the
	// dispatcher. Chaining probably should remain exclusive to windows.
	void mouse_capture(const bool capture = true);
	void keyboard_capture(twidget* widget);

	/**
	 * Adds the widget to the keyboard chain.
	 *
	 * @todo rename to keyboard_add_to_chain.
	 * @param widget              The widget to add to the chain. The widget
	 *                            should be valid widget, which hasn't been
	 *                            added to the chain yet.
	 */
	void add_to_keyboard_chain(twidget* widget);

	/**
	 * Remove the widget from the keyboard chain.
	 *
	 * @todo rename to keyboard_remove_from_chain.
	 *
	 * @param widget              The widget to be removed from the chain.
	 */
	void remove_from_keyboard_chain(twidget* widget);

private:

	/***** ***** ***** signal handlers ***** ****** *****/

	void signal_handler_sdl_video_resize(
			const event::tevent event, bool& handled, const tpoint& new_size);

	void signal_handler_click_dismiss(
			const event::tevent event, bool& handled, bool& halt);

	void signal_handler_click_dismiss2(
			const event::tevent event, bool& handled, bool& halt, const tpoint& coordinate);

	void signal_handler_sdl_key_down(
			const event::tevent event, bool& handled, const SDLKey key);

	void signal_handler_message_show_tooltip(
			  const event::tevent event
			, bool& handled
			, event::tmessage& message);

	void signal_handler_request_placement(
			  const event::tevent event
			, bool& handled);
};

template <class D, class W, void (D::*fptr)(twindow&, W&)>
void dialog_callback2(twidget* caller)
{
	D* dialog = dynamic_cast<D*>(caller->dialog());
	twindow* window = dynamic_cast<twindow*>(caller->get_window());
	(dialog->*fptr)(*window, *dynamic_cast<W*>(caller));
}

template <class D, class W, void (D::*fptr)(twindow&, W&, const int)>
void dialog_callback3(twidget* caller, const int type)
{
	D* dialog = dynamic_cast<D*>(caller->dialog());
	twindow* window = dynamic_cast<twindow*>(caller->get_window());
	(dialog->*fptr)(*window, *dynamic_cast<W*>(caller), type);
}

} // namespace gui2

#endif
