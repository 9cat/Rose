/* $Id: control.hpp 54584 2012-07-05 18:33:49Z mordante $ */
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

#ifndef GUI_WIDGETS_CONTROL_HPP_INCLUDED
#define GUI_WIDGETS_CONTROL_HPP_INCLUDED

#include "gui/auxiliary/widget_definition.hpp"
#include "gui/auxiliary/formula.hpp"
#include "gui/widgets/widget.hpp"
#include "integrate.hpp"

namespace gui2 {

namespace implementation {
	struct tbuilder_control;
} // namespace implementation

/** Base class for all visible items. */
class tcontrol : public virtual twidget
{
public:
	static bool force_add_to_dirty_list;

	class ttext_maximum_width_lock
	{
	public:
		ttext_maximum_width_lock(tcontrol& widget, int text_maximum_width2)
			: widget_(widget)
			, original_(widget.text_maximum_width_)
		{
			widget_.text_maximum_width_ = text_maximum_width2 - widget.config_->text_extra_width;
		}
		~ttext_maximum_width_lock()
		{
			widget_.text_maximum_width_ = original_;
		}

	private:
		tcontrol& widget_;
		int original_;
	};

	class tadd_to_dirty_list_lock
	{
	public:
		tadd_to_dirty_list_lock()
			: force_add_to_dirty_list_(force_add_to_dirty_list)
		{
			force_add_to_dirty_list = true;
		}
		~tadd_to_dirty_list_lock()
		{
			force_add_to_dirty_list = force_add_to_dirty_list_;
		}

	private:
		bool force_add_to_dirty_list_;
	};

	/** @deprecated Used the second overload. */
	explicit tcontrol(const unsigned canvas_count);

	/**
	 * Constructor.
	 *
	 * @param builder             The builder object with the settings for the
	 *                            object.
	 *
	 * @param canvas_count        The number of canvasses in the control.
	 */
	tcontrol(
			  const implementation::tbuilder_control& builder
			, const unsigned canvas_count
			, const std::string& control_type);
	virtual ~tcontrol();

	/**
	 * Sets the members of the control.
	 *
	 * The map contains named members it can set, controls inheriting from us
	 * can add additional members to set by this function. The following
	 * members can by the following key:
	 *  * label_                  label
	 *  * tooltip_                tooltip
	 *
	 *
	 * @param data                Map with the key value pairs to set the members.
	 */
	virtual void set_members(const string_map& data);

	/***** ***** ***** ***** State handling ***** ***** ***** *****/

	/**
	 * Sets the control's state.
	 *
	 *  Sets the control in the active state, when inactive a control can't be
	 *  used and doesn't react to events. (Note read-only for a ttext_ is a
	 *  different state.)
	 */
	virtual void set_active(const bool active) = 0;

	/** Gets the active state of the control. */
	virtual bool get_active() const = 0;

	int insert_animation(const ::config& cfg, bool pre);
	void erase_animation(int id);

	void set_canvas_variable(const std::string& name, const variant& value);

protected:
	/** Returns the id of the state.
	 *
	 * The current state is also the index canvas_.
	 */
	virtual unsigned get_state() const = 0;

	tpoint best_size_from_builder() const;

public:

	/***** ***** ***** ***** Easy close handling ***** ***** ***** *****/

	/**
	 * Inherited from twidget.
	 *
	 * The default behavious is that a widget blocks easy close, if not it
	 * hould override this function.
	 */
	bool disable_click_dismiss() const;

	/** Inherited from twidget. */
	virtual iterator::twalker_* create_walker();

	/***** ***** ***** ***** layout functions ***** ***** ***** *****/

	/**
	 * Gets the default size as defined in the config.
	 *
	 * @pre                       config_ !=  NULL
	 *
	 * @returns                   The size.
	 */
	tpoint get_config_default_size() const;

	/**
	 * Gets the best size as defined in the config.
	 *
	 * @pre                       config_ !=  NULL
	 *
	 * @returns                   The size.
	 */
	tpoint get_config_maximum_size() const;

	/**
	 * Returns the number of characters per line.
	 *
	 * This value is used to call @ref ttext::set_characters_per_line
	 * (indirectly).
	 *
	 * @returns                   The characters per line. This implementation
	 *                            always returns 0.
	 */
	virtual unsigned get_characters_per_line() const;

	/** Inherited from twidget. */
	/** @todo Also handle the tooltip state if shrunken_ &&
	 * use_tooltip_on_label_overflow_. */
	void layout_init(const bool full_initialization);

	void refresh_locator_anim(std::vector<tintegrate::tlocator>& locator);
	void set_integrate_default_color(const SDL_Color& color);

	virtual void set_surface(const surface& surf, int w, int h);

	/** Inherited from twidget. */
	tpoint calculate_best_size() const;

	tpoint request_reduce_width(const unsigned maximum_width);

	bool drag_detect_started() const { return drag_detect_started_; }
	const tpoint& first_drag_coordinate() const { return first_drag_coordinate_; }
	const tpoint& last_drag_coordinate() const { return last_drag_coordinate_; }
	void control_drag_detect(bool start, int x = npos, int y = npos, const tdrag_direction type = drag_none);
	void set_drag_coordinate(int x, int y);
	int drag_satisfied();
	void set_enable_drag_draw_coordinate(bool enable) { enable_drag_draw_coordinate_ = enable; }

	void set_draw_offset(int x, int y);
	const tpoint& draw_offset() const { return draw_offset_; }

protected:
	virtual bool exist_anim();
	virtual void calculate_integrate();

	tintegrate* integrate_;
	SDL_Color integrate_default_color_;
	std::vector<tintegrate::tlocator> locator_;

	/**
	 * Surface of all in state.
	 *
	 * If it is surface style button, surfs_ will not empty.
	 */
	std::vector<surface> surfs_;

public:

	/** Inherited from twidget. */
	void place(const tpoint& origin, const tpoint& size);

	/***** ***** ***** ***** Inherited ***** ***** ***** *****/

private:

	/**
	 * Loads the configuration of the widget.
	 *
	 * Controls have their definition stored in a definition object. In order to
	 * determine sizes and drawing the widget this definition needs to be
	 * loaded. The member definition_ contains the name of the definition and
	 * function load the proper configuration.
	 *
	 * @depreciated @ref definition_load_configuration() is the replacement.
	 */
	void load_config();

	/**
	 * Uses the load function.
	 *
	 * @note This doesn't look really clean, but the final goal is refactor
	 * more code and call load_config in the ctor, removing the use case for
	 * the window. That however is a longer termine refactoring.
	 */
	friend class twindow;

public:
	/** Inherited from twidget. */
	twidget* find_at(const tpoint& coordinate, const bool must_be_active)
	{
		return (twidget::find_at(coordinate, must_be_active)
			&& (!must_be_active || get_active())) ? this : NULL;
	}

	/** Inherited from twidget. */
	const twidget* find_at(const tpoint& coordinate,
			const bool must_be_active) const
	{
		return (twidget::find_at(coordinate, must_be_active)
			&& (!must_be_active || get_active())) ? this : NULL;
	}

	/** Inherited from twidget.*/
	twidget* find(const std::string& id, const bool must_be_active)
	{
		return (twidget::find(id, must_be_active)
			&& (!must_be_active || get_active())) ? this : NULL;
	}

	/** Inherited from twidget.*/
	const twidget* find(const std::string& id,
			const bool must_be_active) const
	{
		return (twidget::find(id, must_be_active)
			&& (!must_be_active || get_active())) ? this : NULL;
	}

	/**
	 * Sets the definition.
	 *
	 * This function sets the definition of a control and should be called soon
	 * after creating the object since a lot of internal functions depend on the
	 * definition.
	 *
	 * This function should be called one time only!!!
	 */
	void set_definition(const std::string& definition);

	/***** ***** ***** setters / getters for members ***** ****** *****/
	bool get_use_tooltip_on_label_overflow() const { return use_tooltip_on_label_overflow_; }
	void set_use_tooltip_on_label_overflow(const bool use_tooltip = true)
		{ use_tooltip_on_label_overflow_ = use_tooltip; }

	const std::string& label() const { return label_; }
	virtual void set_label(const std::string& label);

	virtual void set_text_editable(bool editable);
	bool get_text_editable() const { return text_editable_; }

	const t_string& tooltip() const { return tooltip_; }
	// Note setting the tooltip_ doesn't dirty an object.
	void set_tooltip(const t_string& tooltip)
		{ tooltip_ = tooltip; set_wants_mouse_hover(!tooltip_.empty()); }

	// const versions will be added when needed
	std::vector<tcanvas>& canvas() { return canvas_; }
	tcanvas& canvas(const unsigned index)
		{ assert(index < canvas_.size()); return canvas_[index]; }
	const tcanvas& canvas(const unsigned index) const
		{ assert(index < canvas_.size()); return canvas_[index]; }

	void set_text_alignment(const PangoAlignment text_alignment);
	PangoAlignment get_text_alignment() const
	{
		return text_alignment_;
	}

	tresolution_definition_ptr config() { return config_; }
	tresolution_definition_const_ptr config() const { return config_; }

	// limit width of text when calculate_best_size.
	void set_text_maximum_width(int maximum);
	void clear_label_size_cache();

	void set_callback_place(boost::function<void (tcontrol*, const SDL_Rect&)> callback)
		{ callback_place_ = callback; }

	void set_callback_control_drag_detect(boost::function<bool (tcontrol*, bool start, const tdrag_direction)> callback)
		{ callback_control_drag_detect_ = callback; }

	void set_callback_set_drag_coordinate(boost::function<bool (tcontrol*, const tpoint& first, const tpoint& last)> callback)
		{ callback_set_drag_coordinate_ = callback; }

	void set_callback_pre_impl_draw_children(boost::function<void (tcontrol*, surface&, int, int)> callback)
		{ callback_pre_impl_draw_children_ = callback; }

	void set_best_size(const std::string& width, const std::string& height, const std::string& hdpi_off);

protected:
	void set_config(tresolution_definition_ptr config) { config_ = config; }

	/***** ***** ***** ***** miscellaneous ***** ***** ***** *****/

	/**
	 * Updates the canvas(ses).
	 *
	 * This function should be called if either the size of the widget changes
	 * or the text on the widget changes.
	 */
	virtual void update_canvas();

	/**
	 * Returns the maximum width available for the text.
	 *
	 * This value makes sense after the widget has been given a size, since the
	 * maximum width is based on the width of the widget.
	 */
	int get_text_maximum_width() const;

	/**
	 * Returns the maximum height available for the text.
	 *
	 * This value makes sense after the widget has been given a size, since the
	 * maximum height is based on the height of the widget.
	 */
	int get_text_maximum_height() const;

	/** Contain the non-editable text associated with control. */
	std::string label_;

	/** Read only for the label? */
	bool text_editable_;

	bool drag_detect_started_;
	tpoint first_drag_coordinate_;
	tpoint last_drag_coordinate_;
	bool enable_drag_draw_coordinate_;

	tpoint draw_offset_;

	boost::function<void (tcontrol*, surface& frame_buffer, int x_offset, int y_offset)> callback_pre_impl_draw_children_;

	tformula<unsigned> best_width_;
	tformula<unsigned> best_height_;
	bool hdpi_off_width_;
	bool hdpi_off_height_;

private:
	/**
	 * The definition is the id of that widget class.
	 *
	 * Eg for a button it [button_definition]id. A button can have multiple
	 * definitions which all look different but for the engine still is a
	 * button.
	 */
	std::string definition_;

	mutable std::pair<int, tpoint> label_size_;

	std::vector<int> pre_anims_;
	std::vector<int> post_anims_;

	// call when after place.
	boost::function<void (tcontrol*, const SDL_Rect&)> callback_place_;

	boost::function<bool (tcontrol*, bool start, const tdrag_direction type)> callback_control_drag_detect_;

	boost::function<bool (tcontrol*, const tpoint& first, const tpoint& last)> callback_set_drag_coordinate_;

	/**
	 * If the text doesn't fit on the label should the text be used as tooltip?
	 *
	 * This only happens if the tooltip is empty.
	 */
	bool use_tooltip_on_label_overflow_;

	/**
	 * Tooltip text.
	 *
	 * The hovering event can cause a small tooltip to be shown, this is the
	 * text to be shown. At the moment the tooltip is a single line of text.
	 */
	t_string tooltip_;

	/**
	 * Holds all canvas objects for a control.
	 *
	 * A control can have multiple states, which are defined in the classes
	 * inheriting from us. For every state there is a separate canvas, which is
	 * stored here. When drawing the state is determined and that canvas is
	 * drawn.
	 */
	std::vector<tcanvas> canvas_;

	/**
	 * Contains the pointer to the configuration.
	 *
	 * Every control has a definition of how it should look, this contains a
	 * pointer to the definition. The definition is resolution dependant, where
	 * the resolution is the size of the Wesnoth application window. Depending
	 * on the resolution widgets can look different, use different fonts.
	 * Windows can use extra scrollbars use abbreviations as text etc.
	 */
	tresolution_definition_ptr config_;

	/**
	 * Load class dependant config settings.
	 *
	 * load_config will call this method after loading the config, by default it
	 * does nothing but classes can override it to implement custom behaviour.
	 */
	virtual void load_config_extra() {}

	/**
	 * Loads the configuration of the widget.
	 *
	 * Controls have their definition stored in a definition object. In order to
	 * determine sizes and drawing the widget this definition needs to be
	 * loaded. The member definition_ contains the name of the definition and
	 * function load the proper configuration.
	 */
	void definition_load_configuration(const std::string& control_type);

protected:
	/** Inherited from twidget. */
	void impl_draw_background(
			  surface& frame_buffer
			, int x_offset
			, int y_offset);

	/** Inherited from twidget. */
	void impl_draw_foreground(
			  surface& /*frame_buffer*/
			, int /*x_offset*/
			, int /*y_offset*/)
	{ /* do nothing */ }

	void child_populate_dirty_list(twindow& caller, const std::vector<twidget*>& call_stack);

private:

	/**
	 * Gets the best size for a text.
	 *
	 * @param minimum_size        The minimum size of the text.
	 * @param maximum_size        The wanted maximum size of the text, if not
	 *                            possible it's ignored. A value of 0 means
	 *                            that it's ignored as well.
	 *
	 * @returns                   The best size.
	 */
	tpoint get_best_text_size(const tpoint& minimum_size,
		const tpoint& maximum_size = tpoint(0,0)) const;

	/**
	 * Contains a helper cache for the rendering.
	 *
	 * Creating a ttext object is quite expensive and is done on various
	 * occasions so it's cached here.
	 *
	 * @todo Maybe if still too slow we might also copy this cache to the
	 * canvas so it can reuse our results, but for now it seems fast enough.
	 * Unfortunately that would make the dependency between the classes bigger
	 * as wanted.
	 */
	// mutable font::ttext renderer_;

	/** The maximum width for the text in a control. */
	int text_maximum_width_;

	/** The alignment of the text in a control. */
	PangoAlignment text_alignment_;

	/** Is the widget smaller as it's best size? */
	bool shrunken_;

	/***** ***** ***** signal handlers ***** ****** *****/

	void signal_handler_show_tooltip(
			  const event::tevent event
			, bool& handled
			, const tpoint& location);

	void signal_handler_notify_remove_tooltip(
			  const event::tevent event
			, bool& handled);
};

} // namespace gui2

#endif

