/* $Id: title_screen.hpp 48740 2011-03-05 10:01:34Z mordante $ */
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

#ifndef GUI_DIALOGS_ROSE_HPP_INCLUDED
#define GUI_DIALOGS_ROSE_HPP_INCLUDED

#include "gui/dialogs/dialog.hpp"
#include "lobby.hpp"
#include "build.hpp"

class display;
class hero_map;
class hero;

namespace gui2 {

class ttree_view_node;

/**
 * This class implements the title screen.
 *
 * The menu buttons return a result back to the caller with the button pressed.
 * So at the moment it only handles the tips itself.
 *
 * @todo Evaluate whether we can handle more buttons in this class.
 */
class trose : public tdialog, public tlobby::thandler, public tbuild
{
public:
	class tselect_all_lock
	{
	public:
		tselect_all_lock(trose& rose)
			: rose_(rose)
			, original_(rose.select_all_)
		{
			rose_.select_all_ = true;
		}
		~tselect_all_lock()
		{
			rose_.select_all_ = original_;
		}

	private:
		trose& rose_;
		bool original_;
	};

	trose(display& disp, hero& player_hero);
	~trose();

	/**
	 * Values for the menu-items of the main menu.
	 *
	 * @todo Evaluate the best place for these items.
	 */
	enum tresult {
			EDIT_DIALOG = 1     /**< Let user select a campaign to play */
			, PLAYER
			, EDIT_THEME
			, CHANGE_LANGUAGE
			, MESSAGE
			, EDIT_PREFERENCES
			, DESIGN
			, QUIT_GAME
			, NOTHING             /**<
			                       * Default, nothing done, no redraw needed
			                       */
			};

	bool handle(int tag, tsock::ttype type, const config& data);
	bool walk_dir2(const std::string& dir, const SDL_dirent2* dirent, ttree_view_node* root);

private:
	/** Inherited from tdialog, implemented by REGISTER_DIALOG. */
	virtual const std::string& window_id() const;

	/** Inherited from tdialog. */
	void pre_show(CVideo& video, twindow& window);

	void set_retval(twindow& window, int retval);
	void icon_toggled(twidget* widget);

	bool compare_sort(const ttree_view_node& a, const ttree_view_node& b);
	void fill_items(twindow& window);

	void pre_base(twindow& window);

	void check_build_toggled(twidget* widget);
	void set_working_dir(twindow& window);
	void do_refresh(twindow& window);
	void do_build(twindow& window);
	void set_build_active(twindow& window);
	void on_start_build();
	void on_stop_build();
	void handle_task();

	void on_change_working_dir(twindow& window, const std::string& dir);

	void right_click_explorer(ttree_view_node& node, bool& handled, bool& halt);
	void extract_app(tapp_copier& copier);
	void generate_mod(tmod_copier& current_mod);
	bool generate_kingdom_mod_res(tmod_copier& mod_config, const std::string& kingdom_res, const std::string& kingdom_star_patch, const std::string& kingdom_star);

	ttree_view_node& add_explorer_node(const std::string& dir, ttree_view_node& parent, const std::string& name, bool isdir);
	void build_finished(twindow& window);

private:
	display& disp_;
	hero& player_hero_;
	twindow* window_;

	std::vector<tmod_copier> mod_copiers_;
	std::vector<tapp_copier> app_copiers_;
	bool select_all_;
	bool back_2_path_;

	const tapp_copier* current_copier_;

	struct tcookie {
		tcookie(std::string dir, std::string name, bool isdir)
			: dir(dir)
			, name(name)
			, isdir(isdir)
		{}

		std::string dir;
		std::string name;
		bool isdir;
	};
	std::map<const ttree_view_node*, tcookie> cookies_;

	std::vector<tmod_copier> mod_copiers;
	std::vector<tapp_copier> app_copiers;
};

} // namespace gui2

#endif
