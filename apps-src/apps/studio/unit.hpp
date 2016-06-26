/* $Id: editor_display.hpp 47608 2010-11-21 01:56:29Z shadowmaster $ */
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

#ifndef STUDIO_UNIT_HPP_INCLUDED
#define STUDIO_UNIT_HPP_INCLUDED

#include "base_unit.hpp"
#include "gui/auxiliary/widget_definition.hpp"
#include "gui/dialogs/cell_setting.hpp"

class mkwin_controller;
class mkwin_display;
class unit_map;
class unit;

struct tres {
	tres(int width, int height)
		: width(width)
		, height(height)
	{}

	bool operator<(const tres& that) const 
	{
		return height > that.height || (height == that.height && width > that.width);
	}
	bool operator==(const tres& that) const 
	{
		return height == that.height && width == that.width;
	}
	bool operator!=(const tres& that) const 
	{
		return height != that.height || width != that.width;
	}

	int width;
	int height;
};

struct tmode {
	tmode(const std::string& id, int width, int height)
		: id(id)
		, res(tres(width, height))
	{}

	bool operator==(const tmode& that) const
	{
		return id == that.id && res.height == that.res.height && res.width == that.res.width;
	}

	bool operator!=(const tmode& that) const
	{
		return id != that.id || res.height != that.res.height || res.width != that.res.width;
	}

	int base_index() const;

	static const int res_count = 3;
	static const std::string def_id;

	std::string id;
	tres res;
};

struct tadjust {
	enum {NONE, REMOVE, REMOVE2, CHANGE};

	tadjust(int type, const tmode& mode, const config& cfg) 
		: type(type)
		, mode(mode)
		, cfg(cfg)
	{}
	bool valid() const { return type != NONE; }

	static std::set<std::string> rect_fields;
	static std::set<std::string> change2_fields;
	static std::set<std::string> change_fields;
	static void init_fields();
	static config generate_empty_rect_cfg();
	static void full_change_cfg(config& cfg, bool rect);
	static bool cfg_is_rect_fields(const config& cfg);
	static bool cfg_has_rect_fields(const config& cfg);

	void pure_change_fields(bool rect);
	bool different_change_cfg(const config& that, tristate rect) const;
	bool different_change_cfg2(const config& that, bool rect) const;

	void pure_remove2_fields(const unit& u);
	bool is_empty_remove2_fields() const;
	std::set<int> newed_remove2_cfg(const config& that) const;

	int type;
	tmode mode;
	config cfg;
};
extern const tadjust null_adjust;

class unit: public base_unit
{
public:
	enum {NONE, WIDGET, WINDOW, COLUMN, ROW};
	struct tchild
	{
		tchild()
			: window(NULL)
		{}

		void clear(bool del)
		{
			if (del) {
				if (window) {
					delete window;
				}
				for (std::vector<unit*>::const_iterator it = rows.begin(); it != rows.end(); ++ it) {
					delete *it;
				}
				for (std::vector<unit*>::const_iterator it = cols.begin(); it != cols.end(); ++ it) {
					delete *it;
				}
				for (std::vector<unit*>::const_iterator it = units.begin(); it != units.end(); ++ it) {
					delete *it;
				}
			}
			window = NULL;
			rows.clear();
			cols.clear();
			units.clear();
		}

		bool is_all_spacer() const
		{
			for (std::vector<unit*>::const_iterator it = units.begin(); it != units.end(); ++ it) {
				const unit* u = *it;
				if (!u->is_spacer()) {
					return false;
				}
			}
			return true;
		}

		void generate(config& cfg) const;
		void from(mkwin_controller& controller, mkwin_display& disp, unit_map& units2, unit* parent, int number, const config& cfg);
		void generate_adjust(const tmode& mode, config& cfg) const;

		void draw_minimap_architecture(surface& screen, const SDL_Rect& minimap_location, const double xscaling, const double yscaling, int level) const;

		// delete unit in unit_map
		void erase(unit_map& units2);

		unit* find_unit(const std::string& id) const;
		std::pair<unit*, int> find_layer(const std::string& id) const;

		unit* window;
		std::vector<unit*> rows;
		std::vector<unit*> cols;
		std::vector<unit*> units;
	};

	struct tparent
	{
		tparent(unit* u, int number = -1)
			: u(u)
			, number(number)
		{}

		unit* u;
		int number;
	};

	static const std::string widget_prefix;
	static const std::string tpl_type;
	static const std::string tpl_widget_prefix;
	static const std::string tpl_id_prefix;

	static std::string form_widget_png(const std::string& type, const std::string& definition);
	static std::string form_widget_tpl(const std::string& id);
	static std::string form_tpl_widget_id(const std::string& tpl_id);
	static bool is_widget_tpl(const std::string& tpl);
	static bool is_tpl_id(const std::string& id);
	static std::string extract_from_widget_tpl(const std::string& tpl);
	static std::string extract_from_tpl_widget_id(const std::string& id);

	unit(mkwin_controller& controller, mkwin_display& disp, unit_map& units, const std::pair<std::string, gui2::tcontrol_definition_ptr>& widget, unit* parent, int number);
	unit(mkwin_controller& controller, mkwin_display& disp, unit_map& units, int type, unit* parent, int number);
	unit(const unit& that);
	virtual ~unit();

	bool require_sort() const { return true; }
	void redraw_unit();
	const std::pair<std::string, gui2::tcontrol_definition_ptr>& widget() const { return widget_; }

	std::string image() const;
	void set_click_dismiss(bool val) { cell_.window.click_dismiss = val; }
	gui2::tcell_setting& cell() { return cell_; }
	const gui2::tcell_setting& cell() const { return cell_; }
	void set_cell(const gui2::tcell_setting& cell);

	void set_child(int number, const tchild& child);
	bool has_child() const { return !children_.empty(); }
	tchild& child(int index) { return children_[index]; }
	const tchild& child(int index) const { return children_[index]; }
	const std::vector<tchild>& children() const { return children_; }
	std::vector<tchild>& children() { return children_; }
	int children_layers() const;

	bool is_spacer() const { return type_ == WIDGET && widget_.first == "spacer"; }
	bool is_track() const { return type_ == WIDGET && widget_.first == "track"; }
	bool is_grid() const { return type_ == WIDGET && widget_.first == "grid"; }
	bool is_stacked_widget() const { return type_ == WIDGET && widget_.first == "stacked_widget"; }
	bool is_drawing() const { return type_ == WIDGET && widget_.first == "drawing"; }
	bool is_listbox() const { return type_ == WIDGET && widget_.first == "listbox"; }
	bool is_scroll_label() const { return type_ == WIDGET && widget_.first == "scroll_label"; }
	bool is_scroll_text_box() const { return type_ == WIDGET && widget_.first == "scroll_text_box"; }
	bool is_scrollbar_panel() const { return type_ == WIDGET && widget_.first == "scrollbar_panel"; }
	bool is_tree_view() const { return type_ == WIDGET && widget_.first == "tree_view"; }
	bool is_report() const { return type_ == WIDGET && widget_.first == "report"; }
	bool is_slider() const { return type_ == WIDGET && widget_.first == "slider"; }
	bool is_text_box2() const { return type_ == WIDGET && widget_.first == "text_box2"; }
	bool is_scroll() const { return is_listbox() || is_scroll_label() || is_scroll_text_box() || is_scrollbar_panel() || is_tree_view() || is_report(); }
	bool is_panel() const { return type_ == WIDGET && (widget_.first == "panel" || widget_.first == "toggle_panel"); }
	bool has_special_setting() const { return is_drawing(); }
	bool is_image() const { return type_ == WIDGET && widget_.first == "image"; }
	bool has_size() const { return is_scroll() || is_drawing() || is_spacer() || is_image() || is_track(); }
	bool has_drag() const { return widget_.first == "button" || widget_.first == "toggle_panel"; }
	bool is_main_map() const;

	const tparent& parent() const { return parent_; }
	void set_parent(unit* parent, int number) { parent_ = tparent(parent, number); }
	void set_parent_number(int number) { parent_.number = number; }
	const unit* parent_at_top() const;
	int type() const { return type_; }

	void insert_child(int w, int h);
	void erase_child(int index);
	void insert_listbox_child(int w, int h);
	void insert_treeview_child();
	std::string child_tag(int index) const;

	const std::vector<tadjust>& adjusts() { return adjusts_; }
	tadjust adjust(const tmode& mode, int type) const;
	void insert_adjust(const tadjust& adjust);
	void erase_adjust(const tmode& mode, int type);

	void generate(config& cfg) const;
	void from(const config& cfg);
	void from_widget(const config& cfg, bool unpack);

	void set_widget_definition(const gui2::tcontrol_definition_ptr& definition) { widget_.second = definition; }
	std::string widget_tag() const;

	void generate_adjust(const tmode& mode, config& cfg);
	bool is_tpl() const;
	bool fix_rect() const { return !cell_.rect_cfg["rect"].empty(); }

	config generate_change2_cfg() const;
	config generate2_change2_cfg(const gui2::tcell_setting& cell) const;

	config get_base_change_cfg(const tmode& mode, bool rect, const config& main_dim_cfg) const;
	void adjust_clear_rect_cfg(const tmode* mode);

	bool get_base_remove(const tmode& mode) const;
	bool has_remove_adjust() const;

	config get_base_remove2_cfg(const tmode& mode) const;

protected:
	void redraw_widget(int xsrc, int ysrc, int width, int height) const;

	virtual void generate_window(config& cfg) const;
	void generate_row(config& cfg) const;
	void generate_column(config& cfg) const;
	void generate_widget(config& cfg) const;
	void generate_widget_tpl(config& cfg) const;
	void generate_grid(config& cfg) const;
	void generate_stacked_widget(config& cfg) const;
	void generate_listbox(config& cfg) const;
	void generate_toggle_panel(config& cfg) const;
	void generate_scrollbar_panel(config& cfg) const;
	void generate_tree_view(config& cfg) const;
	void generate_report(config& cfg) const;
	void generate_slider(config& cfg) const;
	void generate_text_box2(config& cfg) const;
	void generate_drawing(config& cfg) const;
	
	virtual void from_window(const config& cfg);
	void from_row(const config& cfg);
	void from_column(const config& cfg);
	bool from_widget_tpl(const config& cfg);
	void from_grid(const config& cfg);
	void from_stacked_widget(const config& cfg);
	void from_listbox(const config& cfg);
	void from_toggle_panel(const config& cfg);
	void from_scrollbar_panel(const config& cfg);
	void from_tree_view(const config& cfg);
	void from_report(const config& cfg);
	void from_slider(const config& cfg);
	void from_text_box2(const config& cfg);
	void from_drawing(const config& cfg);

protected:
	mkwin_controller& controller_;
	mkwin_display& disp_;
	unit_map& units_;

	int type_;
	tparent parent_;
	std::pair<std::string, gui2::tcontrol_definition_ptr> widget_;
	gui2::tcell_setting cell_;
	std::vector<tchild> children_;

	std::vector<tadjust> adjusts_;
};

std::string formual_extract_str(const std::string& str);

#endif
