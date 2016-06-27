/* $Id: campaign_difficulty.hpp 49603 2011-05-22 17:56:17Z mordante $ */
/*
   Copyright (C) 2010 - 2011 by Ignacio Riquelme Morelle <shadowm2006@gmail.com>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef GUI_DIALOGS_CHAT_HPP_INCLUDED
#define GUI_DIALOGS_CHAT_HPP_INCLUDED

#include "gui/dialogs/dialog.hpp"
#include "lobby.hpp"
#include <integrate.hpp>
#include "gui/widgets/widget.hpp"

class config;
class display;
class tgroup;

namespace gui2 {

class tlabel;
class tbutton;
class tspacer;
class ttree_view_node;
class ttoggle_button;
class tscroll_label;
class ttext_box;
class tscroll_text_box;
class tstacked_widget;
class treport;
class ttree_view;
class tlistbox;
class tstacked_widget;
class tgrid;

class tchat_: public tdialog, public tlobby::thandler
{
public:
	static std::string err_encode_str;

	enum {CONTACT_LAYER, FIND_LAYER, CHANNEL_LAYER, MSG_LAYER};

	struct tcookie 
	{
		tcookie(size_t signature = 0, ttree_view_node* node = NULL, const std::string& nick = null_str, int id = tlobby_channel::npos, bool channel = true)
			: signature(signature)
			, node(node)
			, nick(nick)
			, id(id)
			, channel(channel)
			, unread(0)
			, online(false)
			, away(false)
		{}

		bool valid_user() const { return !channel; }

		size_t signature;
		ttree_view_node* node;
		std::string nick;
		int id;
		bool channel;
		int unread;
		bool online;
		bool away;
	};

	enum {f_none, f_min, f_netdiag = f_min, f_copy, f_reply, f_face, f_part, 
		f_part_friend, f_explore, f_max = f_explore};
	enum {ft_none = 0x1, ft_person = 0x2, ft_channel = 0x4, ft_chan_person = 0x8};
	struct tfunc 
	{
		tfunc(int id);

		int id;
		int type;
		std::string image;
		std::string tooltip;
	};

	struct tsession
	{
		static int logs_per_page;
		tsession(chat_logs::treceiver& receiver);

		int current_logs(std::vector<chat_logs::tlog>& logs) const;
		int pages() const;
		bool can_previous() const;
		bool can_next() const;
		const chat_logs::tlog& log(int at) const;

		chat_logs::treceiver* receiver;
		std::vector<chat_logs::tlog> history;
		int current_page;
	};

	tchat_(display& disp, int chat_page);
	virtual ~tchat_();

	virtual void handle_status(int at, tsock::ttype type);
	virtual bool handle_raw(int at, tsock::ttype type, const char* param[]);
protected:
	/** Inherited from tdialog. */
	void pre_show(twindow& window);

	/** Inherited from tdialog. */
	void post_show(twindow& window);

	void process_message(const std::string& chann, const std::string& from, const std::string& text);
	void process_userlist(const std::string& chan, const std::string& names);
	void process_userlist_end(const std::string& chan);
	void process_uparted(const std::string& chan);
	void process_part(const std::string& chan, const std::string& nick, const std::string& reason);
	void process_join(const std::string& chan, const std::string& nick);
	void process_whois(const std::string& chan, const std::string& nick, bool online, bool away);
	void process_quit(const std::string& nick);
	void process_chanlist_start();
	void process_chanlist(const std::string& chan, int users, const std::string& topic);
	void process_chanlist_end();
	void process_online(const char* nicks);
	void process_offline(const char* nicks);
	void process_forbid_join(const std::string& chan, const std::string& reason);

	void process_network_status(bool connected);

	virtual void desire_swap_page(twindow& window, int page, bool open) {}

	void contact_person_toggled(twidget* widget);
	void contact_channel_toggled(twidget* widget);
	void simulate_cookie_toggled(bool person, int cid, int id, bool channel);

	void user_double_click_callback(twidget* widget);
	void netdiag(twindow& window);
	void send(bool& handled, bool& halt);
	void find(twindow& window);
	void join_channel(twindow& window);
	void insert_face(twindow& window, int index);
	void signal_handler_sdl_key_down(bool& halt
		, bool& handled
		, const SDLKey key
		, SDLMod modifier
		, const Uint16 unicode);

	void find_chan_toggled(twindow& window, tlistbox& list, const int type = twidget::drag_none);

	void channel2_toggled(twidget* widget);
	void refresh_channel2_toolbox(const tlobby_user& user);
	void chat_to(twindow& window);
	void join_friend(twindow& window);

	std::vector<tcookie>& ready_branch(bool person, const tlobby_channel& channel);
	void update_node_internal(const std::vector<tcookie>& cookies, const tcookie& cookie);
	std::pair<std::vector<tcookie>*, tcookie* > contact_find(bool person, size_t signature);
	std::pair<std::vector<tcookie>*, tcookie* > contact_find(bool person, int cid, int id, bool channel);
	std::pair<const std::vector<tcookie>*, const tcookie* > contact_find(bool person, int cid, int id, bool channel) const;

	// std::string format_log_str(const tsession& sess, std::vector<tintegrate::tlocator>& locator) const;
	void format_log_2_listbox(tlistbox& list, const tsession& sess, int cursel) const;

	void chat_2_scroll_label(tlistbox& list, const tsession& sess, int cursel = twidget::npos);
	void ready_face(twindow& window);

	void swap_page(twindow& window, int page, bool swap);

	void toggle_report(twidget* widget);
	bool click_report(twidget* widget);

	tsession& get_session(chat_logs::treceiver& receiver, bool allow_create = false);
	void switch_session(bool person, std::vector<tcookie>& branch, tcookie& cookie);

	bool gui_ready() const;
	void generate_channel_tree(tlobby_channel& channel);

private:
	void switch_to_contact(twindow& window);
	void switch_to_find(twindow& window);
	void switch_to_channel(twindow& window);
	void switch_to_msg(twindow& window);

	void reload_toolbar(twindow& window);
	void refresh_toolbar(int type, int id);

	void reload_catalog(twindow& window);
	void contact_switch_to_person();
	void contact_switch_to_channel();
	void user_to_title(const tcookie& cookie) const;
	void insert_user(bool person, std::vector<tcookie>& branch, const tlobby_user& user);
	void erase_user(bool person, std::vector<tcookie>& branch, int uid);
	void clear_branch(bool person, int type);
	void remove_branch(bool person, int type);
	void handle_multiline(const char* nick, const char* chan, const char* text);
	void previous_page(twindow& window);
	void next_page(twindow& window);

	void enter_inputing(bool enter);

protected:
	size_t signature_;
	std::map<int, std::vector<tcookie> > person_cookies_;
	std::map<int, std::vector<tcookie> > channel_cookies_;
	std::vector<tcookie> channel2_branch_;
	config gamelist_;
	std::vector<std::string> users_;

	int current_page_;
	tstacked_widget* page_panel_;
	std::vector<std::string> chans_;
	treport* catalog_;
	treport* toolbar_;
	std::vector<tfunc> funcs_;
	std::vector<tsession> sessions_;
	tsession* current_session_;
	bool inputing_;
	bool swap_resultion_;
	bool portrait_;

private:
	display& disp_;
	twindow* window_;
	tstacked_widget* panel_;
	tgrid* contact_layer_;
	tgrid* find_layer_;
	tgrid* channel_layer_;
	tgrid* msg_layer_;
	ttree_view* person_tree_;
	ttree_view* channel_tree_;
	ttree_view* channel2_tree_;
	tbutton* previous_page_;
	tbutton* next_page_;
	tlabel* pagenum_;
	tlistbox* history_;
	tscroll_text_box* input_;
	ttext_box* input_tb_;
	tspacer* input_scale_;
	tbutton* send_;
	tbutton* find_;
	tbutton* switch_to_chat_find_;
	tbutton* switch_to_chat_channel_;
	tbutton* join_channel_;
	tlistbox* chanlist_;
	tbutton* chat_to_;
	tbutton* join_friend_;
	int current_ft_;

	bool in_find_chan_;
	int cond_min_users_;
	std::vector<std::string> list_chans_;

	int src_pos_;
	int chat_page_;
};

class tchat2: public tchat_
{
public:
	explicit tchat2(display& disp);

private:
	/** Inherited from tdialog, implemented by REGISTER_DIALOG. */
	virtual const std::string& window_id() const;

	/** Inherited from tdialog. */
	void pre_show(CVideo& video, twindow& window);

	enum {CHAT_PAGE, CHATING_PAGE, NONE_PAGE};
	
	void handle_status(int at, tsock::ttype type);

private:
	display& disp_;
};


}


#endif /* ! GUI_DIALOGS_CAMPAIGN_DIFFICULTY_HPP_INCLUDED */
