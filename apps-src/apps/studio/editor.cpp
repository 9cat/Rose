#include "global.hpp"
#include "rose_config.hpp"
#include "filesystem.hpp"
#include "language.hpp"
#include "loadscreen.hpp"
#include "editor.hpp"
#include <sys/stat.h>
#include "display.hpp"
#include "wml_exception.hpp"
#include "gettext.hpp"
#include "gui/dialogs/message.hpp"
#include "serialization/parser.hpp"
#include "formula_string_utils.hpp"
#include "sdl_filesystem.h"

#include "animation.hpp"
#include "builder.hpp"
#include <boost/foreach.hpp>
#include <boost/bind.hpp>

bool is_apps_res(const std::string& folder)
{
	const std::string APPS_RES = "apps-res";
	size_t pos = folder.find(APPS_RES);
	return pos != std::string::npos && pos + APPS_RES.size() == folder.size();
}

bool check_res_folder(const std::string& folder)
{
	std::stringstream ss;
	
	// <wok>\data\_main.cfg
	ss << folder << "\\data\\_main.cfg";
	if (!file_exists(ss.str())) {
		return false;
	}

	return true;
}

static config generate_cfg;

namespace editor_config
{
	config data_cfg;
	int type = BIN_WML;
	std::string campaign_id;

void reload_data_bin()
{
	const config& game_cfg = data_cfg.child("game_config");
	game_config::load_config(game_cfg? &game_cfg : NULL);
}

}

editor::wml2bin_desc::wml2bin_desc()
	: bin_name()
	, wml_nfiles(0)
	, wml_sum_size(0)
	, wml_modified(0)
	, bin_nfiles(0)
	, bin_sum_size(0)
	, bin_modified(0)
	, require_build(false)
{}

void editor::wml2bin_desc::refresh_checksum(const std::string& working_dir)
{
	VALIDATE(valid(), null_str);
	std::string bin_file = working_dir + "/xwml/";
	if (!app.empty()) {
		bin_file += game_config::generate_app_dir(app) + "/";
	}
	bin_file += bin_name;
	wml_checksum_from_file(bin_file, &bin_nfiles, &bin_sum_size, (uint32_t*)&bin_modified);
}

#define BASENAME_DATA		"data.bin"
#define BASENAME_GUI		"gui.bin"
#define BASENAME_LANGUAGE	"language.bin"

// file processor function only support prefixed with game_config::path.
int editor::tres_path_lock::deep = 0;
editor::tres_path_lock::tres_path_lock(editor& o)
	: original_(game_config::path)
{
	VALIDATE(!deep, null_str);
	deep ++;
	game_config::path = o.working_dir_;
}

editor::tres_path_lock::~tres_path_lock()
{
	game_config::path = original_;
	deep --;
}

editor::editor(const std::string& working_dir) 
	: cache_(game_config::config_cache::instance())
	, working_dir_(working_dir)
	, wml2bin_descs_()
{
}

void editor::set_working_dir(const std::string& dir)
{
	if (working_dir_ == dir) {
		return;
	}
	working_dir_ = dir;
}

bool editor::make_system_bins_exist()
{
	std::string file;
	std::vector<editor::BIN_TYPE> system_bins;
	for (editor::BIN_TYPE type = editor::BIN_MIN; type <= editor::BIN_SYSTEM_MAX; type = (editor::BIN_TYPE)(type + 1)) {
		if (type == MAIN_DATA) {
			file = working_dir_ + "/xwml/" + BASENAME_DATA;
		} else if (type == GUI) {
			file = working_dir_ + "/xwml/" + BASENAME_GUI;
		} else if (type == LANGUAGE) {
			file = working_dir_ + "/xwml/" + BASENAME_LANGUAGE;
		} else {
			VALIDATE(false, null_str);
		}
		if (!file_exists(file)) {
			system_bins.push_back(type);
		}
	}
	if (system_bins.empty()) {
		return true;
	}
	get_wml2bin_desc_from_wml(system_bins);
	const std::vector<std::pair<editor::BIN_TYPE, editor::wml2bin_desc> >& descs = wml2bin_descs();

	int count = (int)descs.size();
	for (int at = 0; at < count; at ++) {
		const std::pair<editor::BIN_TYPE, editor::wml2bin_desc>& desc = descs[at];

		bool ret = false;
		try {
			ret = load_game_cfg(desc.first, desc.second.bin_name, desc.second.app, true, desc.second.wml_nfiles, desc.second.wml_sum_size, (uint32_t)desc.second.wml_modified);
		} catch (twml_exception& /*e*/) {
			return false;
		}
		if (!ret) {
			return false;
		}
	}

	return true;
}

// check location:
//   1. heros_army of artifcal
//   2. service_heros of artifcal
//   3. wander_heros of artifcal
//   4. heros_army of unit
std::string editor::check_scenario_cfg(const config& scenario_cfg)
{
	std::set<std::string> holded_str;
	std::set<int> holded_number;
	std::set<std::string> officialed_str;
	std::map<std::string, std::set<std::string> > officialed_map;
	std::map<std::string, std::string> mayor_map;
	int number;
	std::vector<std::string> str_vec;
	std::vector<std::string>::const_iterator tmp;
	std::stringstream str;

	BOOST_FOREACH (const config& side, scenario_cfg.child_range("side")) {
		const std::string leader = side["leader"];
		BOOST_FOREACH (const config& art, side.child_range("artifical")) {
			officialed_str.clear();
			const std::string cityno = art["cityno"].str();
			mayor_map[cityno] = art["mayor"].str();

			str_vec = utils::split(art["heros_army"]);
			for (tmp = str_vec.begin(); tmp != str_vec.end(); ++ tmp) {
				if (holded_str.count(*tmp)) {
					str << "." << scenario_cfg["id"].str() << ", hero number: " << *tmp << " is conflicted!";
					return str.str();
				}
				number = lexical_cast_default<int>(*tmp);
				if (holded_number.count(number)) {
					str << "." << scenario_cfg["id"].str() << ", hero number: " << *tmp << " is invalid!";
					return str.str();
				}
				holded_str.insert(*tmp);
				holded_number.insert(number);
			}
			str_vec = utils::split(art["service_heros"]);
			for (tmp = str_vec.begin(); tmp != str_vec.end(); ++ tmp) {
				if (holded_str.count(*tmp)) {
					str << "." << scenario_cfg["id"].str() << ", hero number: " << *tmp << " is conflicted!";
					return str.str();
				}
				number = lexical_cast_default<int>(*tmp);
				if (holded_number.count(number)) {
					str << "." << scenario_cfg["id"].str() << ", hero number: " << *tmp << " is invalid!";
					return str.str();
				}
				holded_str.insert(*tmp);
				holded_number.insert(number);
				officialed_str.insert(*tmp);
			}
			str_vec = utils::split(art["wander_heros"]);
			for (tmp = str_vec.begin(); tmp != str_vec.end(); ++ tmp) {
				if (holded_str.count(*tmp)) {
					str << "." << scenario_cfg["id"].str() << ", hero number: " << *tmp << " is conflicted!";
					return str.str();
				}
				number = lexical_cast_default<int>(*tmp);
				if (holded_number.count(number)) {
					str << "." << scenario_cfg["id"].str() << ", hero number: " << *tmp << " is invalid!";
					return str.str();
				}
				holded_str.insert(*tmp);
				holded_number.insert(number);
			}
			officialed_map[cityno] = officialed_str;
		}
		BOOST_FOREACH (const config& u, side.child_range("unit")) {
			const std::string cityno = u["cityno"].str();
			std::map<std::string, std::set<std::string> >::iterator find_it = officialed_map.find(cityno);
			if (cityno != "0" && find_it == officialed_map.end()) {
				str << "." << scenario_cfg["id"].str() << ", heros_army=" << u["heros_army"].str() << " uses undefined cityno: " << cityno << "";
				return str.str();
			}
			str_vec = utils::split(u["heros_army"]);
			for (tmp = str_vec.begin(); tmp != str_vec.end(); ++ tmp) {
				if (holded_str.count(*tmp)) {
					str << "." << scenario_cfg["id"].str() << ", hero number: " << *tmp << " is conflicted!";
					return str.str();
				}
				number = lexical_cast_default<int>(*tmp);
				if (holded_number.count(number)) {
					str << "." << scenario_cfg["id"].str() << ", hero number: " << *tmp << " is invalid!";
					return str.str();
				}
				holded_str.insert(*tmp);
				holded_number.insert(number);
				if (find_it != officialed_map.end()) {
					find_it->second.insert(*tmp);
				}
			}
		}
		for (std::map<std::string, std::set<std::string> >::const_iterator it = officialed_map.begin(); it != officialed_map.end(); ++ it) {
			std::map<std::string, std::string>::const_iterator mayor_it = mayor_map.find(it->first);
			if (mayor_it->second.empty()) {
				continue;
			}
			if (mayor_it->second == leader) {
				str << "." << scenario_cfg["id"].str() << ", in cityno=" << it->first << " mayor(" << mayor_it->second << ") cannot be leader!";
				return str.str();
			}
			if (it->second.find(mayor_it->second) == it->second.end()) {
				str << "." << scenario_cfg["id"].str() << ", in ciytno=" << it->first << " mayor(" << mayor_it->second << ") must be in offical hero!";
				return str.str();
			}
		}
	}
	return "";
}

// check location:
//   1. heros_army of artifcal
//   2. service_heros of artifcal
//   3. wander_heros of artifcal
std::string editor::check_mplayer_bin(const config& mplayer_cfg)
{
	std::set<std::string> holded_str;
	std::set<int> holded_number;
	int number;
	std::vector<std::string> str_vec;
	std::vector<std::string>::const_iterator tmp;
	std::stringstream str;

	BOOST_FOREACH (const config& faction, mplayer_cfg.child_range("faction")) {
		BOOST_FOREACH (const config& art, faction.child_range("artifical")) {
			str_vec = utils::split(art["heros_army"]);
			for (tmp = str_vec.begin(); tmp != str_vec.end(); ++ tmp) {
				if (holded_str.count(*tmp)) {
					str << "hero number: " << *tmp << " is conflicted!";
					return str.str();
				}
				number = lexical_cast_default<int>(*tmp);
				if (holded_number.count(number)) {
					str << "hero number: " << *tmp << " is invalid!";
					return str.str();
				}
				holded_str.insert(*tmp);
				holded_number.insert(number);
			}
			str_vec = utils::split(art["service_heros"]);
			for (tmp = str_vec.begin(); tmp != str_vec.end(); ++ tmp) {
				if (holded_str.count(*tmp)) {
					str << "hero number: " << *tmp << " is conflicted!";
					return str.str();
				}
				number = lexical_cast_default<int>(*tmp);
				if (holded_number.count(number)) {
					str << "hero number: " << *tmp << " is invalid!";
					return str.str();
				}
				holded_str.insert(*tmp);
				holded_number.insert(number);
			}
			str_vec = utils::split(art["wander_heros"]);
			for (tmp = str_vec.begin(); tmp != str_vec.end(); ++ tmp) {
				if (holded_str.count(*tmp)) {
					str << "hero number: " << *tmp << " is conflicted!";
					return str.str();
				}
				number = lexical_cast_default<int>(*tmp);
				if (holded_number.count(number)) {
					str << "hero number: " << *tmp << " is invalid!";
					return str.str();
				}
				holded_str.insert(*tmp);
				holded_number.insert(number);
			}
		}
	}
	return "";
}

// check location:
std::string editor::check_data_bin(const config& data_cfg)
{
	std::stringstream str;

	BOOST_FOREACH (const config& campaign, data_cfg.child_range("campaign")) {
		if (!campaign.has_attribute("id")) {
			str << "Compaign hasn't id!";
			return str.str();
		}
	}
	return "";
}

void editor::generate_app_bin_config()
{
	config bin_cfg;
	bool ret = true;
	std::stringstream ss;
	SDL_DIR* dir = SDL_OpenDir(working_dir_.c_str());
	if (!dir) {
		return;
	}
	SDL_dirent2* dirent;
	
	campaigns_config_.clear();
	cache_.clear_defines();
	while (dirent = SDL_ReadDir(dir)) {
		if (SDL_DIRENT_DIR(dirent->mode)) {
			std::string app = game_config::extract_app_from_app_dir(dirent->name);
			if (app.empty()) {
				continue;
			}
			const std::string bin_file = std::string(dir->directory) + "/" + dirent->name + "/bin.cfg";
			if (!file_exists(bin_file)) {
				continue;
			}
			cache_.get_config(bin_file, bin_cfg);
			VALIDATE(!bin_cfg[BINKEY_ID_CHILD].str().empty(), bin_file + " hasn't no 'id_child' key!");
			VALIDATE(!bin_cfg[BINKEY_SCENARIO_CHILD].str().empty(), bin_file + " hasn't no 'scenario_child' key!");
			VALIDATE(!bin_cfg[BINKEY_PATH].str().empty(), bin_file + " hasn't no 'path' key!");
			std::string path = working_dir_ + "/" + bin_cfg[BINKEY_PATH];

			config& sub = campaigns_config_.add_child("bin");
			cache_.get_config(path, sub);
			sub["app"] = app;
			sub[BINKEY_ID_CHILD] = bin_cfg[BINKEY_ID_CHILD].str();
			sub[BINKEY_SCENARIO_CHILD] = bin_cfg[BINKEY_SCENARIO_CHILD].str();
			sub[BINKEY_PATH] = bin_cfg[BINKEY_PATH].str();
			sub[BINKEY_MACROS] = bin_cfg[BINKEY_MACROS].str();
			cache_.clear_defines();
		}
	}
	SDL_CloseDir(dir);
}

bool editor::load_game_cfg(const editor::BIN_TYPE type, const std::string& name, const std::string& app, bool write_file, uint32_t nfiles, uint32_t sum_size, uint32_t modified)
{
	config tmpcfg;

	tres_path_lock lock(*this);
	game_config::config_cache_transaction main_transaction;

	try {
		cache_.clear_defines();

		if (type == editor::TB_DAT) {
			// VALIDATE(!editor_config::data_cfg.empty(), "Generate TB_DAT must be after data.bin!");

			std::string str = name;
			const size_t pos_ext = str.rfind(".");
			str = str.substr(0, pos_ext);
			str = str.substr(terrain_builder::tb_dat_prefix.size());

			const config& tb_cfg = tbs_config_.find_child("tb", "id", str);
			cache_.add_define(tb_cfg["define"].str());
			cache_.get_config(working_dir_ + "/data/tb.cfg", tmpcfg);

			if (write_file) {
				const config& tb_parsed_cfg = tmpcfg.find_child("tb", "id", str);
				binary_paths_manager paths_manager(tb_parsed_cfg);
				terrain_builder(tb_parsed_cfg, nfiles, sum_size, modified);
			}

		} else if (type == editor::SCENARIO_DATA) {
			std::string name_str = name;
			const size_t pos_ext = name_str.rfind(".");
			name_str = name_str.substr(0, pos_ext);

			const config& app_cfg = campaigns_config_.find_child("bin", "app", app);
			const config& campaign_cfg = app_cfg.find_child(app_cfg[BINKEY_ID_CHILD], "id", name_str);

			if (!campaign_cfg["define"].empty()) {
				cache_.add_define(campaign_cfg["define"].str());
			}
			if (!app_cfg[BINKEY_MACROS].empty()) {
				cache_.get_config(working_dir_ + "/" + app_cfg[BINKEY_MACROS], tmpcfg);
			}
			cache_.get_config(working_dir_ + "/" + app_cfg[BINKEY_PATH] + "/" + name_str, tmpcfg);

			const config& refcfg = tmpcfg.child(app_cfg[BINKEY_SCENARIO_CHILD]);
			// check scenario config valid
			BOOST_FOREACH (const config& scenario, refcfg.child_range("scenario")) {
				std::string err_str = check_scenario_cfg(scenario);
				if (!err_str.empty()) {
					throw game::error(std::string("<") + name + std::string(">") + err_str);
				}
			}

			if (write_file) {
				const std::string xwml_app_path = working_dir_ + "/xwml/" + game_config::generate_app_dir(app);
				SDL_MakeDirectory(xwml_app_path.c_str());
				wml_config_to_file(xwml_app_path + "/" + name, refcfg, nfiles, sum_size, modified);
			}

		} else if (type == editor::GUI) {
			// no pre-defined
			cache_.get_config(working_dir_ + "/data/gui", tmpcfg);
			if (write_file) {
				wml_config_to_file(working_dir_ + "/xwml/" + BASENAME_GUI, tmpcfg, nfiles, sum_size, modified);
			}

		} else if (type == editor::LANGUAGE)  {
			// no pre-defined
			cache_.get_config(working_dir_ + "/data/languages", tmpcfg);
			if (write_file) {
				wml_config_to_file(working_dir_ + "/xwml/" + BASENAME_LANGUAGE, tmpcfg, nfiles, sum_size, modified);
			}
		} else if (type == editor::EXTENDABLE)  {
			// no pre-defined
			VALIDATE(!write_file, "EXTENDABLE don't support write!");

			// app's bin
			generate_app_bin_config();

			// terrain builder rule
			const std::string tb_cfg = working_dir_ + "/data/tb.cfg";
			if (file_exists(tb_cfg)) {
				cache_.get_config(tb_cfg, tbs_config_);
			}
		} else {
			// type == editor::MAIN_DATA
			cache_.add_define("CORE");
			cache_.get_config(working_dir_ + "/data", tmpcfg);

			// check scenario config valid
			std::string err_str = check_data_bin(tmpcfg);
			if (!err_str.empty()) {
				throw game::error(std::string("<") + BASENAME_DATA + std::string(">") + err_str);
			}

			if (write_file) {
				wml_config_to_file(working_dir_ + "/xwml/" + BASENAME_DATA, tmpcfg, nfiles, sum_size, modified);
			}
			editor_config::data_cfg = tmpcfg;

			editor_config::reload_data_bin();
		} 
	}
	catch (game::error& e) {
		display* disp = display::get_singleton();
		gui2::show_error_message(disp->video(), _("Error loading game configuration files: '") + e.message + _("' (The game will now exit)"));
		return false;
	}
	return true;
}

void editor::reload_extendable_cfg()
{
	load_game_cfg(EXTENDABLE, null_str, null_str, false);
	// load_game_cfg will translate relative msgid without load textdomain.
	// result of load_game_cfg used to known what campaign, not detail information.
	// To detail information, need load textdomain, so call t_string::reset_translations(), 
	// let next translate correctly.
	t_string::reset_translations();
}

std::vector<std::string> generate_tb_short_paths(const std::string& id, const config& cfg)
{
	std::stringstream ss;
	std::vector<std::string> short_paths;

	ss.str("");
	ss << "data/core/terrain-graphics-" << id;
	short_paths.push_back(ss.str());

	binary_paths_manager paths_manager(cfg);
	const std::vector<std::string>& paths = paths_manager.paths();
	if (paths.empty()) {
		ss.str("");
		ss << "data/core/images/terrain-" << id;
		short_paths.push_back(ss.str());
	} else {
		for (std::vector<std::string>::const_iterator it = paths.begin(); it != paths.end(); ++ it) {
			ss.str("");
			ss << *it << "images/terrain-" << id;
			short_paths.push_back(ss.str());
		}
	}

	return short_paths;
}

// @path: c:\kingdom-res\data
void editor::get_wml2bin_desc_from_wml(const std::vector<editor::BIN_TYPE>& system_bin_types)
{
	tres_path_lock lock(*this);

	editor::wml2bin_desc desc;
	file_tree_checksum dir_checksum;
	std::vector<std::string> short_paths;
	std::string bin_to_path = working_dir_ + "/xwml";

	std::vector<editor::BIN_TYPE> bin_types = system_bin_types;

	// tb-[tile].dat
	std::vector<config> tbs;
	size_t tb_index = 0;
	BOOST_FOREACH (const config& cfg, tbs_config_.child_range("tb")) {
		tbs.push_back(cfg);
		bin_types.push_back(editor::TB_DAT);
	}

	// search <data>/campaigns, and form [campaign].bin
	std::vector<tapp_bin> app_bins;
	size_t app_bin_index = 0;

	BOOST_FOREACH (const config& bcfg, campaigns_config_.child_range("bin")) {
		const std::string& key = bcfg[BINKEY_ID_CHILD].str();
		BOOST_FOREACH (const config& cfg, bcfg.child_range(key)) {
			app_bins.push_back(tapp_bin(cfg["id"].str(), bcfg["app"].str(), bcfg[BINKEY_PATH].str(), bcfg[BINKEY_MACROS].str()));
			bin_types.push_back(editor::SCENARIO_DATA);
		}
	}

	wml2bin_descs_.clear();

	for (std::vector<editor::BIN_TYPE>::const_iterator itor = bin_types.begin(); itor != bin_types.end(); ++ itor) {
		editor::BIN_TYPE type = *itor;

		short_paths.clear();
		bool calculated_wml_checksum = false;

		int filter = SKIP_MEDIA_DIR;
		if (type == editor::TB_DAT) {
			const std::string& id = tbs[tb_index]["id"].str();
			short_paths = generate_tb_short_paths(id, tbs[tb_index]);
			filter = 0;

			data_tree_checksum(short_paths, dir_checksum, filter);
			desc.wml_nfiles = dir_checksum.nfiles;
			desc.wml_sum_size = dir_checksum.sum_size;
			desc.wml_modified = dir_checksum.modified;

			struct stat st;
			const std::string terrain_graphics_cfg = working_dir_ + "/data/core/terrain-graphics-" + id + ".cfg";
			if (::stat(terrain_graphics_cfg.c_str(), &st) != -1) {
				if (st.st_mtime > desc.wml_modified) {
					desc.wml_modified = st.st_mtime;
				}
				desc.wml_sum_size += st.st_size;
				desc.wml_nfiles ++;
			}
			calculated_wml_checksum = true;

			desc.bin_name = terrain_builder::tb_dat_prefix + id + ".dat";
			tb_index ++;

		} else if (type == editor::SCENARIO_DATA) {
			tapp_bin& bin = app_bins[app_bin_index ++];
			if (!bin.macros.empty()) {
				short_paths.push_back(bin.macros);
			}
			short_paths.push_back(bin.path + "/" + bin.id);
			filter |= SKIP_GUI_DIR | SKIP_INTERNAL_DIR | SKIP_BOOK;

			desc.bin_name = bin.id + ".bin";
			desc.app = bin.app;

			bin_to_path = working_dir_ + "/xwml/" + game_config::generate_app_dir(bin.app);

		} else if (type == editor::GUI) {
			short_paths.push_back("data/gui");

			desc.bin_name = BASENAME_GUI;

		} else if (type == editor::LANGUAGE) {
			short_paths.push_back("data/languages");

			desc.bin_name = BASENAME_LANGUAGE;

		} else {
			// (type == editor::MAIN_DATA)
			// no pre-defined
			short_paths.push_back("data");
			filter |= SKIP_SCENARIO_DIR | SKIP_GUI_DIR;

			desc.bin_name = BASENAME_DATA;
		}

		if (!calculated_wml_checksum) {
			data_tree_checksum(short_paths, dir_checksum, filter);
			desc.wml_nfiles = dir_checksum.nfiles;
			desc.wml_sum_size = dir_checksum.sum_size;
			desc.wml_modified = dir_checksum.modified;
		}

		if (!wml_checksum_from_file(bin_to_path + "/" + desc.bin_name, &desc.bin_nfiles, &desc.bin_sum_size, (uint32_t*)&desc.bin_modified)) {
			desc.bin_nfiles = desc.bin_sum_size = desc.bin_modified = 0;
		}

		wml2bin_descs_.push_back(std::pair<BIN_TYPE, wml2bin_desc>(type, desc));
	}

	return;
}

void reload_generate_cfg()
{
	generate_cfg.clear();
	if (check_res_folder(game_config::path)) {
		game_config::config_cache& cache = game_config::config_cache::instance();
		cache.clear_defines();

		// topen_unicode_lock lock(true);
		cache.get_config(game_config::path + "/absolute/generate.cfg", generate_cfg);
	}
}

void reload_mod_configs(display& disp, std::vector<tmod_copier>& mod_copiers, std::vector<tapp_copier>& app_copiers)
{
	if (!generate_cfg.empty()) {
		return;
	}

	reload_generate_cfg();

	BOOST_FOREACH (const config& c, generate_cfg.child_range("generate")) {
		const std::string& type = c["type"].str();
		if (type == "mod") {
			mod_copiers.push_back(tmod_copier(c));

		} else if (type.find(tapp_copier::app_prefix) == 0) {
			app_copiers.push_back(tapp_copier(c));
		}
	}
}

const config& get_generate_cfg(const config& data_cfg, const std::string& type)
{
	if (generate_cfg.empty()) {
		reload_generate_cfg();
	}

	BOOST_FOREACH (const config& c, generate_cfg.child_range("generate")) {
		if (type == c["type"].str()) {
			return c;
		}
	}
	return null_cfg;
}

const std::string tcopier::current_path_marker = ".";

tcopier::tres::tres(res_type type, const std::string& name, const std::string& allow_str)
	: type(type)
	, name(name)
	, allow()
{
	if (!allow_str.empty()) {
		std::vector<std::string> vstr = utils::split(allow_str, '-');
		for (std::vector<std::string>::const_iterator it = vstr.begin(); it != vstr.end(); ++ it) {
			allow.insert(*it);
		}
	}
}

tcopier::tcopier(const config& cfg)
	: name_(cfg["name"])
	, copy_res_()
	, remove_res_()
{
	const std::string path_prefix = "path-";
	BOOST_FOREACH (const config::attribute& attr, cfg.attribute_range()) {
		if (attr.first.find(path_prefix) != 0) {
			continue;
		}
		std::string tag = attr.first.substr(path_prefix.size());
		std::string path = attr.second;
		const char c = path.at(path.size() - 1);
		if (c == '\\' || c == '/') {
			path.erase(path.size() - 1);
		}
		paths_.insert(std::make_pair(tag, path));
	}

	set_delete_paths(cfg["delete_paths"].str());

	const config& res_cfg = cfg.child("resource");
	if (!res_cfg) {
		return;
	}
	std::map<std::string, std::vector<tres>* > v;
	v.insert(std::make_pair("copy", &copy_res_));
	v.insert(std::make_pair("remove", &remove_res_));

	for (std::map<std::string, std::vector<tres>* >::const_iterator it = v.begin(); it != v.end(); ++ it) {
		const config& op_cfg = res_cfg.child(it->first);
		if (op_cfg) {
			BOOST_FOREACH (const config::attribute& attr, op_cfg.attribute_range()) {
				std::vector<std::string> vstr = utils::split(attr.second);
				VALIDATE(vstr.size() == 2, "resource item must be 2!");
				res_type type = res_none;
				if (vstr[0] == "file") {
					type = res_file;
				} else if (vstr[0] == "dir") {
					type = res_dir;
				} else if (vstr[0] == "files") {
					type = res_files;
				}
				VALIDATE(type != res_none, "error resource type, must be file or dir!");
				std::string name = vstr[1];
				std::replace(name.begin(), name.end(), '\\', '/');

				std::string allow_str;
				size_t pos = attr.first.find('-');
				if (pos != std::string::npos) {
					allow_str = attr.first.substr(pos + 1);
				}
				it->second->push_back(tres(type, name, allow_str));
			}
		}
	}
}

const std::string& tcopier::get_path(const std::string& tag) const
{
	std::stringstream err;
	std::map<std::string, std::string>::const_iterator it = paths_.find(tag);
	err << "Invalid tag: " << tag;
	VALIDATE(it != paths_.end(), err.str());
	return it->second;
}

bool tcopier::valid() const
{
	if (paths_.empty()) {
		return false;
	}
	for (std::map<std::string, std::string>::const_iterator it = paths_.begin(); it != paths_.end(); ++ it) {
		const std::string& path = it->second;
		if (path.size() < 2 || path.at(1) != ':') {
			return false;
		}
	}
	return true;
}

static std::string type_name(int tag)
{
	if (tag == tcopier::res_file) {
		return _("File");
	} else if (tag == tcopier::res_dir) {
		return _("Directory");
	} else if (tag == tcopier::res_files) {
		return _("Files");
	}
	return null_str;
}

bool tcopier::make_path(display& disp, const std::string& tag, bool del) const
{
	utils::string_map symbols;
	const std::string& path = get_path(tag);

	size_t pos = path.rfind("/");
	if (pos == std::string::npos) {
		return true;
	}

	std::string subpath = path.substr(0, pos);
	SDL_MakeDirectory(subpath.c_str());

	if (del) {
		if (!SDL_DeleteFiles(path.c_str())) {
			symbols["type"] = _("Directory");
			symbols["dst"] = path;
			gui2::show_message(disp.video(), null_str, vgettext2("Delete $type, from $dst fail!", symbols));
			return false;
		}
	}
	return true;
}

bool tcopier::do_copy(display& disp, const std::string& src2_tag, const std::string& dst_tag) const
{
	const std::string& src_path = get_path(src2_tag);
	const std::string& dst_path = get_path(dst_tag);
	utils::string_map symbols;
	bool fok = true;
	std::string src, dst;

	// copy
	if (is_directory(src_path)) {
		for (std::vector<tres>::const_iterator it = copy_res_.begin(); it != copy_res_.end(); ++ it) {
			const tres r = *it;
			if (!r.allow.empty() && r.allow.find(dst_tag) == r.allow.end()) {
				continue;
			}
			src = src_path + "/" + r.name;
			dst = dst_path + "/" + r.name;
			if (r.type == res_file) {
				if (!file_exists(src)) {
					continue;
				}
				std::string tmp = dst.substr(0, dst.rfind('/'));
				SDL_MakeDirectory(tmp.c_str());
			} else if (r.type == res_dir || r.type == res_files) {
				if (r.name == current_path_marker) {
					src = src_path;
					dst = dst_path;
				}
				if (!is_directory(src.c_str())) {
					continue;
				}
				if (r.type == res_dir) {
					// make sure system don't exsit dst! FO_COPY requrie it.
					if (!SDL_DeleteFiles(dst.c_str())) {
						symbols["type"] = _("Directory");
						symbols["dst"] = dst;
						gui2::show_message(disp.video(), null_str, vgettext2("Delete $type, from $dst fail!", symbols));
						fok = false;
						break;
					}
				} else if (r.type == res_files) {
					SDL_MakeDirectory(dst.c_str());
				}
			}
			if (r.type == res_file || r.type == res_dir) {
				fok = SDL_CopyFiles(src.c_str(), dst.c_str());
				if (fok && r.type == res_dir) {
					fok = compare_directory(src, dst);
				}
			} else {
				fok = copy_root_files(src.c_str(), dst.c_str());
			}
			if (!fok) {
				symbols["type"] = type_name(r.type);
				symbols["src"] = src;
				symbols["dst"] = dst;
				gui2::show_message(disp.video(), null_str, vgettext2("Copy $type, from $src to $dst fail!", symbols));
				break;
			}
		}
	}
	
	return fok;
}

bool tcopier::do_remove(display& disp, const std::string& tag) const
{
	const std::string& path = get_path(tag);
	utils::string_map symbols;
	bool fok = true;
	std::string src, dst;

	// remove
	for (std::vector<tres>::const_iterator it = remove_res_.begin(); it != remove_res_.end(); ++ it) {
		const tres r = *it;
		if (!r.allow.empty() && r.allow.find(tag) == r.allow.end()) {
			continue;
		}
		dst = path + "/" + r.name;
		if (r.type == res_file) {
			if (!file_exists(dst)) {
				continue;
			}
		} else {
			if (!is_directory(dst.c_str())) {
				continue;
			}
		}
		if (!SDL_DeleteFiles(dst.c_str())) {
			symbols["type"] = r.type == res_file? _("File"): _("Directory");
			symbols["dst"] = dst;
			gui2::show_message(disp.video(), null_str, vgettext2("Delete $type, from $dst fail!", symbols));
			fok = false;
			break;
		}
	}

	return fok;
}

void tcopier::set_delete_paths(const std::string& paths)
{
	delete_paths_.clear();
	std::vector<std::string> vstr = utils::split(paths);
	for (std::vector<std::string>::const_iterator it = vstr.begin(); it != vstr.end(); ++ it) {
		const std::string& path = *it;
		if (paths_.find(path) != paths_.end()) {
			delete_paths_.push_back(get_path(path));
		} else {
			delete_paths_.push_back(path);
		}
	}
}

void tcopier::do_delete_path(bool result)
{
	if (!result) {
		display* disp = display::get_singleton();
		do_delete_path2(*disp);
	}
	// delete_paths_.clear();
}

bool tcopier::do_delete_path2(display& disp) const
{
	utils::string_map symbols;
	for (std::vector<std::string>::const_iterator it = delete_paths_.begin(); it != delete_paths_.end(); ++ it) {
		const std::string& path = *it;
		if (!is_directory(path)) {
			continue;
		}
		symbols["name"] = path;
		if (!SDL_DeleteFiles(path.c_str())) {
			gui2::show_message(disp.video(), null_str, vgettext2("Delete: $name fail!", symbols));
			return false;
		}
	}
	return true;
}

const std::string tmod_copier::res_tag = "res";
const std::string tmod_copier::patch_tag = "patch";

tmod_copier::tmod_copier(const config& cfg)
	: tcopier(cfg)
	, res_short_path()
{
	const std::string& res_path = get_path(res_tag);

	if (!res_path.empty()) {
		size_t pos = res_path.rfind('/');
		if (pos != std::string::npos) {
			res_short_path = res_path.substr(pos + 1);
		}
	}
}

bool tmod_copier::opeate_file(display& disp, bool patch_2_res)
{
	const std::string& patch_path = get_path(patch_tag);
	const std::string& src2_tag = patch_2_res? patch_tag: res_tag;
	const std::string& dst_tag = patch_2_res? res_tag: patch_tag;

	if (!patch_2_res) {
		set_delete_paths(patch_path);
	}
	tcallback_lock lock(false, boost::bind(&tcopier::do_delete_path, this, _1));

	if (!patch_2_res && !make_path(disp, patch_tag, true)) {
		return false;
	}

	// remove
	if (patch_2_res && !do_remove(disp, res_tag)) {
		return false;
	}

	// copy
	if (!do_copy(disp, src2_tag, dst_tag)) {
		return false;
	}

	lock.set_result(true);
	
	return true;
}

const std::string tapp_copier::app_prefix = "app-";
const std::string tapp_copier::res_tag = "res";
const std::string tapp_copier::src2_tag = "src2";
const std::string tapp_copier::android_prj_tag = "android_prj";
const std::string tapp_copier::app_res_tag = "app_res";
const std::string tapp_copier::app_src_tag = "app_src";
const std::string tapp_copier::app_src2_tag = "app_src2";
const std::string tapp_copier::app_android_prj_tag = "app_android_prj";
const tdomain tapp_copier::default_bundle_id("a.b.c");
const std::string tapp_copier::default_app_id = "a";

tapp_copier::tapp_copier(const config& cfg)
	: tcopier(cfg)
	, app()
	, bundle_id(cfg["bundle_id"].str())
	, ble(cfg["ble"].to_bool())
{
	VALIDATE(paths_.find(res_tag) == paths_.end(), null_str);
	VALIDATE(paths_.find(src2_tag) == paths_.end(), null_str);
	VALIDATE(paths_.find(android_prj_tag) == paths_.end(), null_str);
	VALIDATE(paths_.find(app_src_tag) == paths_.end(), null_str);
	VALIDATE(paths_.find(app_android_prj_tag) == paths_.end(), null_str);

	VALIDATE(bundle_id.valid(), null_str);
	const std::string& type = cfg["type"].str();

	std::string allow_str;
	size_t pos = type.find(app_prefix);
	VALIDATE(!pos, null_str);

	app = type.substr(pos + app_prefix.size());
	VALIDATE(!app.empty(), null_str);

	// calculate src_path base on res_path
	std::string res_path = game_config::path;
	std::string src_path = directory_name(res_path) + "apps-src";
	std::string src2_path = directory_name(res_path) + "apps-src/apps";
	std::string app_res_path;
	std::string app_src2_path;

	paths_.insert(std::make_pair(res_tag, res_path));
	paths_.insert(std::make_pair(src2_tag, src2_path));

	// app_res/app_src maybe overlay be *.cfg.
	if (paths_.find(app_res_tag) == paths_.end()) {
		app_res_path = directory_name(res_path) + app + "-res";
		paths_.insert(std::make_pair(app_res_tag, app_res_path));
	} else {
		app_res_path = paths_.find(app_res_tag)->second;
	}
	if (paths_.find(app_src2_tag) == paths_.end()) {
		app_src2_path = directory_name(res_path) + app + "-src/" + app;
		paths_.insert(std::make_pair(app_src2_tag, app_src2_path));
	} else {
		app_src2_path = paths_.find(app_src2_tag)->second;
	}

	std::string app_src_path = directory_name(app_src2_path);
	char c = app_src_path.at(app_src_path.size() - 1);
	if (c == '\\' || c == '/') {
		app_src_path.erase(app_src_path.size() - 1);
	}
	paths_.insert(std::make_pair(app_src_tag, app_src_path));

	paths_.insert(std::make_pair(android_prj_tag, src2_path + "/projectfiles/android-prj"));
	paths_.insert(std::make_pair(app_android_prj_tag, app_src2_path + "/projectfiles/android"));

	delete_paths_.push_back(app_res_path);
	delete_paths_.push_back(app_src_path);
}

struct tcopy_cookie
{
	tcopy_cookie(const std::string& dir)
		: current_path(dir)
	{
		char c = current_path.at(current_path.size() - 1);
		if (c == '\\' || c == '/') {
			current_path.erase(current_path.size() - 1);
		}
	}

	bool cb_copy_cookie(const std::string& dir, const SDL_dirent2* dirent);

	std::string current_path;
};

bool tcopy_cookie::cb_copy_cookie(const std::string& dir, const SDL_dirent2* dirent)
{
	if (SDL_DIRENT_DIR(dirent->mode)) {
		tcopy_cookie ccp2(current_path + "/" + dirent->name);
		const std::string cookie_cki = "__cookie.cki";
		{
			tfile lock(ccp2.current_path + "/" + cookie_cki, GENERIC_WRITE, CREATE_ALWAYS);
		}

		if (!walk_dir(ccp2.current_path, false, boost::bind(&tcopy_cookie::cb_copy_cookie, &ccp2, _1, _2))) {
			return false;
		}
	}
	return true;
}

bool tapp_copier::opeate_file(display& disp)
{
	tcallback_lock lock(false, boost::bind(&tcopier::do_delete_path, this, _1));

	// 1. delete require deleted paths.
	if (!do_delete_path2(disp)) {
		return false;
	}

	// 2. create path
	if (!make_path(disp, app_src2_tag, false) || !make_path(disp, app_res_tag, false)) {
		return false;
	}

	if (!do_copy(disp, src2_tag, app_src2_tag) || !do_copy(disp, res_tag, app_res_tag)) {
		return false;
	}

	if (!generate_android_prj(disp)) {
		return false;
	}

	if (!do_remove(disp, app_res_tag)) {
		return false;
	}

	// generate three app.cfg if necessary.
	generate_app_cfg(app_data);
	generate_app_cfg(app_core);
	generate_app_cfg(app_gui);

	{
		// generate __cookie.cki on every directory.
		tcopy_cookie ccp(get_path(tapp_copier::app_res_tag));
		walk_dir(ccp.current_path, false, boost::bind(&tcopy_cookie::cb_copy_cookie, &ccp, _1, _2));
	}

	lock.set_result(true);
	return true;
}

void tapp_copier::generate_app_cfg(int type) const
{
	const std::string& app_res_path = get_path(app_res_tag);
	const std::string app_short_dir = std::string("app-") + app;

	std::string base_dir, app_dir;
	std::stringstream ss;
	
	ss << "{";
	if (type == app_data) {
		base_dir = app_res_path + "\\data";
		ss << app_short_dir << "/_main.cfg";

	} else if (type == app_core) {
		base_dir = app_res_path + "\\data\\core";
		ss << "core/" << app_short_dir << "/_main.cfg";

	} else if (type == app_gui) {
		base_dir = app_res_path + "\\data\\gui";
		ss << "gui/" << app_short_dir << "/_main.cfg";
	} else {
		return;
	}
	ss << "}";

	// if _main.cfg doesn't exist, create a empty app.cfg. it is necessary for arthiture.
	std::string file = base_dir + "\\app.cfg";

	posix_file_t fp = INVALID_FILE;
	posix_fopen(file.c_str(), GENERIC_WRITE, CREATE_ALWAYS, fp);
	if (fp == INVALID_FILE) {
		return;
	}

	file = base_dir + "/" + app_short_dir + "/_main.cfg";
	if (file_exists(file)) {
		posix_fwrite(fp, ss.str().c_str(), ss.str().length());
	}
	posix_fclose(fp);

}

const char* skip_blank_characters(const char* start)
{
	while (start[0] == '\r' || start[0] == '\n' || start[0] == '\t' || start[0] == ' ') {
		start ++;
	}
	return start;
}

const char* until_blank_characters(const char* start)
{
	while (start[0] != '\r' && start[0] != '\n' && start[0] != '\t' && start[0] != ' ') {
		start ++;
	}
	return start;
}

bool tapp_copier::generate_android_prj(display& disp) const
{
	if (!do_copy(disp, android_prj_tag, app_android_prj_tag)) {
		return false;
	}

	// <app_android_prj>/app/src/main/AndroidManifest.xml
	const std::string& app_android_prj_path = get_path(app_android_prj_tag);
	{
		tfile file(app_android_prj_path + "/app/src/main/AndroidManifest.xml", GENERIC_WRITE, OPEN_EXISTING);
		int fsize = file.read_2_data();
		if (!fsize) {
			return false;
		}
		// i think length of appended data isn't more than 512 bytes.
		file.resize_data(fsize + 512);
		file.data[fsize] = '\0';
		// replace with app's bundle id.
		const char* prefix = "\"http://schemas.android.com/apk/res/android\"";
		const char* ptr = strstr(file.data, prefix);
		if (!ptr) {
			return false;
		}
		ptr = skip_blank_characters(ptr + strlen(prefix) + 1);
		if (memcmp(ptr, "package=\"", 9)) {
			return false;
		}
		ptr += 9;
		if (memcmp(ptr, default_bundle_id.id().c_str(), default_bundle_id.size())) {
			return false;
		}
		fsize = file.replace_span(ptr - file.data, default_bundle_id.size(), bundle_id.id().c_str(), bundle_id.size(), fsize);
		file.data[fsize] = '\0';

		if (ble) {
			// insert ble permission.
			const char* prefix2 = "<uses-permission android:name=\"android.permission.MODIFY_AUDIO_SETTINGS\"/>";
			ptr = strstr(ptr, prefix2);
			if (!ptr) {
				return false;
			}
			ptr = skip_blank_characters(ptr + strlen(prefix2) + 1);
			std::stringstream permission;
			permission << "<uses-permission android:name=\"android.permission.BLUETOOTH\" />\r\n";
			permission << "    <uses-permission android:name=\"android.permission.BLUETOOTH_ADMIN\" />\r\n";
			permission << "    <uses-permission android:name=\"android.permission.BLUETOOTH_PRIVILEGED\" />\r\n";
			permission << "    <uses-permission android:name=\"android.permission.ACCESS_COARSE_LOCATION\"/>\r\n";
			permission << "\r\n    ";
			fsize = file.replace_span(ptr - file.data, 0, permission.str().c_str(), permission.str().size(), fsize);
		}

		// default bundle_id is "a.b.c", any valid bundle_id must lengther than it.
		posix_fseek(file.fp, 0);
		posix_fwrite(file.fp, file.data, fsize);
	}

	// <app_android_prj>/app/src/main/java/a/b/c/app.java
	{
		tfile file(app_android_prj_path + "/app/src/main/java/a/b/c/app.java", GENERIC_WRITE, OPEN_EXISTING);
		int fsize = file.read_2_data();
		if (!fsize) {
			return false;
		}
		// i think length of appended data isn't more than 512 bytes.
		file.resize_data(fsize + 512);
		file.data[fsize] = '\0';
		// replace with app's bundle id.
		const char* prefix = "package";
		const char* ptr = strstr(file.data, prefix);
		if (!ptr) {
			return false;
		}
		ptr = skip_blank_characters(ptr + strlen(prefix) + 1);
		fsize = file.replace_span(ptr - file.data, default_bundle_id.size(), bundle_id.id().c_str(), bundle_id.size(), fsize);

		std::string app_java_dir = app_android_prj_path + "/app/src/main/java/" + bundle_id.node(0) + "/" + bundle_id.node(1) + "/" + bundle_id.node(2);
		SDL_MakeDirectory(app_java_dir.c_str());
		tfile file2(app_java_dir + "/app.java", GENERIC_WRITE, CREATE_ALWAYS);
		posix_fwrite(file2.fp, file.data, fsize);
	}
	SDL_DeleteFiles((app_android_prj_path + "/app/src/main/java/a").c_str());

	// <app_android_prj>/app/build.gradle
	{
		tfile file(app_android_prj_path + "/app/build.gradle", GENERIC_WRITE, OPEN_EXISTING);
		int fsize = file.read_2_data();
		if (!fsize) {
			return false;
		}
		// i think length of appended data isn't more than 512 bytes.
		file.resize_data(fsize + 512);
		file.data[fsize] = '\0';
		// replace with app's bundle id.
		const char* prefix = "defaultConfig {";
		const char* ptr = strstr(file.data, prefix);
		if (!ptr) {
			return false;
		}
		ptr = skip_blank_characters(ptr + strlen(prefix) + 1);
		if (memcmp(ptr, "applicationId", 13)) {
			return false;
		}
		ptr += 13;
		ptr = skip_blank_characters(ptr);
		if (ptr[0] != '\"') {
			return false;
		}
		ptr ++;
		if (memcmp(ptr, default_bundle_id.id().c_str(), default_bundle_id.size())) {
			return false;
		}
		fsize = file.replace_span(ptr - file.data, default_bundle_id.size(), bundle_id.id().c_str(), bundle_id.size(), fsize);
		file.data[fsize] = '\0';

		if (ble) {
			// ble is at least level-18.
			const char* prefix2 = "minSdkVersion";
			ptr = strstr(ptr, prefix2);
			if (!ptr) {
				return false;
			}
			ptr = skip_blank_characters(ptr + strlen(prefix2) + 1);
			const char* ptr2 = until_blank_characters(ptr);
			const int min_ble_level = 18;
			std::string str = str_cast(min_ble_level);
			fsize = file.replace_span(ptr - file.data, ptr2 - ptr, str.c_str(), str.size(), fsize);
		}

		// default bundle_id is "a.b.c", any valid bundle_id must lengther than it.
		posix_fseek(file.fp, 0);
		posix_fwrite(file.fp, file.data, fsize);
	}

	// <android-prj>/app/src/main/res/values/strings.xml
	{
		tfile file(app_android_prj_path + "/app/src/main/res/values/strings.xml", GENERIC_WRITE, OPEN_EXISTING);
		int fsize = file.read_2_data();
		if (!fsize) {
			return false;
		}
		// i think length of appended data isn't more than 512 bytes.
		file.resize_data(fsize + 512);
		file.data[fsize] = '\0';
		// replace with app's bundle id.
		const char* prefix = "<string name=\"app_name\">";
		const char* ptr = strstr(file.data, prefix);
		if (!ptr) {
			return false;
		}
		ptr = skip_blank_characters(ptr + strlen(prefix));
		fsize = file.replace_span(ptr - file.data, default_app_id.size(), app.c_str(), app.size(), fsize);

		// default app id is "a", any valid app_id must lengther than it.
		posix_fseek(file.fp, 0);
		posix_fwrite(file.fp, file.data, fsize);
	}

	// <android_prj>/app/jni/Android.mk
	{
		tfile file(app_android_prj_path + "/app/jni/Android.mk", GENERIC_WRITE, OPEN_EXISTING);
		int fsize = file.read_2_data();
		if (!fsize) {
			return false;
		}
		// i think length of appended data isn't more than 512 bytes.
		file.resize_data(fsize + 512);
		file.data[fsize] = '\0';
		// replace with app's bundle id.
		const char* prefix = "$(LOCAL_PATH)/librose";
		const char* ptr = strstr(file.data, prefix);
		if (!ptr) {
			return false;
		}
		ptr = skip_blank_characters(ptr + strlen(prefix) + 1);
		if (memcmp(ptr, "\\", 1)) {
			return false;
		}
		ptr += 1;
		std::stringstream includes;
		includes << "\n\t";
		includes << "$(LOCAL_PATH)/" << app;
		fsize = file.replace_span(ptr - file.data, 0, includes.str().c_str(), includes.str().size(), fsize);

		// insert LOCAL_SRC_FILES
		const char* prefix2 = "$(subst $(LOCAL_PATH)/,,";
		ptr = strstr(ptr, prefix2);
		if (!ptr) {
			return false;
		}
		ptr = skip_blank_characters(ptr + strlen(prefix2) + 1);
		if (memcmp(ptr, "\\", 1)) {
			return false;
		}
		ptr += 1;
		std::stringstream src_files;
		src_files << "\n";
		src_files << "\t$(wildcard $(LOCAL_PATH)/" << app <<"/*.c) \\\n";
		src_files << "\t$(wildcard $(LOCAL_PATH)/" << app <<"/*.cpp) \\";
		if (is_directory(get_path(app_src2_tag) + "/" + app + "/gui/dialogs")) {
			src_files << "\n";
			src_files << "\t$(wildcard $(LOCAL_PATH)/" << app <<"/gui/dialogs/*.c) \\\n";
			src_files << "\t$(wildcard $(LOCAL_PATH)/" << app <<"/gui/dialogs/*.cpp) \\";
		}
		fsize = file.replace_span(ptr - file.data, 0, src_files.str().c_str(), src_files.str().size(), fsize);

		// default bundle_id is "a.b.c", any valid bundle_id must lengther than it.
		posix_fseek(file.fp, 0);
		posix_fwrite(file.fp, file.data, fsize);
	}

	std::string app_src_path = get_path(app_src_tag);
	std::replace(app_src_path.begin(), app_src_path.end(), path_sep(false), path_sep(true));
	// <app_src>/scripts/android_set_variable.tpl
	{
		tfile file(app_src_path + "/scripts/android_set_variable.tpl", GENERIC_READ, OPEN_EXISTING);
		int fsize = file.read_2_data();
		if (!fsize) {
			return false;
		}
		// i think length of appended data isn't more than 512 bytes.
		file.resize_data(fsize + 512);
		file.data[fsize] = '\0';
		// replace with app's bundle id.
		const char* prefix = "_APP_SRC";
		const char* ptr = strstr(file.data, prefix);
		if (!ptr) {
			return false;
		}
		ptr = skip_blank_characters(ptr + strlen(prefix) + 1);
		ptr = strstr(ptr, "=");
		if (!ptr) {
			return false;
		}
		ptr = skip_blank_characters(ptr + 1);
		const char* ptr2 = until_blank_characters(ptr);
		fsize = file.replace_span(ptr - file.data, ptr2 - ptr, app_src_path.c_str(), app_src_path.size(), fsize);
		file.data[fsize] = '\0';

		// bat don't support set variable use another variable.
		const char* prefix2 = "%_APP_SRC%";
		while (ptr) {
			ptr = strstr(ptr, prefix2);
			if (!ptr) {
				break;
			}
			fsize = file.replace_span(ptr - file.data, strlen(prefix2), app_src_path.c_str(), app_src_path.size(), fsize);
			ptr += app_src_path.size();
			file.data[fsize] = '\0';
		}

		// app directory
		std::stringstream app_dir;
		app_dir << "\n\nset " << app << "=" << app_src_path << "/apps/projectfiles/android/app";
		
		std::string app_dir_str = app_dir.str();
		std::replace(app_dir_str.begin(), app_dir_str.end(), path_sep(false), path_sep(true));
		fsize = file.replace_span(fsize, 0, app_dir_str.c_str(), app_dir_str.size(), fsize);

		tfile file2(app_src_path + "/scripts/android_set_variable.bat", GENERIC_WRITE, CREATE_ALWAYS);
		posix_fwrite(file2.fp, file.data, fsize);
	}
	SDL_DeleteFiles((app_src_path + "/scripts/android_set_variable.tpl").c_str());

	return true;
}

bool tapp_copier::copy_res_2_android_prj() const
{
	// <app_android_prj>/app/src/main/assets/res
	const std::string& app_res_path = get_path(app_res_tag);
	const std::string android_res_path = get_path(app_android_prj_tag) + "/app/src/main/assets/res";
	SDL_MakeDirectory(android_res_path.c_str());

	bool ret = true;
	std::stringstream ss;
	SDL_DIR* dir = SDL_OpenDir(app_res_path.c_str());
	if (!dir) {
		return false;
	}
	SDL_dirent2* dirent;
	
	while (dirent = SDL_ReadDir(dir)) {
		if (SDL_DIRENT_DIR(dirent->mode)) {
			if (dirent->name[0] != '.') {
				if (!SDL_strcmp(dirent->name, "po")) {
					continue;
				}
				std::stringstream src;
				src << app_res_path << "/" << dirent->name;

				SDL_CopyFiles(src.str().c_str(), android_res_path.c_str());
			}
		}
	}
	SDL_CloseDir(dir);

	return true;
}

bool tapp_copier::valid() const
{
	if (!tcopier::valid()) {
		return false;
	}

	std::map<std::string, std::string>::const_iterator it = paths_.find(app_res_tag);
	if (it == paths_.end()) {
		return false;
	}
	it = paths_.find(app_src2_tag);
	if (it == paths_.end()) {
		return false;
	}
	return true;
}