/* $Id: scrollbar_container.hpp 54007 2012-04-28 19:16:10Z mordante $ */
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

#ifndef GUI_WIDGETS_SCROLLBAR_CONTAINER_HPP_INCLUDED
#define GUI_WIDGETS_SCROLLBAR_CONTAINER_HPP_INCLUDED

#include "gui/auxiliary/notifiee.hpp"
#include "gui/widgets/container.hpp"
#include "gui/widgets/scrollbar.hpp"

namespace gui2 {

class tspacer;

namespace implementation {
	struct tbuilder_scroll_label;
	struct tbuilder_scroll_text_box;
	struct tbuilder_report;
	struct tbuilder_scrollbar_panel;
}

/**
 * Base class for creating containers with one or two scrollbar(s).
 *
 * For now users can't instanciate this class directly and needs to use small
 * wrapper classes. Maybe in the future users can use the class directly.
 *
 * @todo events are not yet send to the content grid.
 */
class tscrollbar_container: public tcontainer_
{
	friend struct implementation::tbuilder_scroll_label;
	friend struct implementation::tbuilder_scroll_text_box;
	friend struct implementation::tbuilder_report;
	friend struct implementation::tbuilder_scrollbar_panel;
	friend class tlistbox;
	friend class ttree_view;
	friend class tscroll_label;
	friend class tscroll_text_box;
	friend class treport;
	friend class tscrollbar_panel;
	friend struct tscrollbar_container_implementation;

public:
	class tcalculte_reduce_lock
	{
	public:
		tcalculte_reduce_lock(tscrollbar_container& container)
			: container_(container)
			, original_(container.calculate_reduce_)
		{
			container_.calculate_reduce_ = true;
		}
		~tcalculte_reduce_lock()
		{
			container_.calculate_reduce_ = original_;
		}

	private:
		tscrollbar_container& container_;
		bool original_;
	};

	explicit tscrollbar_container(const unsigned canvas_count, bool listbox = false);

	~tscrollbar_container() { delete content_grid_; }

	/** The way to handle the showing or hiding of the scrollbar. */
	enum tscrollbar_mode {
		always_invisible,         /**<
		                           * The scrollbar is never shown even not
		                           * when needed. There's also no space
		                           * reserved for the scrollbar.
		                           */
		auto_visible,             /**<
		                           * The scrollbar is shown when the number of
		                           * items is larger as the visible items. The
		                           * space for the scrollbar is always
		                           * reserved, just in case it's needed after
		                           * the initial sizing (due to adding items).
		                           */
	};

	/***** ***** ***** ***** layout functions ***** ***** ***** *****/

	/** Inherited from tcontainer_. */
	void layout_init(const bool full_initialization);

	/** Inherited from tcontainer_. */
	bool can_wrap() const
	{
		// Note this function is called before the object is finalized.
		return content_grid_
			? content_grid_->can_wrap()
			: false;
	}

	const tgrid& layout_grid() const { return *content_grid_; }

private:
	/** Inherited from tcontainer_. */
	tpoint calculate_best_size() const;

	virtual void adjust_offset(int& x_offset, int& y_offset) {}
	virtual void set_content_grid_origin(const tpoint& origin, const tpoint& content_origin);
	virtual void set_content_grid_visible_area(const SDL_Rect& area);
public:

	/** Inherited from tcontainer_. */
	void place(const tpoint& origin, const tpoint& size);

	/** Inherited from tcontainer_. */
	void set_origin(const tpoint& origin);

	/** Inherited from tcontainer_. */
	void set_visible_area(const SDL_Rect& area);

	/***** ***** ***** inherited ****** *****/

	/** Inherited from tcontainer_. */
	bool get_active() const { return state_ != DISABLED; }

	/** Inherited from tcontainer_. */
	unsigned get_state() const { return state_; }

	/** Inherited from tcontainer_. */
	twidget* find_at(const tpoint& coordinate, const bool must_be_active);

	/** Inherited from tcontainer_. */
	const twidget* find_at(const tpoint& coordinate, const bool must_be_active) const;

	/** Inherited from tcontainer_. */
	twidget* find(const std::string& id, const bool must_be_active);

	/** Inherited from tcontrol.*/
	const twidget* find(const std::string& id, const bool must_be_active) const;

	/** Inherited from tcontainer_. */
	bool disable_click_dismiss() const;

	/***** ***** ***** setters / getters for members ***** ****** *****/

	/** @note shouldn't be called after being shown in a dialog. */
	void set_vertical_scrollbar_mode(const tscrollbar_mode scrollbar_mode);
	tscrollbar_mode get_vertical_scrollbar_mode() const
		{ return vertical_scrollbar_mode_; }

	/** @note shouldn't be called after being shown in a dialog. */
	void set_horizontal_scrollbar_mode(const tscrollbar_mode scrollbar_mode);
	tscrollbar_mode get_horizontal_scrollbar_mode() const
		{ return horizontal_scrollbar_mode_; }

	const tgrid* vertical_scrollbar_grid() const { return vertical_scrollbar_grid_; }

	tgrid *content_grid() { return content_grid_; }
	const tgrid *content_grid() const { return content_grid_; }

	const SDL_Rect& content_visible_area() const
		{ return content_visible_area_; }

	/***** ***** ***** scrollbar helpers ***** ****** *****/

	/**
	 * Scrolls the vertical scrollbar.
	 *
	 * @param scroll              The position to scroll to.
	 */
	void scroll_vertical_scrollbar(const tscrollbar_::tscroll scroll);

	/**
	 * Scrolls the horizontal scrollbar.
	 *
	 * @param scroll              The position to scroll to.
	 */
	void scroll_horizontal_scrollbar(const tscrollbar_::tscroll scroll);

	/**
	 * Callback when the scrollbar moves (NOTE maybe only one callback needed).
	 * Maybe also make protected or private and add a friend.
	 */
	void vertical_scrollbar_moved()
		{ scrollbar_moved(); }

	void horizontal_scrollbar_moved()
		{ scrollbar_moved(); }

	void set_scroll_to_end(bool val) { scroll_to_end_ = val; }

	tpoint scrollbar_size(const tgrid& scrollbar_grid, tscrollbar_mode scrollbar_mode) const;

	virtual bool content_empty() const { return false; }
	virtual void invalidate_layout(bool calculate_linked_group);

	std::string generate_layout_str(const int level) const;

protected:
	/**
	 * Shows a certain part of the content.
	 *
	 * When the part to be shown is bigger as the visible viewport the top
	 * left of the wanted rect will be the top left of the viewport.
	 *
	 * @param rect                The rect which should be visible.
	 */
	void show_content_rect(const SDL_Rect& rect);

	/*
	 * The widget contains the following three grids.
	 *
	 * * _vertical_scrollbar_grid containing at least a widget named
	 *   _vertical_scrollbar
	 *
	 * * _horizontal_scrollbar_grid containing at least a widget named
	 *   _horizontal_scrollbar
	 *
	 * * _content_grid a grid which holds the contents of the area.
	 *
	 * NOTE maybe just hardcode these in the finalize phase...
	 *
	 */

	/**
	 * Sets the status of the scrollbar buttons.
	 *
	 * This is needed after the scrollbar moves so the status of the buttons
	 * will be active or inactive as needed.
	 */
	void set_scrollbar_button_status();

protected:

	/***** ***** ***** ***** keyboard functions ***** ***** ***** *****/

	/**
	 * Home key pressed.
	 *
	 * @param modifier            The SDL keyboard modifier when the key was
	 *                            pressed.
	 * @param handled             If the function handles the key it should
	 *                            set handled to true else do not modify it.
	 *                            This is used in the keyboard event
	 *                            changing.
	 */
	virtual void handle_key_home(SDLMod modifier, bool& handled);

	/**
	 * End key pressed.
	 *
	 * @param modifier            The SDL keyboard modifier when the key was
	 *                            pressed.
	 * @param handled             If the function handles the key it should
	 *                            set handled to true else do not modify it.
	 *                            This is used in the keyboard event
	 *                            changing.
	 */
	virtual void handle_key_end(SDLMod modifier, bool& handled);

	/**
	 * Page up key pressed.
	 *
	 * @param modifier            The SDL keyboard modifier when the key was
	 *                            pressed.
	 * @param handled             If the function handles the key it should
	 *                            set handled to true else do not modify it.
	 *                            This is used in the keyboard event
	 *                            changing.
	 */
	virtual void handle_key_page_up(SDLMod modifier, bool& handled);

	/**
	 * Page down key pressed.
	 *
	 * @param modifier            The SDL keyboard modifier when the key was
	 *                            pressed.
	 * @param handled             If the function handles the key it should
	 *                            set handled to true else do not modify it.
	 *                            This is used in the keyboard event
	 *                            changing.
	 */
	virtual void handle_key_page_down(SDLMod modifier, bool& handled);


	/**
	 * Up arrow key pressed.
	 *
	 * @param modifier            The SDL keyboard modifier when the key was
	 *                            pressed.
	 * @param handled             If the function handles the key it should
	 *                            set handled to true else do not modify it.
	 *                            This is used in the keyboard event
	 *                            changing.
	 */
	virtual void handle_key_up_arrow(SDLMod modifier, bool& handled);

	/**
	 * Down arrow key pressed.
	 *
	 * @param modifier            The SDL keyboard modifier when the key was
	 *                            pressed.
	 * @param handled             If the function handles the key it should
	 *                            set handled to true else do not modify it.
	 *                            This is used in the keyboard event
	 *                            changing.
	 */
	virtual void handle_key_down_arrow(SDLMod modifier, bool& handled);

	/**
	 * Left arrow key pressed.
	 *
	 * @param modifier            The SDL keyboard modifier when the key was
	 *                            pressed.
	 * @param handled             If the function handles the key it should
	 *                            set handled to true else do not modify it.
	 *                            This is used in the keyboard event
	 *                            changing.
	 */
	virtual void handle_key_left_arrow(SDLMod modifier, bool& handled);

	/**
	 * Right arrow key pressed.
	 *
	 * @param modifier            The SDL keyboard modifier when the key was
	 *                            pressed.
	 * @param handled             If the function handles the key it should
	 *                            set handled to true else do not modify it.
	 *                            This is used in the keyboard event
	 *                            changing.
	 */
	virtual void handle_key_right_arrow(SDLMod modifier, bool& handled);

	bool calculate_scrollbar(const tpoint& actual_size, const tpoint& desire_size);

	tpoint validate_content_grid_origin(const tpoint& content_origin, const tpoint& content_size, const tpoint& origin, const tpoint& size) const;

protected:
	/** When we're used as a fixed size item, this holds the best size. */
	tformula<unsigned> best_width_;
	tformula<unsigned> best_height_;

	// mutable tpoint content_size_;
	bool listbox_;
	mutable bool size_calculated_;
	bool scroll_to_end_;

	/** The grid that holds the content. */
	tgrid *content_grid_;

	/** Dummy spacer to hold the contents location. */
	tspacer *content_;

	bool need_layout_;
	tpoint keep_content_grid_origin_;

private:

	/**
	 * Possible states of the widget.
	 *
	 * Note the order of the states must be the same as defined in settings.hpp.
	 */
	enum tstate { ENABLED, DISABLED, COUNT };

	/**
	 * Current state of the widget.
	 *
	 * The state of the widget determines what to render and how the widget
	 * reacts to certain 'events'.
	 */
	tstate state_;

	/**
	 * The mode of how to show the scrollbar.
	 *
	 * This value should only be modified before showing, doing it while
	 * showing results in UB.
	 */
	tscrollbar_mode
		vertical_scrollbar_mode_,
		horizontal_scrollbar_mode_;

	/** These are valid after finalize_setup(). */
	tgrid
		*vertical_scrollbar_grid_,
		*horizontal_scrollbar_grid_;

	/** These are valid after finalize_setup(). */
	tscrollbar_
		*vertical_scrollbar_,
		*horizontal_scrollbar_;

	/**
	 * Cache for the visible area for the content.
	 *
	 * The visible area for the content needs to be updated when scrolling.
	 */
	SDL_Rect content_visible_area_;

	bool calculate_reduce_;

	/** The builder needs to call us so we do our setup. */
	void finalize_setup(); // FIXME make protected

	/**
	 * Function for the subclasses to do their setup.
	 *
	 * This function is called at the end of finalize_setup().
	 */
	virtual void finalize_subclass() {}

	/** Inherited from tcontainer_. */
	void layout_children();

	/** Inherited from tcontainer_. */
	void impl_draw_children(surface& frame_buffer, int x_offset, int y_offset);

	void broadcast_frame_buffer(surface& frame_buffer);

	/** Inherited from tcontainer_. */
	void child_populate_dirty_list(twindow& caller,
		const std::vector<twidget*>& call_stack);

	/**
	 * Sets the size of the content grid.
	 *
	 * This function normally just updates the content grid but can be
	 * overridden by a subclass.
	 *
	 * @param origin              The origin for the content.
	 * @param size                The size of the content.
	 */
	virtual void place_content_grid(const tpoint& content_origin, const tpoint& content_size, const tpoint& desire_origin) = 0;

	/** Helper function which needs to be called after the scollbar moved. */
	void scrollbar_moved();

	/** Inherited from tcontrol. */
	const std::string& get_control_type() const;

	/***** ***** ***** signal handlers ***** ****** *****/

	void signal_handler_sdl_key_down(const event::tevent event
			, bool& handled
			, const SDLKey key
			, SDLMod modifier);

	void signal_handler_sdl_wheel_up(const event::tevent event
			, bool& handled);
	void signal_handler_sdl_wheel_down(const event::tevent event
			, bool& handled);
	void signal_handler_sdl_wheel_left(const event::tevent event
			, bool& handled);
	void signal_handler_sdl_wheel_right(const event::tevent event
			, bool& handled);
};

} // namespace gui2

#endif

