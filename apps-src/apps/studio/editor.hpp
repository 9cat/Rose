#ifndef STUDIO_EDITOR_HPP_INCLUDED
#define STUDIO_EDITOR_HPP_INCLUDED

#include "config_cache.hpp"
#include "config.hpp"
#include "version.hpp"
#include <set>

#include <boost/bind.hpp>
#include <boost/function.hpp>

class display;

bool is_apps_res(const std::string& folder);
bool check_res_folder(const std::string& folder);

enum {BIN_WML, BIN_BUILDINGRULE};
#define BINKEY_ID_CHILD			"id_child"
#define BINKEY_SCENARIO_CHILD	"scenario_child"
#define BINKEY_PATH				"path"
#define BINKEY_MACROS			"macros"

namespace editor_config
{
	extern config data_cfg;
	extern std::string campaign_id;
	extern int type;

	void reload_data_bin();
}

class editor
{
public:
	class tres_path_lock
	{
	public:
		tres_path_lock(editor& o);
		~tres_path_lock();

	private:
		static int deep;
		std::string original_;
	};

	enum BIN_TYPE {BIN_MIN = 0, MAIN_DATA = BIN_MIN, GUI, LANGUAGE, BIN_SYSTEM_MAX = LANGUAGE, TB_DAT, SCENARIO_DATA, EXTENDABLE};
	struct wml2bin_desc {
		wml2bin_desc();
		std::string bin_name;
		std::string app;
		uint32_t wml_nfiles;
		uint32_t wml_sum_size;
		time_t wml_modified;
		uint32_t bin_nfiles;
		uint32_t bin_sum_size;
		time_t bin_modified;
		bool require_build;

		bool valid() const { return !bin_name.empty(); }
		void refresh_checksum(const std::string& working_dir);
	};
	struct tapp_bin {
		tapp_bin(const std::string& id, const std::string& app, const std::string& path, const std::string& macros)
			: id(id)
			, app(app)
			, path(path)
			, macros(macros)
		{}
		std::string id;
		std::string app;
		std::string path;
		std::string macros;
	};

	editor(const std::string& working_dir);

	void set_working_dir(const std::string& dir);
	const std::string& working_dir() { return working_dir_; }

	bool make_system_bins_exist();

	bool load_game_cfg(const BIN_TYPE type, const std::string& name, const std::string& app, bool write_file, uint32_t nfiles = 0, uint32_t sum_size = 0, uint32_t modified = 0);
	void get_wml2bin_desc_from_wml(const std::vector<editor::BIN_TYPE>& system_bin_types);
	void reload_extendable_cfg();
	std::string check_scenario_cfg(const config& scenario_cfg);
	std::string check_mplayer_bin(const config& mplayer_cfg);
	std::string check_data_bin(const config& data_cfg);

	std::vector<std::pair<BIN_TYPE, wml2bin_desc> >& wml2bin_descs() { return wml2bin_descs_; }
	const std::vector<std::pair<BIN_TYPE, wml2bin_desc> >& wml2bin_descs() const { return wml2bin_descs_; }

	const config& campaigns_config() const { return campaigns_config_; }

private:
	void generate_app_bin_config();

private:
	std::string working_dir_;
	config campaigns_config_;
	config tbs_config_;
	game_config::config_cache& cache_;
	std::vector<std::pair<BIN_TYPE, wml2bin_desc> > wml2bin_descs_;
};

class tcallback_lock
{
public:
	tcallback_lock(bool result, boost::function<void (bool)> callback)
		: result_(result)
		, callback_(callback)
	{}

	~tcallback_lock()
	{
		if (callback_) {
			callback_(result_);
		}
	}
	void set_result(bool val) { result_ = val; }

private:
	bool result_;
	boost::function<void (bool)> callback_;
};

class tcopier
{
public:
	static const std::string current_path_marker;
	enum res_type {res_none, res_file, res_dir, res_files};
	struct tres {
		tres(res_type type, const std::string& name, const std::string& allow_str);

		res_type type;
		std::string name;
		std::set<std::string> allow;
	};

	tcopier(const config& cfg);

	const std::string& name() const { return name_; }
	virtual bool valid() const;
	bool make_path(display& disp, const std::string& tag, bool del) const;
	bool do_copy(display& disp, const std::string& src2_tag, const std::string& dst_tag) const;
	bool do_remove(display& disp, const std::string& tag) const;
	const std::string& get_path(const std::string& tag) const;

	void set_delete_paths(const std::string& paths);
	void do_delete_path(bool result);

protected:
	bool do_delete_path2(display& disp) const;

protected:
	std::string name_;
	std::map<std::string, std::string> paths_;
	std::vector<tres> copy_res_;
	std::vector<tres> remove_res_;
	std::vector<std::string> delete_paths_;
};

class tmod_copier: public tcopier
{
public:
	static const std::string res_tag;
	static const std::string patch_tag;

	tmod_copier(const config& cfg);
	bool opeate_file(display& disp, bool patch_2_res);

public:
	std::string res_short_path;
};

class tapp_copier: public tcopier
{
public:
	enum {app_data, app_core, app_gui};

	static const std::string app_prefix;
	static const std::string res_tag;
	static const std::string src2_tag;
	static const std::string android_prj_tag;
	static const std::string app_res_tag;
	static const std::string app_src_tag;
	static const std::string app_src2_tag;
	static const std::string app_android_prj_tag;
	static const tdomain default_bundle_id;
	static const std::string default_app_id;

	tapp_copier(const config& cfg);
	bool opeate_file(display& disp);
	bool valid() const;

	bool copy_res_2_android_prj() const;

private:
	void generate_app_cfg(int type) const;
	bool generate_android_prj(display& disp) const;

public:
	std::string app;
	tdomain bundle_id;
	bool ble;
};

void reload_generate_cfg();
void reload_mod_configs(display& disp, std::vector<tmod_copier>& mod_copiers, std::vector<tapp_copier>& app_copiers);
const config& get_generate_cfg(const config& data_cfg, const std::string& type);

#endif