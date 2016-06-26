/* $Id: tree_view_node.cpp 54604 2012-07-07 00:49:45Z loonycyborg $ */
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

#define GETTEXT_DOMAIN "rose-lib"

#include "gui/widgets/tree_view_node.hpp"

#include "gettext.hpp"
#include "gui/auxiliary/log.hpp"
#include "gui/widgets/toggle_button.hpp"
#include "gui/widgets/toggle_panel.hpp"
#include "gui/widgets/tree_view.hpp"

#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#define LOG_SCOPE_HEADER \
		get_control_type() + " [" + tree_view().id() + "] " + __func__
#define LOG_HEADER LOG_SCOPE_HEADER + ':'

namespace gui2 {

ttree_view_node& ttree_view_node::node_from_icon(twidget& icon)
{
	// #1, grid
	// #2, toggle_panel
	// #3, tree_view_node
	return *dynamic_cast<ttree_view_node*>(icon.parent()->parent()->parent());
}

const ttree_view_node& ttree_view_node::node_from_icon(const twidget& icon)
{
	// #1, grid
	// #2, toggle_panel
	// #3, tree_view_node
	return *dynamic_cast<const ttree_view_node*>(icon.parent()->parent()->parent());
}

ttree_view_node::ttree_view_node(const std::string& id
		, const std::vector<tnode_definition>& node_definitions
		, ttree_view_node* parent_node
		, ttree_view& parent_tree_view
		, const std::map<std::string /* widget id */, string_map>& data
		, bool branch)
	: twidget()
	, parent_node_(parent_node)
	, tree_view_(parent_tree_view)
	, panel_(NULL)
	, children_()
	, node_definitions_(node_definitions)
	, icon_(NULL)
	, branch_(branch)
{
	// grid_.set_parent(this);
	set_parent(&parent_tree_view);
	if (id != "root") {
		BOOST_FOREACH(const tnode_definition& node_definition, node_definitions_) {
			if (node_definition.id == id) {
				// node_definition.builder->build(&grid_);
				panel_ = dynamic_cast<ttoggle_panel*>(node_definition.builder->build());
				panel_->set_parent(this);
				tgrid& grid = panel_->grid();
				init_grid(&grid, data);

				icon_ = find_widget<ttoggle_button>(
						  &grid
						, "__icon"
						, false
						, false);

				if (icon_) {
					tvisible visible = twidget::HIDDEN;
					if (branch_) {
						visible = twidget::VISIBLE;
					} else if (parent_tree_view.no_indentation_ && get_indention_level() >= 2) {
						visible = twidget::INVISIBLE;
					}
					icon_->set_visible(visible);
					icon_->set_value(true);
					icon_->connect_signal<event::LEFT_BUTTON_CLICK>(
							boost::bind(&ttree_view_node::
								signal_handler_left_button_click
								, this, _2));
				}

				if (parent_node_ && parent_node_->icon_) {
					parent_node_->icon_->set_visible(twidget::VISIBLE);
				}

				panel_->connect_signal<event::LEFT_BUTTON_CLICK>(
							boost::bind(&ttree_view_node::
							signal_handler_label_left_button_click
							, this, _2, _3, _4)
						, event::tdispatcher::front_child);

				panel_->connect_signal<event::LEFT_BUTTON_CLICK>(
							boost::bind(&ttree_view_node::
							signal_handler_label_left_button_click
							, this, _2, _3, _4)
						, event::tdispatcher::front_post_child);

				panel_->connect_signal<event::LEFT_BUTTON_DOUBLE_CLICK>(boost::bind(
							&ttree_view_node::signal_handler_left_button_double_click
							, this, _2, _3, _4)
							, event::tdispatcher::front_child);
				panel_->connect_signal<event::LEFT_BUTTON_DOUBLE_CLICK>(boost::bind(
							&ttree_view_node::signal_handler_left_button_double_click
							, this, _2, _3, _4)
							, event::tdispatcher::front_post_child);

				if (!tree_view().selected_item_) {
					tree_view().selected_item_ = this;
					panel_->set_value(true);
				}

				return;
			}
		}

		VALIDATE(false, _("Unknown builder id for tree view node."));
	}
}

ttree_view_node::~ttree_view_node()
{
	for (std::vector<ttree_view_node*>::const_iterator it = children_.begin(); it != children_.end(); ++ it) {
		delete *it;
	}
	if (tree_view().selected_item_ == this) {
		tree_view().selected_item_ = NULL;
	}
	if (parent_node_) {
		delete panel_;
	}
}

ttree_view_node& ttree_view_node::add_child(
		  const std::string& id
		, const std::map<std::string /* widget id */, string_map>& data
		, const int index
		, const bool branch)
{

	std::vector<ttree_view_node*>::iterator itor = children_.end();

	if (static_cast<size_t>(index) < children_.size()) {
		itor = children_.begin() + index;
	}

	itor = children_.insert(itor, new ttree_view_node(
			  id
			, node_definitions_
			, this
			, tree_view()
			, data
			, branch));

	if (is_folded() || is_root_node()) {
		return **itor;
	}

	if (tree_view().get_size() == tpoint(0, 0)) {
		return **itor;
	}

	tree_view().invalidate_layout(false);

	return **itor;
}

unsigned ttree_view_node::get_indention_level() const
{
	unsigned level = 0;

	const ttree_view_node* node = this;
	while(!node->is_root_node()) {
		node = &node->parent_node();
		++level;
	}

	return level;
}

ttree_view_node& ttree_view_node::parent_node()
{
	assert(!is_root_node());
	return *parent_node_;
}

const ttree_view_node& ttree_view_node::parent_node() const
{
	assert(!is_root_node());
	return *parent_node_;
}

ttree_view& ttree_view_node::tree_view()
{
	return tree_view_;
}

const ttree_view& ttree_view_node::tree_view() const
{
	return tree_view_;
}

bool ttree_view_node::is_folded() const
{
	return icon_ && icon_->get_value();
}

void ttree_view_node::fold()
{
	if (!empty() && icon_ && !icon_->get_value()) {
		icon_->set_value(true);
		if (is_child2(*tree_view().selected_item_)) {
			tree_view().set_select_item(this);
		}
	}
}

void ttree_view_node::unfold()
{
	if (!empty() && icon_ && icon_->get_value()) {
		icon_->set_value(false);
	}
}

void ttree_view_node::fold_children()
{
	for (std::vector<ttree_view_node*>::iterator it = children_.begin(); it != children_.end(); ++ it) {
		ttree_view_node& node = **it;

		if (node.panel_->get_visible() == twidget::INVISIBLE) {
			continue;
		}

		node.fold();
	}
}

void ttree_view_node::unfold_children()
{
	for (std::vector<ttree_view_node*>::iterator it = children_.begin(); it != children_.end(); ++ it) {
		ttree_view_node& node = **it;

		if (node.panel_->get_visible() == twidget::INVISIBLE) {
			continue;
		}

		node.unfold();
	}
}

bool ttree_view_node::is_child2(const ttree_view_node& target) const
{
	const ttree_view_node* parent = &target;
	while (!parent->is_root_node()) {
		parent = &parent->parent_node();
		if (this == parent) {
			return true;
		}
	}
	return false;
}

void ttree_view_node::clear()
{
	/** @todo Also try to find the optimal width. */
	int height_reduction = 0;

	if (!is_folded()) {
		for (std::vector<ttree_view_node*>::const_iterator it = children_.begin(); it != children_.end(); ++ it) {
			ttree_view_node& node = **it;
			height_reduction += node.get_current_size().y;
		}
	}

	children_.clear();

	if (height_reduction == 0) {
		return;
	}

	tree_view().invalidate_layout(false);
}

twidget* ttree_view_node::find_at(const tpoint& coordinate, const bool must_be_active)
{
	twidget* result = parent_node_? panel_->find_at(coordinate, must_be_active): NULL;
	if (result) {
		return result;
	}
	if (is_folded()) {
		return NULL;
	}
	for (std::vector<ttree_view_node*>::const_iterator it = children_.begin(); it != children_.end(); ++ it) {
		ttree_view_node& node = **it;
		result = node.find_at(coordinate, must_be_active);
		if (result) {
			return result;
		}
	}
	return NULL;
}

const twidget* ttree_view_node::find_at(const tpoint& coordinate, const bool must_be_active) const
{
	const twidget* result = parent_node_? panel_->find_at(coordinate, must_be_active): NULL;
	if (result) {
		return result;
	}
	if (is_folded()) {
		return NULL;
	}
	for (std::vector<ttree_view_node*>::const_iterator it = children_.begin(); it != children_.end(); ++ it) {
		const ttree_view_node& node = **it;
		result = node.find_at(coordinate, must_be_active);
		if (result) {
			return result;
		}
	}
	return NULL;
}

twidget* ttree_view_node::find(const std::string& id, const bool must_be_active)
{
	twidget* result = parent_node_? panel_->find(id, must_be_active): NULL;
	if (result) {
		return result;
	}
	for (std::vector<ttree_view_node*>::iterator it = children_.begin(); it != children_.end(); ++ it) {
		ttree_view_node& node = **it;
		result = node.find(id, must_be_active);
		if (result) {
			return result;
		}
	}
	return NULL;
}

const twidget* ttree_view_node::find(const std::string& id, const bool must_be_active) const
{
	const twidget* result = parent_node_? panel_->find(id, must_be_active): NULL;
	if (result) {
		return result;
	}
	for (std::vector<ttree_view_node*>::const_iterator it = children_.begin(); it != children_.end(); ++ it) {
		const ttree_view_node& node = **it;
		result = node.find(id, must_be_active);
		if (result) {
			return result;
		}
	}
	return NULL;
}

void ttree_view_node::impl_populate_dirty_list(twindow& caller
		, const std::vector<twidget*>& call_stack)
{
	std::vector<twidget*> child_call_stack = call_stack;
	if (parent_node_) {
		panel_->populate_dirty_list(caller, child_call_stack);
	}

	if(is_folded()) {
		return;
	}

	for (std::vector<ttree_view_node*>::const_iterator it = children_.begin(); it != children_.end(); ++ it) {
		ttree_view_node& node = **it;
		std::vector<twidget*> child_call_stack = call_stack;
		node.impl_populate_dirty_list(caller, child_call_stack);
	}
}

tpoint ttree_view_node::calculate_best_size() const
{
	return calculate_best_size(-1, tree_view().indention_step_size_);
}

tpoint ttree_view_node::get_current_size() const
{
	if(parent_node_ && parent_node_->is_folded()) {
		return tpoint(0, 0);
	}

	tpoint size = get_folded_size();
	if (is_folded()) {
		return size;
	}

	for (std::vector<ttree_view_node*>::const_iterator it = children_.begin(); it != children_.end(); ++ it) {
		const ttree_view_node& node = **it;

		if(node.panel_->get_visible() == twidget::INVISIBLE) {
			continue;
		}

		tpoint node_size = node.get_current_size();

		size.y += node_size.y;
		size.x = std::max(size.x, node_size.x);
	}

	return size;
}

tpoint ttree_view_node::get_folded_size() const
{
	tpoint size = panel_->get_size();
	if(get_indention_level() > 1) {
		size.x += (get_indention_level() - 1) * tree_view().indention_step_size_;
	}
	return size;
}

tpoint ttree_view_node::get_unfolded_size() const
{
	tpoint size = panel_->get_best_size();
	if(get_indention_level() > 1) {
		size.x += (get_indention_level() - 1) * tree_view().indention_step_size_;
	}

	for (std::vector<ttree_view_node*>::const_iterator it = children_.begin(); it != children_.end(); ++ it) {
		const ttree_view_node& node = **it;

		if(node.panel_->get_visible() == twidget::INVISIBLE) {
			continue;
		}

		tpoint node_size = node.get_unfolded_size();

		size.y += node_size.y;
		size.x = std::max(size.x, node_size.x);
	}

	return size;
}

tpoint ttree_view_node::calculate_best_size(const int indention_level
		, const unsigned indention_step_size) const
{
	if (tree_view().left_align_) {
		return calculate_best_size_left_align(indention_level, indention_step_size);
	}

	tpoint best_size = parent_node_? panel_->get_best_size(): tpoint(0, 0);
	if(indention_level > 0) {
		best_size.x += indention_level * indention_step_size;
	}

	if(is_folded()) {

		DBG_GUI_L << LOG_HEADER
				<< " Folded grid return own best size " << best_size << ".\n";
		return best_size;
	}

	DBG_GUI_L << LOG_HEADER << " own grid best size " << best_size << ".\n";

	for (std::vector<ttree_view_node*>::const_iterator it = children_.begin(); it != children_.end(); ++ it) {
		const ttree_view_node& node = **it;

		if(node.panel_->get_visible() == twidget::INVISIBLE) {
			continue;
		}

		const tpoint node_size = node.calculate_best_size(indention_level + 1,
				indention_step_size);

		best_size.y += node_size.y;
		best_size.x = std::max(best_size.x, node_size.x);
	}

	DBG_GUI_L << LOG_HEADER << " result " << best_size << ".\n";
	return best_size;
}

tpoint ttree_view_node::calculate_best_size_left_align(const int indention_level
		, const unsigned indention_step_size) const
{
	tpoint best_size = panel_->get_best_size();
	if(indention_level > 0) {
		best_size.x += indention_step_size;
	}

	if(is_folded()) {

		DBG_GUI_L << LOG_HEADER
				<< " Folded grid return own best size " << best_size << ".\n";
		return best_size;
	}

	DBG_GUI_L << LOG_HEADER << " own grid best size " << best_size << ".\n";

	int max_node_width = 0;
	int node_height = 0;
	for (std::vector<ttree_view_node*>::const_iterator it = children_.begin(); it != children_.end(); ++ it) {
		const ttree_view_node& node = **it;

		if(node.panel_->get_visible() == twidget::INVISIBLE) {
			continue;
		}

		const tpoint node_size = node.calculate_best_size_left_align(indention_level + 1,
				indention_step_size);

		// if (is_root_node() || itor != children_.begin()) {
		//	best_size.y += node_size.y;
		// }
		max_node_width = std::max(max_node_width, node_size.x);
		node_height += node_size.y;
	}
	best_size.x += max_node_width;
	best_size.y = std::max(node_height, best_size.y);;
	
	DBG_GUI_L << LOG_HEADER << " result " << best_size << ".\n";
	return best_size;
}

void ttree_view_node::set_origin(const tpoint& origin)
{
	// Inherited.
	twidget::set_origin(origin);

	// Using layout_children seems to fail.
	place(tree_view().indention_step_size_, origin, get_size().x);
}

void ttree_view_node::place(const tpoint& origin, const tpoint& size)
{
	// Inherited.
	twidget::place(origin, size);

	VALIDATE(is_root_node(), "Only root node use normal place!");
	VALIDATE(origin == tree_view_.content_grid()->get_origin() && size == tree_view_.content_grid()->get_size(), null_str);

	place(tree_view_.indention_step_size_, origin, size.x);
	set_visible_area(tree_view_.content_visible_area());
}

unsigned ttree_view_node::place(const unsigned indention_step_size, tpoint origin, unsigned width)
{
	if (tree_view().left_align_) {
		return place_left_align(indention_step_size, origin, width);
	}

	const unsigned offset = origin.y;
	tpoint best_size(0, 0);
	if (parent_node_) {
		best_size = panel_->get_best_size();
	}
	best_size.x = width;
	if (panel_) {
		panel_->place(origin, best_size);
	}

	if (!is_root_node()) {
		origin.x += indention_step_size;
		width -= indention_step_size;
	}
	origin.y += best_size.y;

	if (is_folded()) {
		DBG_GUI_L << LOG_HEADER << " folded node done.\n";
		return origin.y - offset;
	}

	for (std::vector<ttree_view_node*>::iterator it = children_.begin(); it != children_.end(); ++ it) {
		ttree_view_node& node = **it;
		origin.y += node.place(indention_step_size, origin, width);
	}

	// Inherited.
	twidget::set_size(tpoint(width, origin.y - offset));

	DBG_GUI_L << LOG_HEADER << " result " << ( origin.y - offset) << ".\n";
	return origin.y - offset;
}

unsigned ttree_view_node::place_left_align(const unsigned indention_step_size, tpoint origin, unsigned width)
{
	const unsigned offset = origin.y;
	tpoint best_size = panel_->get_best_size();
	// grid_size_ = best_size;
	// best_size.x = width;
	panel_->place(origin, best_size);

	if(!is_root_node()) {
		origin.x += indention_step_size + panel_->get_width();
		width -= indention_step_size + panel_->get_width();
	}
	origin.y += best_size.y;

	if(is_folded()) {
		DBG_GUI_L << LOG_HEADER << " folded node done.\n";
		return origin.y - offset;
	}

	DBG_GUI_L << LOG_HEADER << " set children.\n";
	int index = 0;
	for (std::vector<ttree_view_node*>::iterator it = children_.begin(); it != children_.end(); ++ it) {
		ttree_view_node& node = **it;
		if (index == 0) {
			origin.y -= best_size.y;
		}
		index ++;
		origin.y += node.place_left_align(!node.empty()? indention_step_size: 0, origin, width);
	}

	// Inherited.
	twidget::set_size(tpoint(width, origin.y - offset));

	DBG_GUI_L << LOG_HEADER << " result " << ( origin.y - offset) << ".\n";
	return origin.y - offset;
}

void ttree_view_node::set_visible_area(const SDL_Rect& area)
{
	log_scope2(log_gui_layout, LOG_SCOPE_HEADER);
	DBG_GUI_L << LOG_HEADER << " area " << area << ".\n";
	if (parent_node_) {
		panel_->set_visible_area(area);
	}

	if (is_folded()) {
		DBG_GUI_L << LOG_HEADER << " folded node done.\n";
		return;
	}

	for (std::vector<ttree_view_node*>::iterator it = children_.begin(); it != children_.end(); ++ it) {
		ttree_view_node& node = **it;
		node.set_visible_area(area);
	}
}

void ttree_view_node::impl_draw_children(
		  surface& frame_buffer
		, int x_offset
		, int y_offset)
{
	if (parent_node_) {
		// if draw from windowself, it requre bouns entry to draw background of node, it is here.
		// if draw from terminal, draw of twindow can draw background of node, if it is terminal, also draw children,
		// attention: draw children isn't here, is ttoggle_panel::draw_children.
		panel_->draw_background(frame_buffer, x_offset, y_offset);
		panel_->draw_children(frame_buffer, x_offset, y_offset);
	}

	if (is_folded()) {
		return;
	}

	for (std::vector<ttree_view_node*>::iterator it = children_.begin(); it != children_.end(); ++ it) {
		ttree_view_node& node = **it;
		node.impl_draw_children(frame_buffer, x_offset, y_offset);
	}
}

void ttree_view_node::signal_handler_left_button_click(
		const event::tevent event)
{
	// is_folded() returns the new state.
	if (is_folded()) {
		// From unfolded to folded.
		if (is_child2(*tree_view().selected_item_)) {
			tree_view().set_select_item(this);
		}

	} else {
		// From folded to unfolded.
	}

	tree_view().invalidate_layout(false);
}

void ttree_view_node::signal_handler_label_left_button_click(
		  const event::tevent event
		, bool& handled
		, bool& halt)
{
	// We only snoop on the event so normally don't touch the handled, else if
	// we snoop in preexcept when halting.

	if (panel_->get_value()) {
		// Forbid deselecting
		halt = handled = true;
	} else {
		tree_view().set_select_item(this);
	}
}

void ttree_view_node::signal_handler_left_button_double_click(
		  const event::tevent event
		, bool& handled
		, bool& halt)
{
	// We only snoop on the event so normally don't touch the handled, else if
	// we snoop in preexcept when halting.

	if (is_folded()) {
		unfold();
	} else {
		fold();
	}

	if (panel_->get_value()) {
		// Forbid deselecting
		halt = handled = true;
	} else {
		tree_view().set_select_item(this);
	}

	tree_view().invalidate_layout(false);
}

void ttree_view_node::init_grid(tgrid* grid
		, const std::map<std::string /* widget id */, string_map>& data)
{
	assert(grid);

	for(unsigned row = 0; row < grid->get_rows(); ++row) {
		for(unsigned col = 0; col < grid->get_cols(); ++col) {
			twidget* widget = grid->widget(row, col);
			assert(widget);

			tgrid* child_grid = dynamic_cast<tgrid*>(widget);
//			ttoggle_button* btn = dynamic_cast<ttoggle_button*>(widget);
			ttoggle_panel* panel = dynamic_cast<ttoggle_panel*>(widget);
			tcontrol* ctrl = dynamic_cast<tcontrol*>(widget);

			if(panel) {
				panel->set_child_members(data);
			} else if(child_grid) {
				init_grid(child_grid, data);
			} else if(ctrl) {
				std::map<std::string, string_map>::const_iterator itor =
						data.find(ctrl->id());

				if(itor == data.end()) {
					itor = data.find("");
				}
				if(itor != data.end()) {
					ctrl->set_members(itor->second);
				}
//				ctrl->set_members(data);
			} else {

//				ERROR_LOG("Widget type '"
//						<< typeid(*widget).name() << "'.");
			}
		}
	}

}

const std::string& ttree_view_node::get_control_type() const
{
	static const std::string type = "tree_view_node";
	return type;
}

} // namespace gui2

