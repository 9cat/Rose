/* $Id: game_config.cpp 46969 2010-10-08 19:45:32Z mordante $ */
/*
   Copyright (C) 2003 - 2010 by David White <dave@whitevine.net>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#include "global.hpp"
#include "rose_config.hpp"

#include "config.hpp"
#include "gettext.hpp"
#include "log.hpp"
#include "color_range.hpp"
#include "util.hpp"
#include "version.hpp"
#include "serialization/string_utils.hpp"
#include "gui/widgets/widget.hpp"
#include "font.hpp"
#include "wml_exception.hpp"
#include "filesystem.hpp"

#include <time.h>

#include <boost/foreach.hpp>

void get_serialnumber(char* sn)
{
	strcpy(sn, "CCQJPWCUF4K1");
}

namespace game_config
{
	std::string app;
	std::string app_channel;
	std::string app_title;
	std::string app_dir; // get from app.
	int base_income = 2;
	int village_income = 2;
	int hero_income = 2;
	int poison_amount= 24;
	int rest_heal_amount= 4;
	int recall_cost = 20;
	int kill_experience = 6;
	unsigned lobby_network_timer = 100;
	unsigned lobby_refresh = 4000;
	const int gold_carryover_percentage = 80;
	const std::string rose_version = "1.0.0";
	std::string version = "1.0.31";
	version_info wesnoth_version(version);
	int reside_troop_increase_loyalty = 50;
	int field_troop_increase_loyalty = 10;
	int wander_loyalty_threshold = 112; // HERO_MAX_LOYALTY / 4 * 3
	int move_loyalty_threshold = 130; // HERO_MAX_LOYALTY - 20
	int ai_keep_loyalty_threshold = wander_loyalty_threshold + 5;
	int ai_keep_hp_threshold = 50;
	int max_troop_food = 15;
	int increase_feeling = 1024; // HERO_MAX_FEELING / 64. 64 turns will carry.
	int minimal_activity = 175;
	int maximal_defeated_activity = 0;
	int tower_fix_heros = 6;
	int rpg_fix_members = 12;
	bool score_dirty = true;
	bool local_only = true;
	int max_police = 100;
	int min_tradable_police = max_police / 2;
	int max_commoners = 5;
	int fixed_tactic_slots = 2;
	int active_tactic_slots = fixed_tactic_slots;
	float tower_cost_ratio = 9.0;

	int default_human_level = 1;
	int default_ai_level = 2;
	int current_level = 1;
	int min_level = 1;
	int max_level = 3;
	int siege_map_w = 8;
	int siege_map_h = 8;

	int max_noble_level = 4;
	int tactic_degree_per_turn = 50;
	int tactic_degree_per_point = 40;
	int threshold_score_ai = 60;
	int hot_level_per_point = 5;
	int formation_least_adjacent = 3;

	int least_fix_members = 4;
	int least_roam_members = 4;
	int max_exile = 12;
	int levels_per_rank = 5;
	int max_member_level = 14;
	int min_employee_level = levels_per_rank + 2;
	int min_exile_level = levels_per_rank;
	int employee_base_score = 0;
	int max_employees = 1;
	int coin_score_rate = 200;
	int score_used_draw = 500;
	int ticks_per_turn = 20000;
	int feature_increase_spirit = 30; // 30%
	bool show_side_report = false;
	const std::string theme_window_id = "theme";


	int max_bomb_turns = 2;
	bool no_messagebox = false;
	bool hide_tactic_slot = false;

	std::string shroud_prefix;
	std::string fog_prefix;

	int kill_xp(int level)
	{
		level += 3;
		return level ? kill_experience * level : kill_experience / 2;
	}
	int attack_xp(int level)
	{
		level += 3;
		return level;
	}
	void recycle_score_income(int& coin, int& score)
	{
		score = (coin * game_config::coin_score_rate + score) / 5;
		
		coin = score / game_config::coin_score_rate;
		score = score - coin * game_config::coin_score_rate;
	}

	std::string checksum;
	bool server_matched = true;
	const std::string service_email = "ancientcc@gmail.com";
	const std::string sales_email = "ancientcc@gmail.com";
	bool debug = false, editor = false, no_delay = false, disable_autosave = false;

	bool use_bin = true;
	int cache_compression_level = 6;
	bool tiny_gui = false;
	int start_cards = 6;
	int cards_per_turn = 2;
	int max_cards = 20;
	const std::string campaign_id_siege = "siege";
	map_location inforce_defender_loc(1, 0);
	map_location inforce_attacker_loc(14, 5);
	int max_siege_troops = 11;
	int max_reinforcements = 6;
	int max_sieges = 15;
	int default_siege_turn = 10;
	int max_breaks = 5;
	int salary_score = 20;
	int develop_score = 10;
	size_t max_slots = 3;
	std::string broadcast_username = "--";
	std::string data_server_magic_word = "5a7c";
	size_t timestamp;
	unsigned char secret_key[6];
	int min_valid_damage = 5;
	int build_hp_per_turn = 50;
	int start_percent_hp = 60;

	int cost_per_level = 10;
	int cost_clear_formationed = 20;
	int cost_intervene_move = 80;
	
	std::string theme_object_id_endturn = "endturn";
	std::string theme_object_id_terrain_group = "terrain_group";
	std::string theme_object_id_brush = "brush";


	std::string sn;
	std::map<int, std::string> inapp_items;

	std::string title_music,
			lobby_music,
			default_victory_music,
			default_defeat_music;

	namespace images {
	std::string game_title,
			// orbs and hp/xp bar
			moved_orb,
			unmoved_orb,
			partmoved_orb,
			automatic_orb,
			enemy_orb,
			ally_orb,
			bar_hrl_72,
			bar_hrl_64,
			bar_hrl_56,
			bar_hrl_48,
			// flags
			flag,
			flag_icon,
			big_flag,
			// GUI elements
			observer,
			tod_bright,
			tod_dark,
			///@todo de-hardcode this
			checked_menu = "buttons/checkbox-pressed.png",
			unchecked_menu = "buttons/checkbox.png",
			wml_menu = "buttons/WML-custom.png",
			level,
			ellipsis,
			missing;
	} //images

	namespace terrain {
		// hex overlay
		std::string short_mask = "alphamask.png",
		short_grid_top = "grid-top.png",
		short_grid_bottom = "grid-bottom.png",
		mouseover,
		selected,
		editor_brush,
		unreachable,
		disctrict,
		linger;

		std::string form_img_prefix(const std::string& tile)
		{
			return std::string("terrain-") + tile + "/";
		}

		void modify_according_tile(const std::string& tile)
		{
			const std::string img_prefix = form_img_prefix(tile);

			mouseover = img_prefix + "hover-hex.png";
			selected = "";
			editor_brush = "editor/brush.png";
			unreachable = img_prefix + "darken.png";
			disctrict = img_prefix + "disctrict.png";
			linger = img_prefix + "darken-linger.png";
		}
	}
	const std::string tile_hex = "hexagonal";
	const std::string tile_square = "square";

	std::string logo_png = "misc/logo.png";

	std::string flag_rgb;
	std::vector<Uint32> red_green_scale;
	std::vector<Uint32> red_green_scale_text;

	double hp_bar_scaling = 0.666;
	double xp_bar_scaling = 0.5;

	double hex_brightening = 1.5;
	double hex_semi_brightening = 1.25;

	const size_t max_loop = 65536;

	unsigned char* savegame_cache = NULL;
	int savegame_cache_size = 1024 * 1024; // 1M

	std::map<std::string, color_range > team_rgb_range;
	std::vector<std::pair<std::string, t_string> > team_rgb_name;
	std::map<std::string, std::vector<Uint32> > team_rgb_colors;


	namespace sounds {
		const std::string turn_bell = "bell.wav",
		timer_bell = "timer.wav",
		receive_message = "msg.wav",
		receive_message_highlight = "chat-highlight.ogg",
		receive_message_friend = "chat-friend.ogg",
		receive_message_server = "receive.wav",
		user_arrive = "arrive.wav",
		user_leave = "leave.wav",
		game_user_arrive = "join.wav",
		game_user_leave = "leave.wav";

		const std::string button_press = "button.wav",
		checkbox_release = "checkbox.wav",
		slider_adjust = "slider.wav",
		menu_expand = "expand.wav",
		menu_contract = "contract.wav",
		menu_select = "select.wav";
	}

	std::string path = "";
	std::string preferences_dir = "";
	std::string preferences_dir_utf8 = "";

	std::set<std::string> reserve_players;
	std::vector<server_info> server_list;
	bbs_server_info bbs_server;

	int navigation_per_level = 10000;
	int equation_of_time;

int calculate_equation_of_time()
{
	time_t utc = time(NULL);
	struct tm *tm = gmtime(&utc);

	struct tm ctc_tm;
	memset(&ctc_tm, 0, sizeof(struct tm));
	ctc_tm.tm_year = tm->tm_year;
	ctc_tm.tm_mon = tm->tm_mon;
	ctc_tm.tm_mday = tm->tm_mday;
	ctc_tm.tm_hour = tm->tm_hour;
	ctc_tm.tm_min = tm->tm_min;
	ctc_tm.tm_sec = tm->tm_sec;


	time_t ctc = mktime(&ctc_tm);
	return ctc - utc;
}

void init(const std::string& _app, const std::string& title, const std::string& channel, bool hdpi, bool landscape)
{
	VALIDATE(!_app.empty() && _app.size() <= MAXLEN_APP && !title.empty(), null_str);
	app = _app;
	app_title = title;
	app_channel = channel;
	gui2::twidget::current_landscape = landscape;
	gui2::twidget::hdpi = hdpi;
	gui2::twidget::hdpi_scale = _app != "editor"? SDL_GetDpiScale(): 1;
	if (!hdpi) {
		gui2::twidget::hdpi_scale = 1;
	}
	VALIDATE(gui2::twidget::hdpi_scale > 0 && gui2::twidget::hdpi_scale < 5, null_str);

	if (hdpi) {
		font::SIZE_XTINY = font::SIZE_XTINY_MDPI * gui2::twidget::hdpi_scale;
		font::SIZE_TINY = font::SIZE_TINY_MDPI * gui2::twidget::hdpi_scale;
		font::SIZE_SMALL = font::SIZE_SMALL_MDPI * gui2::twidget::hdpi_scale;
		font::SIZE_NORMAL = font::SIZE_NORMAL_MDPI * gui2::twidget::hdpi_scale;
		font::SIZE_LARGE = font::SIZE_LARGE_MDPI * gui2::twidget::hdpi_scale;
		font::SIZE_XLARGE = font::SIZE_XLARGE_MDPI * gui2::twidget::hdpi_scale;
	}

	app_dir = std::string("app-") + app;
	strcpy(def_textdomain, (app + "-lib").c_str());
	equation_of_time = calculate_equation_of_time();

	// setup userdata directory
#ifdef ANDROID
	set_preferences_dir(null_str);
#elif defined(__APPLE__)
	set_preferences_dir("Documents");
#else
	set_preferences_dir("kingdom");
#endif

}

std::string extract_app_from_app_dir(const std::string& dir)
{
	if (dir.find("app-") != 0) {
		return null_str;
	}
	return dir.substr(4);
}

std::string generate_app_dir(const std::string& app)
{
	return std::string("app-") + app;
}

void load_config(const config* cfg)
{
	if (cfg == NULL) {
		return;
	}

	const config& v = *cfg;

	base_income = v["base_income"].to_int(2);
	village_income = v["village_income"].to_int(2);
	hero_income = v["hero_income"].to_int(2);
	poison_amount = v["poison_amount"].to_int(24);
	rest_heal_amount = v["rest_heal_amount"].to_int(4);
	recall_cost = v["recall_cost"].to_int(20);
	kill_experience = v["kill_experience"].to_int(6);
	lobby_refresh = v["lobby_refresh"].to_int(2000);

	reside_troop_increase_loyalty = v["reside_troop_increase_loyalty"].to_int(15);
	field_troop_increase_loyalty = v["field_troop_increase_loyalty"].to_int(4);
	wander_loyalty_threshold = v["wander_loyalty_threshold"].to_int(112); // HERO_MAX_LOYALTY / 4 * 3
	move_loyalty_threshold = v["wander_loyalty_threshold"].to_int(130); // HERO_MAX_LOYALTY - 20
	ai_keep_loyalty_threshold = wander_loyalty_threshold + 13;
	ai_keep_hp_threshold = v["ai_keep_hp_threshold"].to_int(50);
	max_troop_food = v["max_troop_food"].to_int(15);
	max_police = v["max_police"].to_int(100);
	min_tradable_police = v["min_tradable_police"].to_int(max_police / 2);
	max_commoners = v["max_commoners"].to_int(5);

	minimal_activity = v["minimal_activity"].to_int(175);
	if (minimal_activity > 0xff) {
		minimal_activity = 175;
	} else if (minimal_activity < 2) {
		minimal_activity = 2;
	}

	max_cards = v["max_cards"].to_int(20);
	default_ai_level = v["default_ai_level"].to_int(2);

	{
		char _sn[128];
		get_serialnumber(_sn);
		sn = _sn;
	}
	inapp_items[transaction_type_vip] = "vip2";
	inapp_items[transaction_type_2coin] = "score";
	inapp_items[transaction_type_8coin] = "8coin";
	inapp_items[transaction_type_45coin] = "45coin";
	inapp_items[transaction_type_100coin] = "100coin";

	title_music = v["title_music"].str();
	lobby_music = v["lobby_music"].str();
	default_victory_music = v["default_victory_music"].str();
	default_defeat_music = v["default_defeat_music"].str();

	if (const config &i = v.child("images")){
		using namespace game_config::images;
		game_title = i["game_title"].str();

		moved_orb = i["moved_orb"].str();
		unmoved_orb = i["unmoved_orb"].str();
		partmoved_orb = i["partmoved_orb"].str();
		automatic_orb = i["automatic_orb"].str();
		enemy_orb = i["enemy_orb"].str();
		ally_orb = i["ally_orb"].str();
		bar_hrl_72 = i["bar_hrl_72"].str();
		bar_hrl_64 = i["bar_hrl_64"].str();
		bar_hrl_56 = i["bar_hrl_56"].str();
		bar_hrl_48 = i["bar_hrl_48"].str();

		flag = i["flag"].str();
		flag_icon = i["flag_icon"].str();
		big_flag = i["big_flag"].str();

		observer = i["observer"].str();
		tod_bright = i["tod_bright"].str();
		tod_dark = i["tod_dark"].str();
		level = i["level"].str();
		ellipsis = i["ellipsis"].str();
		missing = i["missing"].str();
	} // images

	hp_bar_scaling = v["hp_bar_scaling"].to_double(0.666);
	xp_bar_scaling = v["xp_bar_scaling"].to_double(0.5);
	hex_brightening = v["hex_brightening"].to_double(1.5);
	hex_semi_brightening = v["hex_semi_brightening"].to_double(1.25);

	shroud_prefix = v["shroud_prefix"].str();
	fog_prefix  = v["fog_prefix"].str();

	game_config::add_color_info(v);

	game_config::server_list.clear();
	BOOST_FOREACH (const config &server, v.child_range("server"))
	{
		game_config::server_info sinf;
		sinf.name = server["name"].str();
		sinf.address = server["address"].str();
		game_config::server_list.push_back(sinf);
	}

	if (const config& c = v.child("bbs_server")) {
		game_config::bbs_server.name = c["name"].str();
		const std::string& address = c["address"];
		int pos = address.find_first_of(":");
		if	(pos == -1) {
			game_config::bbs_server.host = address;
			game_config::bbs_server.port = 80;
		} else {
			game_config::bbs_server.host = address.substr(0, pos);
			game_config::bbs_server.port = lexical_cast_default<unsigned int>(address.substr(pos + 1), 80);
		}
		game_config::bbs_server.url = c["url"].str();
		if (game_config::bbs_server.url.empty() || game_config::bbs_server.url[0] != '/') {
			game_config::bbs_server.url = std::string("/") + game_config::bbs_server.url;
		}
	} else {
		game_config::bbs_server.name = "localhost";
		game_config::bbs_server.host = "localhost";
		game_config::bbs_server.port = 80;
		game_config::bbs_server.url = "/bbs";
	}
}

bool is_reserve_player(const std::string& player)
{
	return reserve_players.find(player) != reserve_players.end();
}

void add_color_info(const config &v)
{
	BOOST_FOREACH (const config &teamC, v.child_range("color_range"))
	{
		const config::attribute_value *a1 = teamC.get("id"),
			*a2 = teamC.get("rgb");
		if (!a1 || !a2) continue;
		std::string id = *a1;
		std::vector<Uint32> temp = string2rgb(*a2);
		team_rgb_range.insert(std::make_pair(id,color_range(temp)));
		team_rgb_name.push_back(std::make_pair(id, teamC["name"].t_str()));
		//generate palette of same name;
		std::vector<Uint32> tp = palette(team_rgb_range[id]);
		if (tp.empty()) continue;
		team_rgb_colors.insert(std::make_pair(id,tp));
		//if this is being used, output log of palette for artists use.
		std::ostringstream str;
		str << id << " = ";
		for (std::vector<Uint32>::const_iterator r = tp.begin(),
			    r_end = tp.end(), r_beg = r; r != r_end; ++r)
		{
			int red = ((*r) & 0x00FF0000) >> 16;
			int green = ((*r) & 0x0000FF00) >> 8;
			int blue = ((*r) & 0x000000FF);
			if (r != r_beg) str << ',';
			str << red << ',' << green << ',' << blue;
		}
	}

	std::map<std::string, color_range >& team_rgb_range_p = team_rgb_range;
	std::vector<std::pair<std::string, t_string > >& team_rgb_name_p = team_rgb_name;
	std::map<std::string, std::vector<Uint32> >& team_rgb_colors_p = team_rgb_colors;

	BOOST_FOREACH (const config &cp, v.child_range("color_palette"))
	{
		BOOST_FOREACH (const config::attribute &rgb, cp.attribute_range())
		{
			try {
				team_rgb_colors.insert(std::make_pair(rgb.first, string2rgb(rgb.second)));
			} catch (bad_lexical_cast&) {
				// throw config::error(_("Invalid team color: ") + rgb_it->second);
				// ERR_NG << "Invalid team color: " << rgb.second << "\n";
			}
		}
	}
}

const color_range& color_info(const std::string& name)
{
	std::map<std::string, color_range>::const_iterator i = team_rgb_range.find(name);
	if(i == team_rgb_range.end()) {
		try {
			team_rgb_range.insert(std::make_pair(name,color_range(string2rgb(name))));
			return color_info(name);
		} catch (bad_lexical_cast&) {
			//ERR_NG << "Invalid color range: " << name;
			//return color_info();
			throw config::error(_("Invalid color range: ") + name);
		}
	}
	return i->second;
}

const std::vector<Uint32>& tc_info(const std::string& name)
{
	std::map<std::string, std::vector<Uint32> >::const_iterator i = team_rgb_colors.find(name);
	if(i == team_rgb_colors.end()) {
		try {
			team_rgb_colors.insert(std::make_pair(name,string2rgb(name)));
			return tc_info(name);
		} catch(bad_lexical_cast&) {
			static std::vector<Uint32> stv;
			// throw config::error(_("Invalid team color: ") + name);
			// ERR_NG << "Invalid team color: " << name << "\n";
			return stv;
		}
	}
	return i->second;
}

} // game_config
