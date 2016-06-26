/* $Id: mkwin_display.cpp 47082 2010-10-18 00:44:43Z shadowmaster $ */
/*
   Copyright (C) 2008 - 2010 by Tomasz Sniatowski <kailoran@gmail.com>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#define GETTEXT_DOMAIN "studio-lib"

#include "unit.hpp"
#include "gettext.hpp"
#include "mkwin_display.hpp"
#include "mkwin_controller.hpp"
#include "gui/dialogs/mkwin_theme.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/report.hpp"
#include "gui/auxiliary/window_builder/helper.hpp"
#include "game_config.hpp"

#include <boost/bind.hpp>

extern std::string noise_config_key(const std::string& key);

const std::string unit::widget_prefix = "widget/";
const std::string unit::tpl_type = "tpl";
const std::string unit::tpl_widget_prefix = "tpl-";
const std::string unit::tpl_id_prefix = "_tpl_";

std::string unit::form_widget_png(const std::string& type, const std::string& definition)
{
	if (type == tpl_type) {
		return widget_prefix + definition + ".png";
	} else if (type != "grid") {
		return widget_prefix + type + ".png";
	} else {
		return "buttons/grid.png";
	}
}

std::string unit::form_widget_tpl(const std::string& id)
{
	std::stringstream ss;
	ss << tpl_widget_prefix << id;
	return ss.str();
}

std::string unit::form_tpl_widget_id(const std::string& tpl_id)
{
	std::stringstream ss;
	ss << tpl_id_prefix << tpl_id;
	return ss.str();
}

bool unit::is_widget_tpl(const std::string& tpl)
{
	return tpl.find(tpl_widget_prefix) == 0;
}

std::string unit::extract_from_widget_tpl(const std::string& tpl)
{
	return tpl.substr(tpl_widget_prefix.size());
}

bool unit::is_tpl_id(const std::string& id)
{
	return id.find(tpl_id_prefix) == 0;
}

std::string unit::extract_from_tpl_widget_id(const std::string& id)
{
	return id.substr(tpl_id_prefix.size());
}

const std::string tmode::def_id = "__def__";

int tmode::base_index() const
{
	if (res.width == 1024 && res.height == 768) {
		return 0;
	} else if (res.width == 640 && res.height == 480) {
		return 1;
	}

	VALIDATE(res.width == 480 && res.height == 320, "Invalid res!");
	return 2;
}

std::set<std::string> tadjust::change_fields;
std::set<std::string> tadjust::rect_fields;
std::set<std::string> tadjust::change2_fields;
const tadjust null_adjust(tadjust::NONE, tmode("", 1024, 768), config());

void tadjust::init_fields()
{
	if (!rect_fields.empty()) {
		return;
	}
	rect_fields.insert("rect");
	rect_fields.insert("ref");
	rect_fields.insert("xanchor");
	rect_fields.insert("yanchor");

	change2_fields.insert("definition");
	change2_fields.insert("unit_width");
	change2_fields.insert("unit_height");
	change2_fields.insert("gap");

	change_fields = rect_fields;
	for (std::set<std::string>::const_iterator it = change2_fields.begin(); it != change2_fields.end(); ++ it) {
		change_fields.insert(*it);
	}
}

void tadjust::full_change_cfg(config& cfg, bool rect)
{
	const std::set<std::string>& fields = rect? rect_fields: change2_fields;
	for (std::set<std::string>::const_iterator it = fields.begin(); it != fields.end(); ++ it) {
		const std::string& key = *it;
		if (!cfg.has_attribute(key)) {
			cfg[key] = null_str;
		}
	}
}

bool tadjust::cfg_is_rect_fields(const config& cfg)
{
	for (std::set<std::string>::const_iterator it = rect_fields.begin(); it != rect_fields.end(); ++ it) {
		const std::string& key = *it;
		if (!cfg.has_attribute(key)) {
			return false;
		}
	}
	BOOST_FOREACH (const config::attribute& attri, cfg.attribute_range()) {
		if (rect_fields.find(attri.first) == rect_fields.end()) {
			return false;
		}
	}
	return true;
}

bool tadjust::cfg_has_rect_fields(const config& cfg)
{
	return cfg.has_attribute("rect");
}

config tadjust::generate_empty_rect_cfg()
{
	config cfg;
	for (std::set<std::string>::const_iterator it = rect_fields.begin(); it != rect_fields.end(); ++ it) {
		cfg[*it] = "";
	}
	return cfg;
}

bool tadjust::different_change_cfg(const config& that, tristate rect) const
{
	// that' fields may more this's fields.
	const std::set<std::string>& fields = (rect == t_unset)? change_fields: ((rect == t_true)? rect_fields: change2_fields);
	for (std::set<std::string>::const_iterator it = fields.begin(); it != fields.end(); ++ it) {
		const std::string& k = *it;
		if (!cfg.has_attribute(k)) {
			continue;
		}
		if (cfg[k] != that[k]) {
			return true;
		}
	}
	return false;
}

bool tadjust::different_change_cfg2(const config& that, bool rect) const
{
	// that' fields may more this's fields.
	const std::set<std::string>& fields = rect? rect_fields: change2_fields;
	for (std::set<std::string>::const_iterator it = fields.begin(); it != fields.end(); ++ it) {
		const std::string& k = *it;
		if (!cfg.has_attribute(k)) {
			continue;
		}
		if (cfg[k] != that[k]) {
			return true;
		}
	}
	return false;
}

void tadjust::pure_change_fields(bool rect)
{
	const std::set<std::string>& require_remove = rect? change2_fields: rect_fields;
	for (std::set<std::string>::const_iterator it = require_remove.begin(); it != require_remove.end(); ++ it) {
		cfg.remove_attribute(*it);
	}

	const std::set<std::string>& require_keep = rect? rect_fields: change2_fields;
	for (std::set<std::string>::const_iterator it = require_keep.begin(); it != require_keep.end(); ++ it) {
		const std::string& key = *it;
		if (cfg.has_attribute(key) && cfg[key].str().empty()) {
			cfg.remove_attribute(*it);
		}
	}
}

void tadjust::pure_remove2_fields(const unit& u)
{
	VALIDATE(u.is_stacked_widget(), "Only stacked_widget's layer can remove!");

	bool remove;
	do {
		remove = false;
		BOOST_FOREACH (const config::attribute &i, cfg.attribute_range()) {
			if (!cfg[i.first].to_bool()) {
				cfg.remove_attribute(i.first);
				remove = true;
				break;
			}
		}
	} while (remove);
}

bool tadjust::is_empty_remove2_fields() const
{
	BOOST_FOREACH (const config::attribute &i, cfg.attribute_range()) {
		if (cfg[i.first].to_bool()) {
			return false;
		}
	}
	return true;
}

std::set<int> tadjust::newed_remove2_cfg(const config& that) const
{
	std::set<int> ret;
	BOOST_FOREACH (const config::attribute &i, cfg.attribute_range()) {
		if (!that.has_attribute(i.first)) {
			ret.insert(lexical_cast_default<int>(i.first));
		}
	}
	return ret;
}

unit::unit(mkwin_controller& controller, mkwin_display& disp, unit_map& units, const std::pair<std::string, gui2::tcontrol_definition_ptr>& widget, unit* parent, int number)
	: base_unit(units)
	, controller_(controller)
	, disp_(disp)
	, units_(units)
	, widget_(widget)
	, type_(WIDGET)
	, parent_(tparent(parent, number))
	, adjusts_()
{
}

unit::unit(mkwin_controller& controller, mkwin_display& disp, unit_map& units, int type, unit* parent, int number)
	: base_unit(units)
	, controller_(controller)
	, disp_(disp)
	, units_(units)
	, widget_()
	, type_(type)
	, parent_(tparent(parent, number))
	, adjusts_()
{
	if (type == WINDOW && !parent) {
		cell_.id = gui2::untitled;
	}
}

unit::unit(const unit& that)
	: base_unit(that)
	, controller_(that.controller_)
	, disp_(that.disp_)
	, units_(that.units_)
	, type_(that.type_)
	, parent_(that.parent_)
	, widget_(that.widget_)
	, cell_(that.cell_)
	, adjusts_(that.adjusts_)
{
	// caller require call set_parent to set self-parent.

	int number = 0;
	for (std::vector<tchild>::const_iterator it = that.children_.begin(); it != that.children_.end(); ++ it, number ++) {
		const tchild& child = *it;
		children_.push_back(tchild());
		tchild& child2 = children_.back();
		child2.window = new unit(*child.window);
		child2.window->parent_ = tparent(this, number);

		for (std::vector<unit*>::const_iterator it2 = child.rows.begin(); it2 != child.rows.end(); ++ it2) {
			const unit& that2 = **it2;
			unit* u = new unit(that2);
			u->parent_ = tparent(this, number);

			child2.rows.push_back(u);
		}
		for (std::vector<unit*>::const_iterator it2 = child.cols.begin(); it2 != child.cols.end(); ++ it2) {
			const unit& that2 = **it2;
			unit* u = new unit(that2);
			u->parent_ = tparent(this, number);
			child2.cols.push_back(u);
		}
		for (std::vector<unit*>::const_iterator it2 = child.units.begin(); it2 != child.units.end(); ++ it2) {
			const unit& that2 = **it2;
			unit* u = new unit(that2);
			u->parent_ = tparent(this, number);
			child2.units.push_back(u);
		}
	}
}

unit::~unit()
{
	for (std::vector<tchild>::iterator it = children_.begin(); it != children_.end(); ++ it) {
		it->clear(true);
	}
	if (this == controller_.copied_unit()) {
		controller_.set_copied_unit(NULL);
	}
}

bool unit::is_tpl() const
{
	return type_ == WIDGET && is_widget_tpl(widget_.first);
}

void unit::redraw_widget(int xsrc, int ysrc, int width, int height) const
{
	disp_.drawing_buffer_add(display::LAYER_UNIT_DEFAULT, 
		loc_, xsrc, ysrc, image::get_image(image(), image::SCALED_TO_ZOOM));

	if (widget_.second.get() && widget_.second->id == "default") {
		disp_.drawing_buffer_add(display::LAYER_UNIT_DEFAULT, 
			loc_, xsrc, ysrc, image::get_image(widget_prefix + "default.png", image::SCALED_TO_ZOOM));
	}

	if (!fix_rect()) {
		unsigned h_flag = cell_.widget.cell.flags_ & gui2::tgrid::HORIZONTAL_MASK;
		surface surf = image::get_image(gui2::horizontal_layout.find(h_flag)->second.icon);
		surf = scale_surface(surf, width / 4, height / 4);
		disp_.drawing_buffer_add(display::LAYER_UNIT_DEFAULT,
			loc_, xsrc + width / 2, ysrc, surf);

		unsigned v_flag = cell_.widget.cell.flags_ & gui2::tgrid::VERTICAL_MASK;
		surf = image::get_image(gui2::vertical_layout.find(v_flag)->second.icon);
		surf = scale_surface(surf, width / 4, height / 4);
		disp_.drawing_buffer_add(display::LAYER_UNIT_DEFAULT,
			loc_, xsrc + width * 3 / 4, ysrc, surf);
	} else {
		surface surf = image::get_image("misc/station.png");
		if (surf) {
			disp_.drawing_buffer_add(display::LAYER_UNIT_DEFAULT, loc_, xsrc + width - surf->w, ysrc, surf);
		}
	}

	surface text_surf;
	if (!cell_.id.empty()) {
		surface text_surf = font::get_rendered_text2(cell_.id, -1, 10 * gui2::twidget::hdpi_scale, font::BLACK_COLOR);
		if (text_surf->w > width) {
			SDL_Rect r = create_rect(0, 0, width, text_surf->h);
			text_surf = cut_surface(text_surf, r); 
		}
		disp_.drawing_buffer_add(display::LAYER_UNIT_DEFAULT,
			loc_, xsrc, ysrc + height / 2, text_surf);
	}
	if (!cell_.widget.label.empty()) {
		text_surf = font::get_rendered_text2(cell_.widget.label, -1, 10 * gui2::twidget::hdpi_scale, font::BLUE_COLOR);
		if (text_surf->w > width) {
			SDL_Rect r = create_rect(0, 0, width, text_surf->h);
			text_surf = cut_surface(text_surf, r); 
		}
		disp_.drawing_buffer_add(display::LAYER_UNIT_DEFAULT,
			loc_, xsrc, ysrc + height - text_surf->h, text_surf);
	}

	if (is_spacer() && (!cell_.widget.width.empty() || !cell_.widget.height.empty())) {
		if (!cell_.widget.width.empty()) {
			text_surf = font::get_rendered_text2(cell_.widget.width, -1, 10 * gui2::twidget::hdpi_scale, font::BAD_COLOR);
		} else {
			text_surf = font::get_rendered_text2("--", -1, 10 * gui2::twidget::hdpi_scale, font::BAD_COLOR);
		}
		disp_.drawing_buffer_add(display::LAYER_UNIT_DEFAULT,
			loc_, xsrc, ysrc + height - 2 * text_surf->h, text_surf);

		if (!cell_.widget.height.empty()) {
			text_surf = font::get_rendered_text2(cell_.widget.height, -1, 10 * gui2::twidget::hdpi_scale, font::BAD_COLOR);
		} else {
			text_surf = font::get_rendered_text2("--", -1, 10 * gui2::twidget::hdpi_scale, font::BAD_COLOR);
		}
		disp_.drawing_buffer_add(display::LAYER_UNIT_DEFAULT,
			loc_, xsrc, ysrc + height - text_surf->h, text_surf);
	}
}

void unit::redraw_unit()
{
	if (!loc_.valid()) {
		return;
	}

	if (refreshing_) {
		return;
	}
	trefreshing_lock lock(*this);
	redraw_counter_ ++;

	const int xsrc = disp_.get_location_x(loc_);
	const int ysrc = disp_.get_location_y(loc_);

	SDL_Rect dst_rect;
	int zoom = disp_.hex_width();
	surface surf = create_neutral_surface(zoom, zoom);

	const unit::tchild& child = parent_.u? parent_.u->child(parent_.number): controller_.top();
	Uint32 color = child.window->cell().window.color;

	if (this == controller_.copied_unit()) {
		SDL_Color col = font::BAD_COLOR;
		if (redraw_counter_ % 30 < 15) {
			col = font::GOOD_COLOR;
		}
		color = SDL_MapRGB(disp_.get_screen_surface()->format, col.r, col.g, col.b);
	}

	if (type_ == WIDGET) {
		{
			surface_lock locker(surf);
			draw_line(surf, color, 0, 0, zoom - 1, 0, false);
			draw_line(surf, color, 0, 0, 0, zoom - 1, false);
			if (!units_.find_unit(map_location(loc_.x + 1, loc_.y))) {
				draw_line(surf, color, zoom - 1, 0, zoom - 1, zoom - 1, false);
			}
			if (!units_.find_unit(map_location(loc_.x, loc_.y + 1))) {
				draw_line(surf, color, 0, zoom - 1, zoom - 1, zoom - 1, false);
			}
		}
		redraw_widget(xsrc, ysrc, zoom, zoom);

		if (has_child()) {
			blit_integer_surface(children_.size(), surf, 0, 0);
		}

	} else if (type_ == WINDOW) {
		{
			surface_lock locker(surf);
			draw_line(surf, color, 0, 0, zoom - 1, 0, false);
			draw_line(surf, color, 0, 0, 0, zoom - 1, false);
		}
		
		if (!parent_.u) {
			disp_.drawing_buffer_add(display::LAYER_UNIT_DEFAULT, 
				loc_, xsrc, ysrc, image::get_image(image(), image::SCALED_TO_ZOOM));
		} else {
			disp_.drawing_buffer_add(display::LAYER_UNIT_DEFAULT, 
				loc_, xsrc, ysrc, image::get_image(parent_.u->image(), image::SCALED_TO_ZOOM));
			if (parent_.u->is_stacked_widget()) {
				blit_integer_surface(parent_.number + 1, surf, 0, 0);
			} else if (parent_.u->is_listbox()) {
				surface text_surf = font::get_rendered_text2(parent_.number? "Body": "Header", -1, 12 * gui2::twidget::hdpi_scale, font::BAD_COLOR);
				dst_rect = create_rect(0, 0, 0, 0);
				sdl_blit(text_surf, NULL, surf, &dst_rect);
			} else if (parent_.u->is_panel()) {
				if (!parent_.u->cell().id.empty()) {
					surface text_surf = font::get_rendered_text2(parent_.u->cell().id, -1, 10 * gui2::twidget::hdpi_scale, font::BLACK_COLOR);
					dst_rect = create_rect(0, 0, 0, 0);
					sdl_blit(text_surf, NULL, surf, &dst_rect);
				}
			}
		}


		if (!cell_.id.empty()) {
			surface text_surf = font::get_rendered_text2(cell_.id, -1, 10 * gui2::twidget::hdpi_scale, font::BLACK_COLOR);
			disp_.drawing_buffer_add(display::LAYER_UNIT_DEFAULT,
				loc_, xsrc, ysrc + zoom / 2, text_surf);
		}

	} else if (type_ == COLUMN) {
		{
			surface_lock locker(surf);
			draw_line(surf, color, 0, 0, zoom - 1, 0, false);
			draw_line(surf, color, 0, 0, 0, zoom - 1, false);
			if (!units_.find_unit(map_location(loc_.x + 1, loc_.y))) {
				draw_line(surf, color, zoom - 1, 0, zoom - 1, zoom - 1, false);
			}
		}

		blit_integer_surface(loc_.x - child.window->get_location().x, surf, 0, 0);
		blit_integer_surface(cell_.column.grow_factor, surf, 0, 12);

	} else if (type_ == ROW) {
		{
			surface_lock locker(surf);
			draw_line(surf, color, 0, 0, zoom - 1, 0, false);
			draw_line(surf, color, 0, 0, 0, zoom - 1, false);
			if (!units_.find_unit(map_location(loc_.x, loc_.y + 1))) {
				draw_line(surf, color, 0, zoom - 1, zoom - 1, zoom - 1, false);
			}
		}

		blit_integer_surface(loc_.y - child.window->get_location().y, surf, 0, 0);
		blit_integer_surface(cell_.row.grow_factor, surf, 0, 12);

	}
	if (surf) {
		disp_.drawing_buffer_add(display::LAYER_UNIT_DEFAULT, loc_, xsrc, ysrc, surf);
	}

	int ellipse_floating = 0;
	if (loc_ == controller_.selected_hex()) {
		disp_.drawing_buffer_add(display::LAYER_UNIT_BG, loc_,
			xsrc, ysrc - ellipse_floating, image::get_image("misc/ellipse-top.png", image::SCALED_TO_ZOOM));
	
		disp_.drawing_buffer_add(display::LAYER_UNIT_FIRST, loc_,
			xsrc, ysrc - ellipse_floating, image::get_image("misc/ellipse-bottom.png", image::SCALED_TO_ZOOM));
	}
}

std::string unit::image() const
{
	if (type_ == COLUMN) {
		return widget_prefix + "column.png";

	} else if (type_ == ROW) {
		return widget_prefix + "row.png";

	} else if (type_ == WINDOW) {
		if (!controller_.current_unit()) {
			return widget_prefix + "window.png";
		} else {
			return widget_prefix + "grid.png";
		}

	} else if (is_widget_tpl(widget_.first)) {
		return form_widget_png(unit::tpl_type, extract_from_widget_tpl(widget_.first));

	} else if (widget_.first == "grid") {
		return widget_prefix + "grid.png";

	} else if (widget_.second.get()) {
		return form_widget_png(widget_.first, widget_.second->id);

	} else {
		return widget_prefix + "bg.png";
	}
}

void unit::set_cell(const gui2::tcell_setting& cell) 
{ 
	cell_ = cell;
	if (is_grid()) {
		children_[0].window->cell().id = cell_.id;
	}
}

void unit::set_child(int number, const tchild& child)
{
	// don't delete units that in children_[number].
	// unit's pointer is share new and old.
	children_[number] = child;
}

int unit::children_layers() const
{
	int max = 0;
	if (!children_.empty()) {
		const tchild& child = children_[0];
		int val;
		for (std::vector<unit*>::const_iterator it2 = child.units.begin(); it2 != child.units.end(); ++ it2) {
			const unit& u = **it2;
			val = u.children_layers();
			if (val > max) {
				max = val;
			}
		}
		return 1 + max;
	}
	return 0;
}

bool unit::is_main_map() const 
{ 
	return type_ == WIDGET && cell_.id == theme::id_main_map; 
}

const unit* unit::parent_at_top() const
{
	const unit* result = this;
	while (result->parent_.u) {
		result = result->parent_.u;
	}
	return result;
}

void unit::insert_child(int w, int h)
{
	unit_map::tconsistent_lock lock(units_);

	tchild child;

	child.window = new unit(controller_, disp_, units_, WINDOW, this, children_.size());
	// child.window->set_location(map_location(0, 0));
	for (int x = 1; x < w; x ++) {
		child.cols.push_back(new unit(controller_, disp_, units_, COLUMN, this, children_.size()));
		// child.cols.back()->set_location(map_location(x, 0));
	}
	for (int y = 1; y < h; y ++) {
		int pitch = y * w;
		for (int x = 0; x < w; x ++) {
			if (x) {
				child.units.push_back(new unit(controller_, disp_, units_, disp_.spacer, this, children_.size()));
				// child.cols.back()->set_location(map_location(x, y));
			} else {
				child.rows.push_back(new unit(controller_, disp_, units_, ROW, this, children_.size()));
				// child.rows.back()->set_location(map_location(0, y));
			}
		}
	}
	children_.push_back(child);
}

void unit::erase_child(int index)
{
	std::vector<tchild>::iterator it = children_.begin();
	std::advance(it, index);
	it->erase(units_);
	children_.erase(it);
	for (int n = index; n < (int)children_.size(); n ++) {
		const unit::tchild& child = children_[n];
		child.window->set_parent_number(n);
		for (std::vector<unit*>::const_iterator it = child.rows.begin(); it != child.rows.end(); ++ it) {
			unit* u = *it;
			u->set_parent_number(n);
		}
		for (std::vector<unit*>::const_iterator it = child.cols.begin(); it != child.cols.end(); ++ it) {
			unit* u = *it;
			u->set_parent_number(n);
		}
		for (std::vector<unit*>::const_iterator it = child.units.begin(); it != child.units.end(); ++ it) {
			unit* u = *it;
			u->set_parent_number(n);
		}
	}
}

void unit::insert_listbox_child(int w, int h)
{
	// header
	insert_child(w, h);

	// body must is toggle_panel
	unit_map::tconsistent_lock lock(units_);

	int body_w = 2;
	tchild child;
	child.window = new unit(controller_, disp_, units_, WINDOW, this, children_.size());
	for (int x = 1; x < body_w; x ++) {
		child.cols.push_back(new unit(controller_, disp_, units_, COLUMN, this, children_.size()));
	}
	for (int y = 1; y < body_w; y ++) {
		int pitch = y * body_w;
		for (int x = 0; x < body_w; x ++) {
			if (x) {
				child.units.push_back(new unit(controller_, disp_, units_, disp_.toggle_panel, this, children_.size()));
				child.units.back()->insert_child(mkwin_controller::default_child_width, mkwin_controller::default_child_height);
			} else {
				child.rows.push_back(new unit(controller_, disp_, units_, ROW, this, children_.size()));
			}
		}
	}
	VALIDATE(child.cols.size() * child.rows.size() == child.units.size(), "count of unit mistake!");
	children_.push_back(child);
}

void unit::insert_treeview_child()
{
	unit_map::tconsistent_lock lock(units_);

	int cols = 3, lines = 2;
	tchild child;
	child.window = new unit(controller_, disp_, units_, WINDOW, this, children_.size());
	for (int x = 1; x < cols; x ++) {
		child.cols.push_back(new unit(controller_, disp_, units_, COLUMN, this, children_.size()));
	}
	for (int y = 1; y < lines; y ++) {
		for (int x = 0; x < cols; x ++) {
			if (x == 1) {
				child.units.push_back(new unit(controller_, disp_, units_, disp_.toggle_button, this, children_.size()));
				child.units.back()->cell().id = "__icon";
			} else if (x) {
				child.units.push_back(new unit(controller_, disp_, units_, disp_.spacer, this, children_.size()));
			} else if (x == 0) {
				child.rows.push_back(new unit(controller_, disp_, units_, ROW, this, children_.size()));
			}
		}
	}

	VALIDATE(child.cols.size() * child.rows.size() == child.units.size(), "count of unit mistake!");
	children_.push_back(child);
}

std::string unit::child_tag(int index) const
{
	std::stringstream ss;
	if (is_grid()) {
		return "GRID";
	} else if (is_stacked_widget()) {
		ss << "S#" << index;
		return ss.str();
	} else if (is_listbox()) {
		if (!index) {
			return "Header";
		} else if (index == 1) {
			return "Body";
		} else if (index == 2) {
			return "Footer";
		}
	} else if (is_panel() || is_scrollbar_panel()) {
		return "Panel";
	} else if (is_tree_view()) {
		return "Node";
	}
	return null_str;
}

void unit::generate(config& cfg) const
{
	if (type_ == WIDGET) {
		generate_widget(cfg);
	} else if (type_ == WINDOW) {
		generate_window(cfg);
	} else if (type_ == ROW) {
		generate_row(cfg);
	} else if (type_ == COLUMN) {
		generate_column(cfg);
	}
}

std::string formual_fill_str(const std::string& core)
{
	return std::string("(") + core + ")";
}

std::string formual_extract_str(const std::string& str)
{
	size_t s = str.size();
	if (s < 2 || str.at(0) != '(' || str.at(s -1) != ')') {
		return str;
	}
	return str.substr(1, s - 2);
}

void unit::generate_window(config& cfg) const
{
	cfg["id"] = cell_.id;
	cfg["description"] = t_string(cell_.window.description, cell_.window.textdomain);

	config& res_cfg = cfg.add_child("resolution");

	res_cfg["definition"] = cell_.window.definition;
	if (cell_.window.click_dismiss) {
		res_cfg["click_dismiss"] = true;
	}
	if (cell_.window.orientation != gui2::twidget::auto_orientation) {
		res_cfg["orientation"] = gui2::orientations.find(cell_.window.orientation)->second.id;
	}

	if (cell_.window.automatic_placement) {
		if (cell_.window.horizontal_placement != gui2::tgrid::HORIZONTAL_ALIGN_CENTER) {
			res_cfg["horizontal_placement"] = gui2::horizontal_layout.find(cell_.window.horizontal_placement)->second.id;
		}
		if (cell_.window.vertical_placement != gui2::tgrid::VERTICAL_ALIGN_CENTER) {
			res_cfg["vertical_placement"] = gui2::vertical_layout.find(cell_.window.vertical_placement)->second.id;
		}
	} else {
		res_cfg["automatic_placement"] = false;
		if (!cell_.window.x.empty()) {
			res_cfg["x"] = formual_fill_str(cell_.window.x);
		}
		if (!cell_.window.y.empty()) {
			res_cfg["y"] = formual_fill_str(cell_.window.y);
		}
		if (!cell_.window.width.empty()) {
			res_cfg["width"] = formual_fill_str(cell_.window.width);
		}
		if (!cell_.window.height.empty()) {
			res_cfg["height"] = formual_fill_str(cell_.window.height);
		}
	}

	controller_.generate_linked_groups(res_cfg);
	controller_.generate_context_menus(res_cfg);

	config tmp;
	tmp["id"] = "tooltip_large";
	res_cfg.add_child("tooltip", tmp);

	tmp.clear();
	tmp["id"] = "helptip_large";
	res_cfg.add_child("helptip", tmp);

}

void unit::generate_row(config& cfg) const
{
	if (cell_.row.grow_factor) {
		cfg["grow_factor"] = cell_.row.grow_factor;
	}
}

void unit::generate_column(config& cfg) const
{
	if (cell_.column.grow_factor) {
		cfg["grow_factor"] = cell_.column.grow_factor;
	}
}

void unit::generate_widget(config& cfg) const
{
	std::stringstream ss;

	if (cell_.widget.cell.border_size_ && cell_.widget.cell.flags_ & gui2::tgrid::BORDER_ALL) {
		ss.str("");
		if ((cell_.widget.cell.flags_ & gui2::tgrid::BORDER_ALL) == gui2::tgrid::BORDER_ALL) {
			ss << "all";
		} else {
			bool first = true;
			if (cell_.widget.cell.flags_ & gui2::tgrid::BORDER_LEFT) {
				first = false;
				ss << "left";
			}
			if (cell_.widget.cell.flags_ & gui2::tgrid::BORDER_RIGHT) {
				if (!first) {
					ss << ", ";
				}
				first = false;
				ss << "right";
			}
			if (cell_.widget.cell.flags_ & gui2::tgrid::BORDER_TOP) {
				if (!first) {
					ss << ", ";
				}
				first = false;
				ss << "top";
			}
			if (cell_.widget.cell.flags_ & gui2::tgrid::BORDER_BOTTOM) {
				if (!first) {
					ss << ", ";
				}
				first = false;
				ss << "bottom";
			}
		}

		cfg["border"] = ss.str();
		cfg["border_size"] = (int)cell_.widget.cell.border_size_;
	}

	unsigned h_flag = cell_.widget.cell.flags_ & gui2::tgrid::HORIZONTAL_MASK;
	if (h_flag == gui2::tgrid::HORIZONTAL_GROW_SEND_TO_CLIENT) {
		cfg["horizontal_grow"] = true;
	} else if (h_flag != gui2::tgrid::HORIZONTAL_ALIGN_CENTER) {
		cfg["horizontal_alignment"] = gui2::horizontal_layout.find(h_flag)->second.id;
	}
	unsigned v_flag = cell_.widget.cell.flags_ & gui2::tgrid::VERTICAL_MASK;
	if (v_flag == gui2::tgrid::VERTICAL_GROW_SEND_TO_CLIENT) {
		cfg["vertical_grow"] = true;
	} else if (v_flag != gui2::tgrid::VERTICAL_ALIGN_CENTER) {
		cfg["vertical_alignment"] = gui2::vertical_layout.find(v_flag)->second.id;
	}

	if (parent_.u && !cell_.id.empty() && has_remove_adjust()) {
		// top unit hasn't [column].
		cfg["id"] = noise_config_key(cell_.id);
	}

	if (is_widget_tpl(widget_.first)) {
		generate_widget_tpl(cfg);
		return;
	}

	config& sub = cfg.add_child(widget_.first);
	if (!cell_.id.empty()) {
		sub["id"] = cell_.id;
	}
	if (!cell_.widget.linked_group.empty()) {
		sub["linked_group"] = cell_.widget.linked_group;
	}
	if (widget_.second.get()) {
		sub["definition"] = widget_.second->id;

		if (has_size()) {
			if (!cell_.widget.width.empty()) {
				sub["width"] = formual_fill_str(cell_.widget.width);
			}
			if (!cell_.widget.height.empty()) {
				sub["height"] = formual_fill_str(cell_.widget.height);
			}
		}
		if (has_drag()) {
			if (cell_.widget.drag) {
				sub["drag"] = gui2::implementation::form_drag_str(cell_.widget.drag);
			}
		}
	}

	if (fix_rect()) {
		// make sure other attribute not exist.
		BOOST_FOREACH (const config::attribute &i, cell_.rect_cfg.attribute_range()) {
			if (!i.second.empty()) {
				sub[i.first] = i.second;
			}
		}
	}

	if (is_scroll()) {
		if (is_report() && cell_.widget.vertical_mode != gui2::tscrollbar_container::auto_visible) {
			sub["horizontal_scrollbar_mode"] = gui2::horizontal_mode.find(gui2::tscrollbar_container::always_invisible)->second.id;

		} else if (cell_.widget.horizontal_mode != gui2::tscrollbar_container::auto_visible) {
			sub["horizontal_scrollbar_mode"] = gui2::horizontal_mode.find(cell_.widget.horizontal_mode)->second.id;
		}

		if (cell_.widget.vertical_mode != gui2::tscrollbar_container::auto_visible) {
			sub["vertical_scrollbar_mode"] = gui2::vertical_mode.find(cell_.widget.vertical_mode)->second.id;
		}
	}

	if (is_grid()) {
		generate_grid(sub);

	} else if (is_stacked_widget()) {
		generate_stacked_widget(sub);

	} else if (is_listbox()) {
		generate_listbox(sub);

	} else if (is_panel()) {
		generate_toggle_panel(sub);

	} else if (is_scrollbar_panel()) {
		generate_scrollbar_panel(sub);

	} else if (is_tree_view()) {
		generate_tree_view(sub);

	} else if (is_report()) {
		generate_report(sub);

	} else if (is_slider()) {
		generate_slider(sub);

	} else if (is_text_box2()) {
		generate_text_box2(sub);

	} else if (is_drawing()) {
		generate_drawing(sub);

	} else {
		if (!cell_.widget.label.empty()) {
			if (cell_.widget.label_textdomain.empty()) {
				sub["label"] = cell_.widget.label;
			} else {
				sub["label"] = t_string(cell_.widget.label, cell_.widget.label_textdomain);
			}
		}
		if (!cell_.widget.tooltip.empty()) {
			if (cell_.widget.tooltip_textdomain.empty()) {
				sub["tooltip"] = cell_.widget.tooltip;
			} else {
				sub["tooltip"] = t_string(cell_.widget.tooltip, cell_.widget.tooltip_textdomain);
			}
		}
	}
}

void unit::generate_widget_tpl(config& cfg) const
{
	// [column]
	//		{GUI__CHAT_WIDGET}
	// [/column]
	std::string id = unit::extract_from_widget_tpl(widget_.first);
	const config& tpl_cfg = controller_.core_config().find_child("tpl_widget", "id", id);
	cfg[noise_config_key("widget")] = tpl_cfg["widget"].str();

	controller_.insert_used_widget_tpl(tpl_cfg);
}

void unit::generate_grid(config& cfg) const
{
	// [grid]
	//		[..cfg..]
	// [/grid]
	const tchild& child = children_[0];
	child.generate(cfg);
}

void unit::generate_stacked_widget(config& cfg) const
{
	// [stacked_widget]
	//		[..cfg..]
	// [/stacked_widget]

	for (std::vector<tchild>::const_iterator it = children_.begin(); it != children_.end(); ++ it) {
		const tchild& child = *it;
		if (child.is_all_spacer()) {
			continue;
		}
		config& layer_cfg = cfg.add_child("layer");
		child.generate(layer_cfg);
	}
}

void unit::generate_listbox(config& cfg) const
{
	// [listbox]
	//		[..cfg..]
	// [/listbox]

	if (!children_[0].is_all_spacer()) {
		children_[0].generate(cfg.add_child("header"));
	}
	children_[1].generate(cfg.add_child("list_definition"));

	// now not support footer
/*
	if (!children_[2].is_all_spacer()) {
		children_[2].generate(cfg.add_child("footer"));
	}
*/
}

void unit::generate_toggle_panel(config& cfg) const
{
	// [toggle_panel]
	//		[..cfg..]
	// [/toggle_panel]

	config& grid_cfg = cfg.add_child("grid");
	const tchild& child = children_[0];
	child.generate(grid_cfg);
}

void unit::generate_scrollbar_panel(config& cfg) const
{
	// [scrollbar_panel]
	//		[..cfg..]
	// [/scrollbar_panel]

	config& grid_cfg = cfg.add_child("definition");
	const tchild& child = children_[0];
	child.generate(grid_cfg);
}

void unit::generate_tree_view(config& cfg) const
{
	// [tree_view]
	//		[..cfg..]
	// [/tree_view]
	cfg["indention_step_size"] = cell_.widget.tree_view.indention_step_size;
	config& node_cfg = cfg.add_child("node");
	node_cfg["id"] = cell_.widget.tree_view.node_id;

	config& node_definition_cfg = node_cfg.add_child("node_definition");
	node_definition_cfg["definition"] = cell_.widget.tree_view.node_definition;

	config& node_grid = node_definition_cfg.add_child("grid");
	const tchild& child = children_[0];
	child.generate(node_grid);
}

void unit::generate_report(config& cfg) const
{
	// [report]
	//		[..cfg..]
	// [/report]
	if (cell_.widget.report.multi_line) {
		cfg["multi_line"] = true;
	}
	if (cell_.widget.report.unit_width) {
		cfg["unit_width"] = cell_.widget.report.unit_width;
	}
	if (cell_.widget.report.unit_height) {
		cfg["unit_height"] = cell_.widget.report.unit_height;
	}
	if (cell_.widget.report.gap) {
		cfg["gap"] = cell_.widget.report.gap;
	}
}

void unit::generate_slider(config& cfg) const
{
	// [slider]
	//		[..cfg..]
	// [/slider]
	if (cell_.widget.slider.minimum_value) {
		cfg["minimum_value"] = cell_.widget.slider.minimum_value;
	}
	if (cell_.widget.slider.maximum_value) {
		cfg["maximum_value"] = cell_.widget.slider.maximum_value;
	}
	if (cell_.widget.slider.step_size) {
		cfg["step_size"] = cell_.widget.slider.step_size;
	}
}

void unit::generate_text_box2(config& cfg) const
{
	// [text_box2]
	//		[..cfg..]
	// [/text_box2]
	if (!cell_.widget.text_box2.text_box.empty() && cell_.widget.text_box2.text_box != "default") {
		cfg["text_box"] = cell_.widget.text_box2.text_box;
	}
}

void unit::generate_drawing(config& cfg) const
{
	// [drawing]
	//		[..cfg..]
	// [/drawing]

	config& draw_cfg = cfg.add_child("draw");
	config& image_cfg = draw_cfg.add_child("image");
	if (!cell_.widget.draw.x.empty()) {
		image_cfg["x"] = formual_fill_str(cell_.widget.draw.x);
	}
	if (!cell_.widget.draw.y.empty()) {
		image_cfg["y"] = formual_fill_str(cell_.widget.draw.y);
	}
	if (!cell_.widget.draw.w.empty()) {
		image_cfg["w"] = formual_fill_str(cell_.widget.draw.w);
	}
	if (!cell_.widget.draw.h.empty()) {
		image_cfg["h"] = formual_fill_str(cell_.widget.draw.h);
	}
	image_cfg["name"] = formual_fill_str(cell_.widget.draw.name);
}

void unit::tchild::generate(config& cfg) const
{
	config unit_cfg;
	config* row_cfg = NULL;
	int current_y = -1;
	if (!window->cell().id.empty() && window->cell().id != gui2::untitled) {
		cfg["id"] = window->cell().id;
	}
	for (int y = 0; y < (int)rows.size(); y ++) {
		int pitch = y * (int)cols.size();
		for (int x = 0; x < (int)cols.size(); x ++) {
			unit* u = units[pitch + x];
			if (u->get_location().y != current_y) {
				row_cfg = &cfg.add_child("row");
				current_y = u->get_location().y;

				unit* row = rows[y];
				row->generate(*row_cfg);
			}
			unit_cfg.clear();
			u->generate(unit_cfg);
			if (y == 0) {
				unit* column = cols[x];
				column->generate(unit_cfg);
			}
			row_cfg->add_child("column", unit_cfg);
		}
		// fixed. one grid must exist one [row].
		if (!cfg.child_count("row")) {
			cfg.add_child("row");
		}
	}
}

void unit::from(const config& cfg)
{
	if (type_ == WIDGET) {
		from_widget(cfg, false);
	} else if (type_ == WINDOW) {
		from_window(cfg);
	} else if (type_ == ROW) {
		from_row(cfg);
	} else if (type_ == COLUMN) {
		from_column(cfg);
	}
}

void unit::from_window(const config& cfg)
{
	// [window] <= cfg
	// [/window]

	set_location(map_location(0, 0));
	cell_.id = cfg["id"].str();
	t_string description = cfg["description"].t_str();
	split_t_string(description, cell_.window.textdomain, cell_.window.description);
	
	const config& res_cfg = cfg.child("resolution");

	cell_.window.definition = res_cfg["definition"].str();
	cell_.window.orientation = gui2::implementation::get_orientation(res_cfg["orientation"]);
	cell_.window.click_dismiss = res_cfg["click_dismiss"].to_bool();
	cell_.window.automatic_placement = res_cfg["automatic_placement"].to_bool(true);
	if (cell_.window.automatic_placement) {
		cell_.window.horizontal_placement = gui2::implementation::get_h_align(res_cfg["horizontal_placement"]);
		cell_.window.vertical_placement = gui2::implementation::get_v_align(res_cfg["vertical_placement"]);
	} else {
		cell_.window.x = formual_extract_str(res_cfg["x"].str());
		cell_.window.y = formual_extract_str(res_cfg["y"].str());
		cell_.window.width = formual_extract_str(res_cfg["width"].str());
		cell_.window.height = formual_extract_str(res_cfg["height"].str());
	}
}

void unit::from_row(const config& cfg)
{
	cell_.row.grow_factor = cfg["grow_factor"].to_int();
}

void unit::from_column(const config& cfg)
{
	cell_.column.grow_factor = cfg["grow_factor"].to_int();
}

void unit::from_widget(const config& cfg, bool unpack)
{
	if (units_.consistent()) {
		cell_.widget.cell.flags_ = gui2::implementation::read_flags(cfg);
		cell_.widget.cell.border_size_ = cfg["border_size"].to_int();
	}

	const config& sub_cfg = units_.consistent()? cfg.child(widget_.first): cfg;
	cell_.id = sub_cfg["id"].str();

	if (!unpack && is_tpl_id(cell_.id)) {
		if (from_widget_tpl(sub_cfg)) {
			return;
		}
	}

	cell_.widget.linked_group = sub_cfg["linked_group"].str();
	if (has_size()) {
		cell_.widget.width = formual_extract_str(sub_cfg["width"].str());
		cell_.widget.height = formual_extract_str(sub_cfg["height"].str());
	}
	if (has_drag()) {
		cell_.widget.drag = gui2::implementation::get_drag(sub_cfg["drag"].str());
	}
	split_t_string(sub_cfg["label"].t_str(), cell_.widget.label_textdomain, cell_.widget.label);
	split_t_string(sub_cfg["tooltip"].t_str(), cell_.widget.tooltip_textdomain, cell_.widget.tooltip);

	if (!sub_cfg["rect"].empty()) {
		const config& rect_cfg = controller_.find_rect_cfg(widget_.first, cell_.id);
		for (std::set<std::string>::const_iterator it = tadjust::rect_fields.begin(); it != tadjust::rect_fields.end(); ++ it) {
			const std::string& k = *it;
			// make sure cell_.rect_cfg include all rect field!
			cell_.rect_cfg[k] = rect_cfg[k];
		}
	}

	if (is_scroll()) {
		cell_.widget.vertical_mode = gui2::implementation::get_scrollbar_mode(sub_cfg["vertical_scrollbar_mode"]);
		cell_.widget.horizontal_mode = gui2::implementation::get_scrollbar_mode(sub_cfg["horizontal_scrollbar_mode"]);
	}
	if (is_grid()) {
		from_grid(sub_cfg);

	} else if (is_stacked_widget()) {
		from_stacked_widget(sub_cfg);

	} else if (is_listbox()) {
		from_listbox(sub_cfg);

	} else if (is_panel()) {
		from_toggle_panel(sub_cfg);

	} else if (is_scrollbar_panel()) {
		from_scrollbar_panel(sub_cfg);

	} else if (is_tree_view()) {
		from_tree_view(sub_cfg);

	} else if (is_report()) {
		from_report(sub_cfg);

	} else if (is_slider()) {
		from_slider(sub_cfg);

	} else if (is_text_box2()) {
		from_text_box2(sub_cfg);

	} else if (is_drawing()) {
		from_drawing(sub_cfg);
	}
}

void unit::tchild::from(mkwin_controller& controller, mkwin_display& disp, unit_map& units2, unit* parent, int number, const config& cfg)
{
	if (parent || !controller.theme()) {
		unit_map::tconsistent_lock lock(units2);

		window = new unit(controller, disp, units2, unit::WINDOW, parent, number);
		if (!cfg["id"].empty()) {
			window->cell().id = cfg["id"].str();
		}
		// generate require x, y. for example, according it to next line.
		window->set_location(map_location(0, 0));

		bool first_row = true;
		std::string type, definition;
		BOOST_FOREACH (const config& row, cfg.child_range("row")) {
			rows.push_back(new unit(controller, disp, units2, unit::ROW, parent, number));
			rows.back()->set_location(map_location(0, rows.size()));
			rows.back()->from(row);
			
			BOOST_FOREACH (const config& col, row.child_range("column")) {
				if (first_row) {
					cols.push_back(new unit(controller, disp, units2, unit::COLUMN, parent, number));
					cols.back()->set_location(map_location(cols.size(), 0));
					cols.back()->from(col);
				}
				int colno = 1;
				BOOST_FOREACH (const config::any_child& c, col.all_children_range()) {
					type = c.key;
					definition = c.cfg["definition"].str();
					gui2::tcontrol_definition_ptr ptr;
					if (type != "grid") {
						ptr = mkwin_display::find_widget(disp, type, definition, c.cfg["id"].str());
					}
					units.push_back(new unit(controller, disp, units2, std::make_pair(type, ptr), parent, number));
					units.back()->set_location(map_location(colno ++, rows.size()));
					units.back()->from(col);
				}
			}
			first_row = false;
		}
	} else {
		std::string type, definition;
		double factor = disp.get_zoom_factor();
		int zoom = disp.hex_size();
		BOOST_FOREACH (const config::any_child& v, cfg.all_children_range()) {
			if (controller.is_theme_reserved(v.key)) {
				continue;
			}
			
			type = v.key;
			definition = v.cfg["definition"].str();
			gui2::tcontrol_definition_ptr ptr;
			if (type != "grid" && type != "main_map") {
				ptr = mkwin_display::find_widget(disp, type, definition, v.cfg["definition"].str());
			}
			SDL_Rect rect = theme::calculate_relative_loc(v.cfg, theme::XDim, theme::YDim);
			rect = create_rect(rect.x * factor, rect.y * factor, rect.w * factor, rect.h * factor);
			rect.x += zoom;
			rect.y += zoom;
			units.push_back(new unit2(controller, disp, units2, std::make_pair(type, ptr), parent, rect));
			units.back()->from(v.cfg);
			units2.insert2(disp, units.back());
		}
	}
}

void unit::tchild::generate_adjust(const tmode& mode, config& cfg) const
{
	for (std::vector<unit*>::const_iterator it = units.begin(); it != units.end(); ++ it) {
		unit* u = *it;
		u->generate_adjust(mode, cfg);

		const std::vector<unit::tchild>& children = u->children();
		for (std::vector<unit::tchild>::const_iterator it2 = children.begin(); it2 != children.end(); ++ it2) {
			it2->generate_adjust(mode, cfg); 
		}
	}
}

void unit::tchild::draw_minimap_architecture(surface& screen, const SDL_Rect& minimap_location, const double xscaling, const double yscaling, int level) const
{
	static std::vector<SDL_Color> candidates;
	if (candidates.empty()) {
		candidates.push_back(font::TITLE_COLOR);
		candidates.push_back(font::BAD_COLOR);
		candidates.push_back(font::GRAY_COLOR);
		candidates.push_back(font::BLACK_COLOR);
		candidates.push_back(font::GOOD_COLOR);
	}
	SDL_Color col = candidates[level % candidates.size()];
	const Uint32 box_color = SDL_MapRGB(screen->format, col.r, col.g, col.b);

	double u_x = window->get_location().x * xscaling;
	double u_y = window->get_location().y * yscaling;
	double u_w = window->cell().window.cover_width * xscaling;
	double u_h = window->cell().window.cover_height * yscaling;

	window->cell().window.color = box_color;
	if (level) {
		draw_rectangle(minimap_location.x + round_double(u_x)
			, minimap_location.y + round_double(u_y)
			, round_double(u_w)
			, round_double(u_h)
			, box_color, screen);
	}

	for (std::vector<unit*>::const_iterator it = units.begin(); it != units.end(); ++ it) {
		unit* u = *it;
		const std::vector<unit::tchild>& children = u->children();
		for (std::vector<unit::tchild>::const_iterator it2 = children.begin(); it2 != children.end(); ++ it2) {
			const unit::tchild& sub = *it2;
			sub.draw_minimap_architecture(screen, minimap_location, xscaling, yscaling, level + 1);
		}
	}
}

void unit::tchild::erase(unit_map& units2)
{
	for (std::vector<unit*>::iterator it = units.begin(); it != units.end(); ) {
		unit* u = *it;
		units2.erase(u->get_location());
		it = units.erase(it);
	}
	for (std::vector<unit*>::iterator it = rows.begin(); it != rows.end(); ) {
		unit* u = *it;
		units2.erase(u->get_location());
		it = rows.erase(it);
	}
	for (std::vector<unit*>::iterator it = cols.begin(); it != cols.end(); ) {
		unit* u = *it;
		units2.erase(u->get_location());
		it = cols.erase(it);
	}
	units2.erase(window->get_location());
	window = NULL;
}

unit* unit::tchild::find_unit(const std::string& id) const
{
	unit* u = NULL;
	for (std::vector<unit*>::const_iterator it = units.begin(); it != units.end(); ++ it) {
		u = *it;
		if (u->cell().id == id) {
			return u;
		}
		const std::vector<tchild>& children = u->children();
		for (std::vector<tchild>::const_iterator it = children.begin(); it != children.end(); ++ it) {
			const tchild& child = *it;
			u = child.find_unit(id);
			if (u) {
				return u;
			}
		}
	}
	return NULL;
}

std::pair<unit*, int> unit::tchild::find_layer(const std::string& id) const
{
	for (std::vector<unit*>::const_iterator it = units.begin(); it != units.end(); ++ it) {
		unit* u = *it;
		
		const std::vector<tchild>& children = u->children();
		for (std::vector<tchild>::const_iterator it = children.begin(); it != children.end(); ++ it) {
			const tchild& child = *it;
			if (u->is_stacked_widget() && child.window->cell().id == id) {
				return std::make_pair(u, std::distance(children.begin(), it));
			}
			std::pair<unit*, int> ret = child.find_layer(id);
			if (ret.first) {
				return ret;
			}
		}
	}
	return std::make_pair((unit*)NULL, gui2::twidget::npos);
}

bool unit::from_widget_tpl(const config& cfg)
{
	std::string tpl_id = extract_from_tpl_widget_id(cell_.id);
	const config& tpl_cfg = controller_.core_config().find_child("tpl_widget", "id", tpl_id);
	if (!tpl_cfg) {
		return false;
	}

	widget_ = std::make_pair(form_widget_tpl(tpl_id), gui2::tcontrol_definition_ptr());
	controller_.insert_used_widget_tpl(tpl_cfg);
	return true;
}

void unit::from_grid(const config& cfg)
{
	children_.push_back(tchild());
	unit::tchild& child = children_.back();
	child.from(controller_, disp_, units_, this, children_.size() - 1, cfg);
}

void unit::from_stacked_widget(const config& cfg)
{
	const config& s = cfg.has_child("stack")? cfg.child("stack"): cfg;
	BOOST_FOREACH (const config& layer, s.child_range("layer")) {
		children_.push_back(tchild());
		children_.back().from(controller_, disp_, units_, this, children_.size() - 1, layer);
	}
}

void unit::from_listbox(const config& cfg)
{
	const config& header = cfg.child("header");
	if (header) {
		children_.push_back(tchild());
		children_.back().from(controller_, disp_, units_, this, children_.size() - 1, header);
	} else {
		insert_child(mkwin_controller::default_child_width, mkwin_controller::default_child_height);
	}

	children_.push_back(tchild());
	children_.back().from(controller_, disp_, units_, this, children_.size() - 1, cfg.child("list_definition"));

	// now not support footer
/*
	const config& footer = cfg.child("footer");
	if (footer) {
		children_.push_back(tchild());
		children_.back().from(controller_, disp_, units_, this, children_.size() - 1, footer);
	} else {
		insert_child(mkwin_controller::default_child_width, mkwin_controller::default_child_height);
	}
*/
}

void unit::from_toggle_panel(const config& cfg)
{
	children_.push_back(tchild());
	children_.back().from(controller_, disp_, units_, this, children_.size() - 1, cfg.child("grid"));
}

void unit::from_scrollbar_panel(const config& cfg)
{
	children_.push_back(tchild());
	children_.back().from(controller_, disp_, units_, this, children_.size() - 1, cfg.child("definition"));
}

void unit::from_tree_view(const config& cfg)
{
	cell_.widget.tree_view.indention_step_size = cfg["indention_step_size"].to_int();
	const config& node_cfg = cfg.child("node");
	if (!node_cfg) {
		return;
	}
	cell_.widget.tree_view.node_id = node_cfg["id"].str();
	const config& node_definition_cfg = node_cfg.child("node_definition");
	if (!node_definition_cfg) {
		return;
	}
	cell_.widget.tree_view.node_definition = node_definition_cfg["definition"].str();

	children_.push_back(tchild());
	children_.back().from(controller_, disp_, units_, this, children_.size() - 1, node_definition_cfg.child("grid"));
}

void unit::from_report(const config& cfg)
{
	cell_.widget.report.multi_line = cfg["multi_line"].to_bool();
	cell_.widget.report.unit_width = cfg["unit_width"].to_int();
	cell_.widget.report.unit_height = cfg["unit_height"].to_int();
	cell_.widget.report.gap = cfg["gap"].to_int();
}

void unit::from_slider(const config& cfg)
{
	cell_.widget.slider.minimum_value = cfg["minimum_value"].to_int();
	cell_.widget.slider.maximum_value = cfg["maximum_value"].to_int();
	cell_.widget.slider.step_size = cfg["step_size"].to_int();
}

void unit::from_text_box2(const config& cfg)
{
	cell_.widget.text_box2.text_box = cfg["text_box"].str();
}

void unit::from_drawing(const config& cfg)
{
	const config& draw_cfg = cfg.child("draw");
	if (!draw_cfg) {
		return;
	}
	const config& image_cfg = draw_cfg.child("image");
	if (!image_cfg) {
		return;
	}
	cell_.widget.draw.x = formual_extract_str(image_cfg["x"].str());
	cell_.widget.draw.y = formual_extract_str(image_cfg["y"].str());
	cell_.widget.draw.w = formual_extract_str(image_cfg["w"].str());
	cell_.widget.draw.h = formual_extract_str(image_cfg["h"].str());
	cell_.widget.draw.name = formual_extract_str(image_cfg["name"].str());
}

std::string unit::widget_tag() const
{
	std::stringstream ss;
	ss << "#" << get_map_index();

	return ss.str();
}

config unit::generate_change2_cfg() const
{
	if (!widget_.second.get()) {
		return null_cfg;
	}
	return generate2_change2_cfg(cell_);
}

config unit::generate2_change2_cfg(const gui2::tcell_setting& cell) const
{
	if (!widget_.second.get()) {
		return null_cfg;
	}

	config cfg;
	cfg["definition"] = widget_.second->id;
	if (is_report()) {
		cfg["multi_line"] = cell.widget.report.multi_line;
		cfg["unit_width"] = cell.widget.report.unit_width;
		cfg["unit_height"] = cell.widget.report.unit_height;
		cfg["gap"] = cell.widget.report.gap;
	}
	return cfg;
}

void calculate_base_modes(const tmode& mode, std::vector<tmode>& base_modes)
{
	VALIDATE(mode.id != tmode::def_id || mode.res.width != 1024, null_str);

	if (mode.id != tmode::def_id) {
		if (mode.res.width != 1024) {
			base_modes.push_back(tmode(mode.id, 1024, 768));
			base_modes.push_back(tmode(tmode::def_id, 640, 480));
			if (mode.res.width != 640) {
				base_modes.push_back(tmode(mode.id, 640, 480));
				base_modes.push_back(tmode(tmode::def_id, 480, 320));
			}
		} 
	} else {
		if (mode.res.width == 480 && mode.res.height == 320) {
			base_modes.push_back(tmode(mode.id, 640, 480));
		}
	}
}

config unit::get_base_change_cfg(const tmode& mode, bool rect, const config& main_dim_cfg) const
{
	config result = main_dim_cfg;
	tadjust::full_change_cfg(result, rect);

	std::vector<tmode> base_modes;
	calculate_base_modes(mode, base_modes);

	bool involve_main = true;
	for (std::vector<tmode>::const_iterator it = base_modes.begin(); it != base_modes.end(); ++ it) {
		const tmode& mode2 = *it;
		if (!involve_main && mode2.id == tmode::def_id) {
			continue;
		}
		tadjust adj = adjust(mode2, tadjust::CHANGE);
		if (adj.valid()) {
			adj.pure_change_fields(rect);
			if (!adj.cfg.empty() && adj.different_change_cfg2(result, rect)) {
				result.merge_attributes(adj.cfg);
				if (involve_main && mode2.id != tmode::def_id) {
					involve_main = false;
				}
			}
		}
	}
	return result;
}

bool unit::get_base_remove(const tmode& mode) const
{
	std::vector<tmode> base_modes;
	calculate_base_modes(mode, base_modes);

	for (std::vector<tmode>::const_iterator it = base_modes.begin(); it != base_modes.end(); ++ it) {
		tadjust adj = adjust(*it, tadjust::REMOVE);
		if (adj.valid()) {
			return true;
		}
	}
	return false;
}

bool unit::has_remove_adjust() const
{
	for (std::vector<tadjust>::const_iterator it = adjusts_.begin(); it != adjusts_.end(); ++ it) {
		if (it->type == tadjust::REMOVE) {
			return true;
		}
	}
	return false;
}

config unit::get_base_remove2_cfg(const tmode& mode) const
{
	config result;

	std::vector<tmode> base_modes;
	calculate_base_modes(mode, base_modes);

	for (std::vector<tmode>::const_iterator it = base_modes.begin(); it != base_modes.end(); ++ it) {
		const tmode& mode2 = *it;
		tadjust adj = adjust(mode2, tadjust::REMOVE2);
		if (adj.valid()) {
			result.merge_attributes(adj.cfg);
		}
	}
	return result;
}

tadjust unit::adjust(const tmode& mode, int type) const
{
	for (std::vector<tadjust>::const_iterator it = adjusts_.begin(); it != adjusts_.end(); ++ it) {
		if (it->type == type && it->mode == mode) {
			return *it;
		}
	}
	return null_adjust;
}

void unit::insert_adjust(const tadjust& adjust)
{
	for (std::vector<tadjust>::iterator it = adjusts_.begin(); it != adjusts_.end(); ++ it) {
		tadjust& adj = *it;
		if (adj.mode == adjust.mode && adj.type == adjust.type) {
			if (adjust.type == tadjust::CHANGE) {
				adj.cfg.merge_attributes(adjust.cfg);

			} else if (adjust.type == tadjust::REMOVE2) {
				adj.cfg.merge_attributes(adjust.cfg);
				adj.pure_remove2_fields(*this);

			} else {
				adj.cfg = adjust.cfg;
			}
			return;
		}
	}
	if (adjust.type == tadjust::REMOVE2) {
		if (adjust.is_empty_remove2_fields()) {
			return;
		}
	}
	adjusts_.push_back(adjust);
}

void unit::erase_adjust(const tmode& mode, int type)
{
	for (std::vector<tadjust>::iterator it = adjusts_.begin(); it != adjusts_.end(); ++ it) {
		if (it->mode == mode && it->type == type) {
			adjusts_.erase(it);
			return;
		}
	}
}

void unit::adjust_clear_rect_cfg(const tmode* mode)
{
	for (std::vector<tadjust>::iterator it = adjusts_.begin(); it != adjusts_.end(); ++ it) {
		tadjust& adjust = *it;
		if (adjust.type != tadjust::CHANGE) {
			continue;
		}
		if (mode && adjust.mode != *mode) {
			continue;
		}
		adjust.pure_change_fields(false);
	}
}

void unit::generate_adjust(const tmode& mode, config& cfg)
{
	for (std::vector<tadjust>::const_iterator it = adjusts_.begin(); it != adjusts_.end(); ++ it) {
		const tadjust& adjust = *it;
		if (adjust.mode != mode) {
			continue;
		}
		if (adjust.type == tadjust::CHANGE) {
			config cfg2 = get_base_change_cfg(mode, true, cell_.rect_cfg);

			cfg2.merge_attributes(get_base_change_cfg(mode, false, generate_change2_cfg()));
			if (adjust.different_change_cfg(cfg2, t_unset)) {
				config& change = cfg.add_child("change");
				change["id"] = cell_.id;
				for (std::set<std::string>::const_iterator it = tadjust::change_fields.begin(); it != tadjust::change_fields.end(); ++ it) {
					const std::string& k = *it;
					if (cfg2[k] != adjust.cfg[k]) {
						if (!adjust.cfg[k].empty()) {
							change[k] = adjust.cfg[k];
						}
					}
				}
			}
			
		} else if (adjust.type == tadjust::REMOVE) {
			config& remove = cfg.add_child("remove");
			remove["id"] = parent_.u? noise_config_key(cell_.id): cell_.id;

		} else if (adjust.type == tadjust::REMOVE2) {
			config base_remove2_cfg = get_base_remove2_cfg(mode);
			std::set<int> ret = adjust.newed_remove2_cfg(base_remove2_cfg);
			for (std::set<int>::const_iterator it2 = ret.begin(); it2 != ret.end(); ++ it2) {
				int number = *it2;
				if (number >= (int)children_.size()) {
					continue;
				}
				config& remove = cfg.add_child("remove");
				remove["id"] = children_[*it2].window->cell().id;
			}

		}
	}
}