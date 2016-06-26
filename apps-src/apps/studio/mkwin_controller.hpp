/* $Id: editor_controller.hpp 47816 2010-12-05 18:08:38Z mordante $ */
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

#ifndef STUDIO_MKWIN_CONTROLLER_HPP_INCLUDED
#define STUDIO_MKWIN_CONTROLLER_HPP_INCLUDED

#include "controller_base.hpp"
#include "mouse_handler_base.hpp"
#include "mkwin_display.hpp"
#include "unit_map.hpp"
#include "map.hpp"
#include "gui/auxiliary/window_builder.hpp"

enum EXIT_STATUS {
	EXIT_NORMAL,
	EXIT_QUIT_TO_DESKTOP,
	EXIT_RELOAD_DATA,
	EXIT_ERROR
};

struct tmenu2
{
	struct titem
	{
		titem(const std::string& id, tmenu2* submenu, bool hide, bool param)
			: id(id)
			, submenu(submenu)
			, hide(hide)
			, param(param)
		{}

		std::string id;
		tmenu2* submenu;
		bool hide;
		bool param;
	};

	tmenu2(const std::string& report, const std::string& id, tmenu2* parent)
		: report(report)
		, id(id)
		, items()
		, parent(parent)
	{}
	~tmenu2();
	
	tmenu2* top_menu();
	bool id_existed(const std::string& id) const;
	void submenus(std::vector<tmenu2*>& result) const;
	void generate(config& cfg) const;

	std::string report;
	std::string id;
	std::vector<titem> items;
	tmenu2* parent;
};

/**
 * The editor_controller class containts the mouse and keyboard event handling
 * routines for the editor. It also serves as the main editor class with the
 * general logic.
 */
class mkwin_controller : public controller_base, public events::mouse_handler_base
{
public:
	static const int default_child_width;
	static const int default_child_height;
	static const int default_dialog_width;
	static const int default_dialog_height;

	static std::string widget_template_cfg();

	class tused_widget_tpl_lock
	{
	public:
		tused_widget_tpl_lock(mkwin_controller& controller)
			: controller_(controller)
		{
			controller_.used_widget_tpl_.clear();
		}
		~tused_widget_tpl_lock()
		{
			controller_.used_widget_tpl_.clear();
		}

	private:
		mkwin_controller& controller_;
	};

	class tfind_rect_cfg_lock
	{
	public:
		tfind_rect_cfg_lock(mkwin_controller& controller, const config& cfg)
			: controller_(controller)
		{
			controller_.top_rect_cfg_ = &cfg;
		}
		~tfind_rect_cfg_lock()
		{
			controller_.top_rect_cfg_ = NULL;
		}

	private:
		mkwin_controller& controller_;
	};

	class tenumerate_lock
	{
	public:
		tenumerate_lock(mkwin_controller& controller, boost::function<bool (const unit*, bool)> fcallback)
			: controller_(controller)
			, original_(controller.fcallback_)
		{
			controller_.fcallback_ = fcallback;
		}

		~tenumerate_lock()
		{
			controller_.fcallback_ = original_;
			controller_.callback_sstr_.clear();
		}

	private:
		mkwin_controller& controller_;
		boost::function<bool (const unit*, bool)> original_;
	};

	class ttheme_top_lock
	{
	public:
		ttheme_top_lock(mkwin_controller& controller);
		~ttheme_top_lock();

	private:
		mkwin_controller& controller_;
		bool require_clear_;
	};

	/**
	 * The constructor. A initial map context can be specified here, the controller
	 * will assume ownership and delete the pointer during destruction, but changes
	 * to the map can be retrieved between the main loop's end and the controller's
	 * destruction.
	 */
	mkwin_controller(const config &top_config, CVideo& video, bool theme);

	~mkwin_controller();

	/** Editor main loop */
	EXIT_STATUS main_loop();

	void init_gui(CVideo& video);
	void init_music(const config& top_config);

	mkwin_display& gui() { return *gui_; }
	const mkwin_display& gui() const { return *gui_; }

	bool theme() const { return theme_; }

	const map_location& selected_hex() const { return selected_hex_; }
	unit* get_window() const;
	bool window_has_valid(bool show_error);

	void mouse_motion(int x, int y, const bool browse);
	bool left_mouse_down(int x, int y, const bool browse);
	bool right_click(int x, int y, const bool browse);

	void click_widget(const std::string& type, const std::string& definition);
	const std::vector<gui2::tlinked_group>& linked_groups() const { return linked_groups_; }
	void set_status();
	const unit* copied_unit() const { return copied_unit_; }
	void set_copied_unit(unit* u);

	base_unit* last_unit() const { return last_unit_; }
	bool in_theme_top() const { return theme_ && !current_unit_; }
	unit* current_unit() const { return current_unit_; }

	bool toggle_report(gui2::twidget* widget);

	bool in_context_menu(const std::string& id) const;
	bool actived_context_menu(const std::string& id) const;

	void do_right_click();
	void select_unit(unit* u);

	void post_change_resolution();

	/* controller_base overrides */
	mouse_handler_base& get_mouse_handler_base();
	mkwin_display& get_display() { return *gui_; }
	const mkwin_display& get_display() const { return *gui_; }

	bool verify_new_mode(const std::string& str);
	void insert_mode(const std::string& id);
	void erase_patch(const std::string id);
	void rename_patch(const std::string from, const std::string& id);
	const std::vector<tmode>& modes() const { return modes_; }
	const tmode& mode(int at) const { return modes_[at]; }

	const unit::tchild& top() const { return top_; }
	std::vector<unit*> form_top_units() const;
	const config& find_rect_cfg(const std::string& widget, const std::string& id) const;

	bool callback_valid_id(const unit* u, bool show_error);

	unit_map& get_units() { return units_; }
	const unit_map& get_units() const { return units_; }

	void layout_dirty(bool force_change_map = false);

	void insert_used_widget_tpl(const config& tpl_cfg);

	void form_linked_groups(const config& res_cfg);
	void generate_linked_groups(config& res_cfg) const;

	std::vector<tmenu2>& context_menus() { return context_menus_; }
	const std::vector<tmenu2>& context_menus() const { return context_menus_; }

	void form_context_menus(const config& res_cfg);
	void generate_context_menus(config& res_cfg);

private:
	void reset_modes();

	/** command_executor override */
	void execute_command2(int command, const std::string& sparam);

	void reload_map(int w, int h, bool redraw);

	void widget_setting(bool special);
	void rect_changed(unit* u);
	void linked_group_setting();
	void erase_widget();
	void copy_widget(unit* u, bool cut);
	void paste_widget();
	void run();
	void system();
	void load_window();
	void save_window(bool as);
	void quit_confirm(EXIT_STATUS mode, bool dirty);
	void insert_row(unit* u, bool top);
	void erase_row(unit* u);
	void insert_column(unit* u, bool left);
	void erase_column(unit* u);

	void theme_into_widget(unit* u);
	void theme_goto_top(bool auto_select);

	void derive_create(unit* u);
	bool can_copy(const unit* u, bool cut) const;
	bool can_paste(const unit* u) const;

	bool can_adjust_row(const unit* u) const;
	bool can_adjust_column(const unit* u) const;
	bool can_erase(const unit* u) const;

	void fill_spacer(unit* parent, int number);
	config generate_window_cfg() const;
	config generate_theme_cfg(const std::vector<unit*>& units);

	const config& find_rect_cfg2(const config& cfg, const std::string& widget, const std::string& id) const;

	bool enumerate_child(const std::vector<unit*>& top_units, bool show_error) const;
	bool enumerate_child2(const unit::tchild& child, bool show_error) const;

	void replace_child_unit(unit* u);
	void unpack_widget_tpl(unit* u);
	void refresh_title();

	void theme_load_patches(const unit::tchild& top, const config& theme_cfg);
	void load_patch_res(const unit::tchild& top, const tmode& patch, const config& cfg);
	void generate_theme_adjust(const tmode& mode, config& cfg) const;

	void form_context_menu(tmenu2& menu, const config& cfg, const std::string& menu_id);

	void fill_object_list();
	bool paste_widget2(const unit& from, const map_location to_loc);

	std::vector<std::string> generate_textdomains(const std::string& file, bool theme) const;

private:
	/** The display object used and owned by the editor. */
	mkwin_display* gui_;

	gamemap map_;
	unit_map units_;


	map_location selected_hex_;
	unit* current_unit_;
	unit::tchild top_;
	unit::tchild theme_top_;
	SDL_Rect theme_top_unit_rect_;
	std::vector<std::string> textdomains_;
	std::vector<gui2::tlinked_group> linked_groups_;
	std::vector<tmenu2> context_menus_;
	
	std::pair<std::string, config> original_;
	unit* copied_unit_;
	bool cut_;
	base_unit* last_unit_;
	std::string file_;
	
	bool theme_;
	int theme_widget_start_;
	const config* top_rect_cfg_;

	std::vector<tmode> modes_;

	boost::function<bool (const unit*, bool)> fcallback_;
	std::set<std::string> callback_sstr_;

	std::set<const config*> used_widget_tpl_;

	/** Quit main loop flag */
	bool do_quit_;
	EXIT_STATUS quit_mode_;
};

#endif
