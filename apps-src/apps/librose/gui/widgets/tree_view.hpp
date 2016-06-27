/* $Id: tree_view.hpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
/*
   Copyright (C) 2010 - 2012 by Mark de Wever <koraq@xs4all.nl>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef GUI_WIDGETS_TREE_VIEW_HPP_INCLUDED
#define GUI_WIDGETS_TREE_VIEW_HPP_INCLUDED

#include "gui/widgets/scrollbar_container.hpp"
#include "gui/auxiliary/window_builder/tree_view.hpp"
#include "gui/widgets/tree_view_node.hpp"

namespace gui2 {

class ttree_view
		: public tscrollbar_container
{
	friend struct implementation::tbuilder_tree_view;
	friend class ttree_view_node;
public:

	typedef implementation::tbuilder_tree_view::tnode tnode_definition;

	/**
	 * Constructor.
	 *
	 * @param has_minimum         Does the listbox need to have one item
	 *                            selected.
	 * @param has_maximum         Can the listbox only have one item
	 *                            selected.
	 * @param placement           How are the items placed.
	 * @param select              Select an item when selected, if false it
	 *                            changes the visible state instead.
	 */
	explicit ttree_view(const std::vector<tnode_definition>& node_definitions);

	using tscrollbar_container::finalize_setup;

	ttree_view_node& get_root_node() { return *root_node_; }

	ttree_view_node& add_node(const std::string& id
			, const std::map<std::string /* widget id */, string_map>& data);

	void remove_node(ttree_view_node* tree_view_node);

	/** Inherited from tscrollbar_container. */
	void child_populate_dirty_list(twindow& caller,
			const std::vector<twidget*>& call_stack);
	void place_content_grid(const tpoint& content_origin, const tpoint& content_size, const tpoint& desire_origin);

	/** Inherited from tcontainer_. */
	void set_self_active(const bool /*active*/)  {}
//		{ state_ = active ? ENABLED : DISABLED; }

	bool empty() const;

	/***** ***** ***** setters / getters for members ***** ****** *****/

	void set_indention_step_size(const unsigned indention_step_size)
	{
		indention_step_size_ = indention_step_size;
	}

	ttree_view_node* selected_item() { return selected_item_; }

	const ttree_view_node* selected_item() const { return selected_item_; }

	void set_select_item(ttree_view_node* node);

	void set_selection_change_callback(boost::function<void()> callback)
	{
		selection_change_callback_ = callback;
	}

	void set_left_align() { left_align_ = true; } 

	void set_no_indentation(bool val) { no_indentation_ = val; }

	/** Inherited from tscrollbar_container. */
	// twidget* find_at(const tpoint& coordinate, const bool must_be_active);

	/** Inherited from tscrollbar_container. */
	tpoint adjust_content_size(const tpoint& size);
	void adjust_offset(int& x_offset, int& y_offset);
protected:

	/***** ***** ***** ***** keyboard functions ***** ***** ***** *****/
#if 0
	/** Inherited from tscrollbar_container. */
	void handle_key_up_arrow(SDLMod modifier, bool& handled);

	/** Inherited from tscrollbar_container. */
	void handle_key_down_arrow(SDLMod modifier, bool& handled);

	/** Inherited from tscrollbar_container. */
	void handle_key_left_arrow(SDLMod modifier, bool& handled);

	/** Inherited from tscrollbar_container. */
	void handle_key_right_arrow(SDLMod modifier, bool& handled);
#endif
private:

	/**
	 * @todo evaluate which way the dependancy should go.
	 *
	 * We no depend on the implementation, maybe the implementation should
	 * depend on us instead.
	 */
	const std::vector<tnode_definition> node_definitions_;

	unsigned indention_step_size_;

	ttree_view_node* root_node_;

	ttree_view_node* selected_item_;

	boost::function<void ()> selection_change_callback_;

	bool left_align_;
	bool no_indentation_;

	/** Inherited from tcontainer_. */
	virtual void finalize_setup();

	/** Inherited from tcontrol. */
	const std::string& get_control_type() const;

	/***** ***** ***** signal handlers ***** ****** *****/

	void signal_handler_left_button_down(const event::tevent event);
};

} // namespace gui2

#endif

