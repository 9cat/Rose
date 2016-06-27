#define GETTEXT_DOMAIN "rose-lib"
#include "global.hpp"
#include <map>
#include <string>
#include <vector>

#include "config.hpp"
#include "filesystem.hpp"
#include "tstring.hpp"
#include "rose_config.hpp"

// terrain_builder
#include "builder.hpp"
#include "image.hpp"

#include "map.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include "posix.h"

#define WMLBIN_MARK_CONFIG		"[cfg]"
#define WMLBIN_MARK_CONFIG_LEN	5
#define WMLBIN_MARK_VALUE		"[val]"
#define WMLBIN_MARK_VALUE_LEN	5

// find index of textdomain. it doesn't exist in current tds, insert it.
static uint32_t tstring_textdomain_idx(const char *textdomain, std::vector<std::string>& tds, std::vector<std::set<std::string> >& msgids) 
{
	if (!textdomain || textdomain[0] == '\0') {
		return 0;
	}
	std::vector<std::string>::const_iterator it = find(tds.begin(), tds.end(), textdomain);
	if (it != tds.end()) {
		return it - tds.begin() + 1;
	} else {
		tds.push_back(textdomain);
		msgids.resize(tds.size());
		return tds.size();
	}
}

// @deep: nesting deep. top level: 0
static uint32_t wml_config_to_fp(posix_file_t fp, const config &cfg, uint32_t *max_str_len, std::vector<std::string>& td, uint16_t deep, std::vector<std::set<std::string> >& msgids)
{
	uint32_t u32n, bytes = 0;
	int first;
		
	// config::child_list::const_iterator	ichildlist;
	// string_map::const_iterator			istrmap;

	// recursively resolve children
	BOOST_FOREACH (const config::any_child &value, cfg.all_children_range()) {
		// save {[cfg]}{len}{name}
		posix_fwrite(fp, WMLBIN_MARK_CONFIG, WMLBIN_MARK_CONFIG_LEN);
		u32n = posix_mku32(value.key.size(), deep);
		posix_fwrite(fp, &u32n, sizeof(u32n));
		posix_fwrite(fp, value.key.c_str(), posix_lo16(u32n));

		bytes += WMLBIN_MARK_CONFIG_LEN + sizeof(u32n) + posix_lo16(u32n);

		*max_str_len = posix_max(*max_str_len, value.key.size());

		// save {[val]}{len}{name0}{len}{val0}{len}{name1}{len}{val1}{...}
		// string_map	&values = value.cfg.gvalues();
		// for (istrmap = values.begin(); istrmap != values.end(); istrmap ++) {
		first = 1;
		BOOST_FOREACH (const config::attribute &istrmap, value.cfg.attribute_range()) {
			if (first) {
				posix_fwrite(fp, WMLBIN_MARK_VALUE, WMLBIN_MARK_VALUE_LEN);

				bytes += WMLBIN_MARK_VALUE_LEN;

				first = 0;
			}
			u32n = istrmap.first.size();
			posix_fwrite(fp, &u32n, sizeof(u32n));
			posix_fwrite(fp, istrmap.first.c_str(), u32n);
			*max_str_len = posix_max(*max_str_len, u32n);

			bytes += sizeof(u32n) + u32n;

			if (istrmap.second.t_str().translatable()) {
				// parse translatable string
				std::vector<t_string_base::trans_str> trans = istrmap.second.t_str().valuex();
				for (std::vector<t_string_base::trans_str>::const_iterator ti = trans.begin(); ti != trans.end(); ti ++) {
					int td_index = 0;
					if (ti == trans.begin()) {
						if (ti->td.empty()) {
							u32n = posix_mku32(0, posix_mku16(0, trans.size()));
						} else {
							td_index = tstring_textdomain_idx(ti->td.c_str(), td, msgids);
							u32n = posix_mku32(0, posix_mku16(td_index, trans.size()));
						}
					} else {
						if (ti->td.empty()) {
							u32n = posix_mku32(0, 0);
						} else {
							td_index = tstring_textdomain_idx(ti->td.c_str(), td, msgids);
							u32n = posix_mku32(0, posix_mku16(td_index, 0));
						}
					}
					// flag
					posix_fwrite(fp, &u32n, sizeof(u32n));
					// length of value
					u32n = ti->str.size();
					posix_fwrite(fp, &u32n, sizeof(u32n));
					posix_fwrite(fp, ti->str.c_str(), u32n);

					if (td_index) {
						std::set<std::string>& item = msgids[td_index - 1];
						item.insert(ti->str);
					}

					bytes += sizeof(u32n) + sizeof(u32n) + u32n;
				}
			} else {
				// flag
				u32n = 0;
				posix_fwrite(fp, &u32n, sizeof(u32n));
				// length of value
				u32n = istrmap.second.str().size();
				posix_fwrite(fp, &u32n, sizeof(u32n));
				posix_fwrite(fp, istrmap.second.str().c_str(), u32n);

				bytes += sizeof(u32n) + sizeof(u32n) + u32n;
			}
			*max_str_len = posix_max(*max_str_len, u32n);

		}		
		bytes += wml_config_to_fp(fp, value.cfg, max_str_len, td, deep + 1, msgids);
	}

	return bytes;
}

bool is_all_asci(const char* c_str, int len)
{
	for (int i = 0; i < len; i ++) {
		if (c_str[i] & 0x80) {
			return false;
		}
	}
	return true;
}

static void generate_cfg_cpp(const std::string& fname, const std::vector<std::string>& tdomain, const std::vector<std::set<std::string> >& msgids, uint32_t max_str_len)
{
	// if destination file is at <res>/xwml, write cfg-cpp.
	const std::string xwml_path = game_config::path + "/xwml";
	if (fname.find(xwml_path) != 0) {
		return;
	}
	const std::string xwml_sub_dir = directory_name(fname.substr(xwml_path.size() + 1));
	const std::string main_name = file_main_name(file_name(fname));

	// write this to path/po/cfg-cpp/<textdomain>/<xwml_sub_dir>/<bin>.cpp
	const int increase_size = max_str_len < 8192? 8192: posix_align_ceil(max_str_len, 1024);
	int at = 0, vsize = 0, msgid_size;
	std::stringstream ss;
	for (std::vector<std::set<std::string> >::const_iterator it = msgids.begin(); it != msgids.end(); ++ it, at ++) {
		const std::set<std::string>& item = *it;
		if (item.empty()) {
			continue;
		}
		ss.str("");
		ss << game_config::path << "/po/cfg-cpp/" << tdomain[at] << "/";
		const std::string dir_name = directory_name(fname);
		if (!xwml_sub_dir.empty()) {
			ss << xwml_sub_dir;
		}
		create_directory_if_missing(ss.str());
		ss << main_name << ".cpp";

		tfile lock(ss.str(), GENERIC_WRITE, CREATE_ALWAYS);
		if (!lock.valid()) {
			continue;
		}
		lock.resize_data(increase_size);
		vsize = 0;
		for (std::set<std::string>::const_iterator it2 = item.begin(); it2 != item.end(); ++ it2) {
			std::string msgid = *it2;
			if (msgid.empty()) {
				continue;
			}
			msgid_size = msgid.size();
			if (!is_all_asci(msgid.c_str(), msgid_size)) {
				continue;
			}

			size_t pos = msgid.find("\n");
			if (pos != std::string::npos && (int)pos < msgid_size - 1) {
				boost::algorithm::replace_all(msgid, "\n", std::string("\\n\"\n\""));
			}
			if (vsize) {
				memcpy(lock.data + vsize, "\n\n", 2);
				vsize += 2;
			}
			memcpy(lock.data + vsize, "_(\"", 3);
			vsize += 3;
			if (vsize + msgid_size + 16 >= lock.data_size) {
				lock.resize_data(lock.data_size + increase_size, vsize);
			}
			memcpy(lock.data + vsize, msgid.c_str(), msgid_size);
			vsize += msgid_size;
			memcpy(lock.data + vsize, "\");", 3);
			vsize += 3;
		}
		posix_fwrite(lock.fp, lock.data, vsize);
	}
	return;
}

void wml_config_to_file(const std::string& fname, const config &cfg, uint32_t nfiles, uint32_t sum_size, uint32_t modified)
{
	uint32_t							max_str_len, u32n; 

	std::vector<std::string>			tdomain;
	
	tfile lock(fname, GENERIC_WRITE, CREATE_ALWAYS);
	if (!lock.valid()) {
		posix_print("------<xwml.cpp>::wml_config_to_file, cannot create %s for write\n", fname.c_str());
		return;
	}

	max_str_len = posix_max(WMLBIN_MARK_CONFIG_LEN, WMLBIN_MARK_VALUE_LEN);
	uint32_t header_len = 16 + sizeof(max_str_len) + sizeof(u32n);
	posix_fseek(lock.fp, header_len);

	std::vector<std::set<std::string> > msgids;
	uint32_t data_len = wml_config_to_fp(lock.fp, cfg, &max_str_len, tdomain, 0, msgids);

	// update max_str_len/data_len
	posix_fseek(lock.fp, 0);

	// 0--15
	u32n = mmioFOURCC('X', 'W', 'M', 'L');
	posix_fwrite(lock.fp, &u32n, 4);
	posix_fwrite(lock.fp, &nfiles, 4);
	posix_fwrite(lock.fp, &sum_size, 4);
	posix_fwrite(lock.fp, &modified, 4);
	// 16--19(max_str_len)
	posix_fwrite(lock.fp, &max_str_len, sizeof(max_str_len));
	// 20--23(data_len)
	posix_fwrite(lock.fp, &data_len, sizeof(u32n));

	// write [textdomain]
	posix_fseek(lock.fp, header_len + data_len);

	// write [textdomain]
	u32n = tdomain.size();
	posix_fwrite(lock.fp, &u32n, sizeof(u32n));

	for (std::vector<std::string>::const_iterator it = tdomain.begin(); it != tdomain.end(); ++ it) {
		const std::string& str = *it;
		u32n = str.size();
		posix_fwrite(lock.fp, &u32n, sizeof(u32n));
		posix_fwrite(lock.fp, str.c_str(), u32n);
	}

	generate_cfg_cpp(fname, tdomain, msgids, max_str_len);
}


bool wml_config_from_data(uint8_t *data, uint32_t datalen, uint8_t *namebuf, uint8_t *valbuf, std::vector<std::string> &tdomain, config &cfg)
{
	int									retval;
	uint8_t								*rdpos = data;
	uint32_t							u32n, len, transcnt, tdidx;
	uint16_t							deep;

	config::child_list					lastcfg;							

	retval = -1;

	lastcfg.push_back(&cfg);

	while (rdpos < data + datalen) {

		// posix_print("in while, rdpos: %p, pos: %u(0x%x)", rdpos, rdpos - data + 4, rdpos - data + 4);
		// read {[cfg]}{len}{name}
		if (memcmp(rdpos, WMLBIN_MARK_CONFIG, WMLBIN_MARK_CONFIG_LEN)) {
			// invalid format.
			return false;
		}
		rdpos = rdpos + WMLBIN_MARK_CONFIG_LEN;
		memcpy(&u32n, rdpos, sizeof(u32n));
		len = posix_lo16(u32n);
		deep = posix_hi16(u32n);
		rdpos = rdpos + sizeof(u32n);

		memcpy(namebuf, rdpos, len);
		namebuf[len] = 0;
		rdpos = rdpos + len;

		// posix_print("deep: %u, name: %s\n", deep, namebuf);

		config &cfgtmp = lastcfg[deep]->add_child(std::string((char *)namebuf));
		if (deep + 1 >= (uint16_t)lastcfg.size()) {
			lastcfg.push_back(&cfgtmp);
		} else {
            lastcfg[deep + 1] = &cfgtmp;
		}

		// read {[val]}{len}{name0}{len}{val0}{len}{name1}{len}{val1}{...}
		if (!memcmp(rdpos, WMLBIN_MARK_VALUE, WMLBIN_MARK_VALUE_LEN)) {
			// ����value
			rdpos = rdpos + WMLBIN_MARK_VALUE_LEN;

			while ((rdpos < data + datalen) && memcmp(rdpos, WMLBIN_MARK_CONFIG, WMLBIN_MARK_CONFIG_LEN)) {
				// name
				memcpy(&len, rdpos, sizeof(len));
				rdpos = rdpos + sizeof(len);

				memcpy(namebuf, rdpos, len);
				namebuf[len] = 0;
				rdpos = rdpos + len;

				// value
				memcpy(&u32n, rdpos, sizeof(u32n));
				rdpos = rdpos + sizeof(u32n);
				
				transcnt = posix_hi8(posix_hi16(u32n));
				tdidx = posix_lo8(posix_hi16(u32n));

				memcpy(&len, rdpos, sizeof(len));
				rdpos = rdpos + sizeof(len);
				memcpy(valbuf, rdpos, len);
				valbuf[len] = 0;
				rdpos = rdpos + len;

				if (transcnt) {
					if (tdidx) {
						cfgtmp[std::string((char *)namebuf)] = t_string((const char *)valbuf, tdomain[tdidx - 1]);
					} else {
						cfgtmp[std::string((char *)namebuf)] = t_string((const char *)valbuf);
					}
					transcnt --;
					while (transcnt != 0) {
						// value
						memcpy(&u32n, rdpos, sizeof(u32n));
						rdpos = rdpos + sizeof(u32n);

						tdidx = posix_lo8(posix_hi16(u32n));

						memcpy(&len, rdpos, sizeof(len));
						rdpos = rdpos + sizeof(len);
						memcpy(valbuf, rdpos, len);
						valbuf[len] = 0;
						rdpos = rdpos + len;

						if (tdidx) {
							cfgtmp[std::string((char *)namebuf)] = cfgtmp[std::string((char *)namebuf)].t_str() + t_string((const char *)valbuf, tdomain[tdidx - 1]);
						} else {
							cfgtmp[std::string((char *)namebuf)] = cfgtmp[std::string((char *)namebuf)].t_str() + t_string((const char *)valbuf);
						}
						transcnt --;
					}
					
				} else {
					cfgtmp[std::string((char *)namebuf)] = t_string((const char *)valbuf);
				}
			}
		}
	}

	return true;
}

#define MIN_XMIN_BIN_SIZE		28	// 16 + 4 + 4 +....+4... last +4 is size of textdomain.

void wml_config_from_file(const std::string &fname, config &cfg, uint32_t* nfiles, uint32_t* sum_size, uint32_t* modified)
{
	int64_t fsize;
	uint32_t							max_str_len, data_len, tdcnt, idx, len;
	uint8_t								*namebuf = NULL, *valbuf = NULL;
	char								tdname[MAXLEN_TEXTDOMAIN + 1];

	std::vector<std::string>			tdomain;

	posix_print("<xwml.cpp>::wml_config_from_file------fname: %s\n", fname.c_str());

	cfg.clear();	// first clear. below action is add.

	tfile lock(fname, GENERIC_READ, OPEN_EXISTING);
	if (!lock.valid()) {
		posix_print("------<xwml.cpp>::wml_config_from_file, cannot create %s for read\n", fname.c_str());
		return;
	}
	fsize = posix_fsize(lock.fp);
	if (fsize <= MIN_XMIN_BIN_SIZE) {
		return;
	}
	posix_fseek(lock.fp, 0);
	posix_fread(lock.fp, &len, 4);
	if (len != mmioFOURCC('X', 'W', 'M', 'L')) {
		return;
	}
	posix_fread(lock.fp, &len, 4);
	if (nfiles) {
		*nfiles = len;
	}
	posix_fread(lock.fp, &len, 4);
	if (sum_size) {
		*sum_size = len;
	}
	posix_fread(lock.fp, &len, 4);
	if (modified) {
		*modified = len;
	}
	posix_fread(lock.fp, &max_str_len, sizeof(max_str_len));
	
	// read data_len
	posix_fread(lock.fp, &data_len, sizeof(data_len));

	uint32_t header_len = 16 + sizeof(max_str_len) + sizeof(data_len);
	posix_fseek(lock.fp, header_len + data_len);

	// read textdomain
	posix_fread(lock.fp, &tdcnt, sizeof(tdcnt));
	for (idx = 0; idx < tdcnt; idx ++) {
		posix_fread(lock.fp, &len, sizeof(uint32_t));
		posix_fread(lock.fp, tdname, len);
		tdname[len] = 0;
		tdomain.push_back(tdname);

		t_string::add_textdomain(tdomain.back(), get_intl_dir());
	}
	
	lock.resize_data(data_len);

	namebuf = (uint8_t *)malloc(max_str_len + 1 + 1024);
	valbuf = (uint8_t *)malloc(max_str_len + 1 + 1024);

	// read data to memory
	posix_fseek(lock.fp, header_len);
	posix_fread(lock.fp, lock.data, data_len);

	wml_config_from_data((uint8_t*)lock.data, data_len, namebuf, valbuf, tdomain, cfg);

	if (namebuf) {
		free(namebuf);
	}
	if (valbuf) {
		free(valbuf);
	}
}

bool wml_checksum_from_file(const std::string &fname, uint32_t* nfiles, uint32_t* sum_size, uint32_t* modified)
{
	int64_t fsize;
	uint32_t tmp;

	tfile lock(fname, GENERIC_READ, OPEN_EXISTING);
	if (!lock.valid()) {
		return false;
	}
	fsize = posix_fsize(lock.fp);
	if (fsize <= 16) {
		return false;
	}
	posix_fseek(lock.fp, 0);
	posix_fread(lock.fp, &tmp, 4);
	if (tmp != mmioFOURCC('X', 'W', 'M', 'L')) {
		return false;
	}
	posix_fread(lock.fp, &tmp, 4);
	if (nfiles) {
		*nfiles = tmp;
	}
	posix_fread(lock.fp, &tmp, 4);
	if (sum_size) {
		*sum_size = tmp;
	}
	posix_fread(lock.fp, &tmp, 4);
	if (modified) {
		*modified = tmp;
	}

	return true;
}

unsigned char calcuate_xor_from_file(const std::string &fname)
{
	int64_t fsize, pos;
	unsigned char ret = 0;

	tfile lock(fname, GENERIC_READ, OPEN_EXISTING);
	if (!lock.valid()) {
		return 0;
	}
	fsize = posix_fsize(lock.fp);
	if (!fsize) {
		return 0;
	}
	posix_fseek(lock.fp, 0);
	lock.resize_data(fsize);
	
	posix_fread(lock.fp, lock.data, fsize);
	pos = 0;
	while (pos < fsize) {
		ret ^= lock.data[pos ++];
	}

	return ret;
}

/*
��ִ��parse_config����ִ��wml_building_rules_from_file
425 ---> 12116: parse_config��������12��
12116 ---> 29059: �Ĺ�֮�󽫽�����17��
===>�����������wml_building_rules_from_fileЧ�ʷǵ�û��߷����ǽ���

��ִ��wml_building_rules_from_file����ִ��parse_config��
wml_building_rules_from_fileֻ�ǽ�������4��
===>�����������wml_building_rules_from_fileЧ���������

1�����Թ�Ӧ�ò���wml_building_rules_from_file��building_rules_.clear���⡣��ʹ����һ���յ�rules��wml_building_rules_from_fileЧ�ʲ�û�ı䡣
*/

// @size: ��ֵ�Ѿ���Ч
#define t_list_to_fp(fp, list, idx, size)	do {	\
	for (idx = 0; idx < size; idx ++) {	\
		posix_fwrite(fp, &(list)[idx].base, sizeof(t_translation::t_layer));	\
		posix_fwrite(fp, &(list)[idx].overlay, sizeof(t_translation::t_layer));	\
	}	\
} while (0)

//   2.1.������ַ��������ܵ�һ��,����hi8(hi16(u32))ֵ���Ӵ���Ŀ,������0
//   2.2.lo16(u32)���Ӵ��ַ�����
#define vstr_to_fp(fp, vstr, idx, size, u32n, max_str_len) do {	\
	size = (vstr).size();	\
	posix_fwrite(fp, &size, sizeof(size));	\
	for (idx = 0; idx < size; idx ++) {	\
		u32n = (vstr)[idx].size();	\
		posix_fwrite(fp, &u32n, sizeof(u32n));	\
		posix_fwrite(fp, (vstr)[idx].c_str(), u32n);	\
		max_str_len = posix_max(max_str_len, u32n);	\
	}	\
} while (0)

void wml_building_rules_to_file(const std::string& fname, terrain_builder::building_rule* rules, uint32_t rules_size, uint32_t nfiles, uint32_t sum_size, uint32_t modified)
{
	posix_file_t						fp = INVALID_FILE;
	uint32_t							max_str_len, u32n, idx, size, size1; 

	posix_print("<xwml.cpp>::wml_building_rules_to_file------fname: %s, will save %u rules\n", fname.c_str(), rules_size);

	posix_fopen(fname.c_str(), GENERIC_WRITE, CREATE_ALWAYS, fp);
	if (fp == INVALID_FILE) {
		posix_print("------<xwml.cpp>::wml_building_rules_to_file, cannot create %s for wrtie\n", fname.c_str());
		return;
	}

	// max str len
	max_str_len = 63;
	// 0--15
	u32n = mmioFOURCC('X', 'W', 'M', 'L');
	posix_fwrite(fp, &u32n, 4);
	posix_fwrite(fp, &nfiles, 4);
	posix_fwrite(fp, &sum_size, 4);
	posix_fwrite(fp, &modified, 4);

	posix_fseek(fp, 16 + sizeof(max_str_len));
	// rules size
	posix_fwrite(fp, &rules_size, sizeof(rules_size));

	uint32_t rule_index = 0;

	for (rule_index = 0; rule_index < rules_size; rule_index ++) {

		//
		// typedef std::multiset<building_rule> building_ruleset;
		//

		// building_rule
		const terrain_builder::building_rule& rule = rules[rule_index];

		// *int precedence
		posix_fwrite(fp, &rule.precedence, sizeof(int));
		
		// *map_location location_constraints;
		posix_fwrite(fp, &rule.location_constraints.x, sizeof(int));
		posix_fwrite(fp, &rule.location_constraints.y, sizeof(int));

		// *int probability
		posix_fwrite(fp, &rule.probability, sizeof(int));

		// size of constraints
		size = rule.constraints.size();
		posix_fwrite(fp, &size, sizeof(uint32_t));

		// constraint_set constraints
		for (terrain_builder::constraint_set::const_iterator constraint = rule.constraints.begin(); constraint != rule.constraints.end(); ++constraint) {
			// typedef std::vector<terrain_constraint> constraint_set;
			posix_fwrite(fp, &constraint->loc.x, sizeof(int));
			posix_fwrite(fp, &constraint->loc.y, sizeof(int));

			//
			// terrain_constraint
			//

			// t_translation::t_match terrain_types_match;
			const t_translation::t_match& match = constraint->terrain_types_match;

//			struct t_match{
//				......
//				t_list terrain;
//				t_list mask;
//				t_list masked_terrain;
//				bool has_wildcard;
//				bool is_empty;
//			};

			// typedef std::vector<t_terrain> t_list;
			// terrain, mask��masked_terrain�϶�һ������, Ϊ��ʡ�ռ�,ֻдһ������ֵ
			size = match.terrain.size();
			posix_fwrite(fp, &size, sizeof(size));
			t_list_to_fp(fp, match.terrain, idx, size);
			t_list_to_fp(fp, match.mask, idx, size);
			t_list_to_fp(fp, match.masked_terrain, idx, size);
			u32n = match.has_wildcard? 1: 0;
			posix_fwrite(fp, &u32n, sizeof(int));
			u32n = match.is_empty? 1: 0;
			posix_fwrite(fp, &u32n, sizeof(int));

			// std::vector<std::string> set_flag;
			vstr_to_fp(fp, constraint->set_flag, idx, size, u32n, max_str_len);
			// std::vector<std::string> no_flag;
			vstr_to_fp(fp, constraint->no_flag, idx, size, u32n, max_str_len);
			// std::vector<std::string> has_flag;
			vstr_to_fp(fp, constraint->has_flag, idx, size, u32n, max_str_len);

			// (typedef std::vector<rule_image> rule_imagelist) rule_imagelist images
			size = constraint->images.size();
			posix_fwrite(fp, &size, sizeof(size));
			for (idx = 0; idx < size; idx ++) {
				const struct terrain_builder::rule_image& ri = constraint->images[idx];
				// rule_image

//				struct rule_image {
//					......
//					int layer;
//					int basex, basey;
//					bool global_image;
//					int center_x, center_y;
//					rule_image_variantlist variants;
//				}

				posix_fwrite(fp, &ri.layer, sizeof(int));
				posix_fwrite(fp, &ri.basex, sizeof(int));
				posix_fwrite(fp, &ri.basey, sizeof(int));
				u32n = ri.global_image? 1: 0;
				posix_fwrite(fp, &u32n, sizeof(int));
				posix_fwrite(fp, &ri.center_x, sizeof(int));
				posix_fwrite(fp, &ri.center_y, sizeof(int));

				// (std::vector<rule_image_variant>) variants
				size1 = ri.variants.size();
				posix_fwrite(fp, &size1, sizeof(size));
				for (std::vector<terrain_builder::rule_image_variant>::const_iterator imgitor = ri.variants.begin(); imgitor != ri.variants.end(); ++imgitor) {
					// value: rule_image_variant

//					struct rule_image_variant {
//						......
//						std::string image_string;
//						std::string variations;
//						std::string tod;
//						animated<image::locator> image;
//						bool random_start;
//					}

					u32n = imgitor->image_string.size();
					posix_fwrite(fp, &u32n, sizeof(u32n));	
					posix_fwrite(fp, imgitor->image_string.c_str(), u32n);
					max_str_len = posix_max(max_str_len, u32n);

					u32n = imgitor->variations.size();
					posix_fwrite(fp, &u32n, sizeof(u32n));	
					posix_fwrite(fp, imgitor->variations.c_str(), u32n);
					max_str_len = posix_max(max_str_len, u32n);

					u32n = imgitor->random_start? 1: 0;
					posix_fwrite(fp, &u32n, sizeof(int));


					//
					// ����Ϊֹ��ֻ��rule_image_variant�е�imageû������
					// image�漰��������class animated��class locator������������private��Ա���ݣ�����ֱ�Ӹ�ֵ
					// ���ǵ�fromʱҲ����ù��캯���Թ����࣬��������Ϲ��캯����Ĳ��������ˡ�
					// ����ɹ��캯������animated��locator�ο���bool terrain_builder::start_animation(building_rule &rule)
					//

					// class animated
					// int starting_frame_time_;
					// bool does_not_change_;
					// bool started_;
					// bool need_first_update_;
					// int start_tick_;
					// bool cycles_;
					// double acceleration_;
					// int last_update_tick_;
					// int current_frame_key_;
					// +std::vector<frame> frames_;
					// int duration_;
					// int start_time_;
					// +T value_;
					// +class locator
					// static int last_index_;
					// int index_;
					// value val_;

//					struct value {
//						......
//						type type_;
//						std::string filename_;
//						map_location loc_;
//						std::string modifications_;
//						int center_x_;
//						int center_y_;
//					}

				}
			}
		}
	}

	// �������Ĵ洢����С
	posix_fseek(fp, 16);
	posix_fwrite(fp, &max_str_len, sizeof(max_str_len));

	posix_fclose(fp);

	posix_print("------<xwml.cpp>::wml_building_rules_to_file, return\n");
	return;
}

#define MAXLEN_BR_STRPLUS1		270

typedef struct {
	int first;
	int second;
} tmp_pair;

terrain_builder::building_rule* wml_building_rules_from_file(const std::string& fname, uint32_t* rules_size_ptr)
{
	posix_file_t fp = INVALID_FILE;
	int64_t fsize;
	uint32_t datalen, max_str_len, rules_size, idx, len, size, size1, idx1, size2, idx2;
	uint8_t* data = NULL, *strbuf = NULL, *variations = NULL;
	uint8_t* rdpos;
	terrain_builder::building_rule * rules = NULL;
	map_location loc;
	tmp_pair tmppair;

	posix_print("<xwml.cpp>::wml_config_from_file------fname: %s\n", fname.c_str());

	if (rules_size_ptr) {
		*rules_size_ptr = 0;
	}

	posix_fopen(fname.c_str(), GENERIC_READ, OPEN_EXISTING, fp);
	if (fp == INVALID_FILE) {
		posix_print("------<xwml.cpp>::wml_building_rules_from_file, cannot create %s for read\n", fname.c_str());
		return NULL;
	}
	fsize = posix_fsize(fp);
	if (fsize <= 16 + sizeof(max_str_len) + sizeof(rules_size)) {
		posix_fclose(fp);
		return NULL;
	}
	posix_fseek(fp, 16);
	posix_fread(fp, &max_str_len, sizeof(max_str_len));
	posix_fread(fp, &rules_size, sizeof(rules_size));

	datalen = fsize - 16 - sizeof(max_str_len) - sizeof(rules_size);
	data = (uint8_t *)malloc(datalen);
	strbuf = (uint8_t *)malloc(max_str_len + 1);
	variations = (uint8_t *)malloc(max_str_len + 1);

	// read file data to memory
	posix_fread(fp, data, datalen);

	posix_print("max_str_len: %u, fsize: %u, datalen: %u\n", max_str_len, (int)fsize, datalen);
	
	rdpos = data;

	// allocate memory for rules pointer array
	if (rules_size) {
		// rules = (terrain_builder::building_rule**)malloc(rules_size * sizeof(terrain_builder::building_rule**));
		rules = new terrain_builder::building_rule[rules_size];
	} else {
		rules = NULL;
	}

	uint32_t rule_index = 0;
/*
	uint32_t previous, current, start, stop = SDL_GetTicks();
*/
	while (rdpos < data + datalen) {
		int x, y;
/*
		start = stop;
		posix_print("#%04u, (", rule_index);
*/
		// building_ruleset::iterator 
		// terrain_builder::building_rule& pbr = *rules.insert(terrain_builder::building_rule());
		// rules.push_back(terrain_builder::building_rule());
		// terrain_builder::building_rule& pbr = rules.back();
		// rules[rule_index] = new terrain_builder::building_rule;
		terrain_builder::building_rule& pbr = rules[rule_index];
/*
		previous = SDL_GetTicks();
		posix_print("%u", previous - start);
*/

		// precedence
		memcpy(&pbr.precedence, rdpos, sizeof(int));
		rdpos = rdpos + sizeof(int);

		// *map_location location_constraints;
		memcpy(&x, rdpos, sizeof(int));
		memcpy(&y, rdpos + sizeof(int), sizeof(int));
		rdpos = rdpos + 2 * sizeof(int);
		pbr.location_constraints = map_location(x, y);

		// *int probability
		memcpy(&pbr.probability, rdpos, sizeof(int));
		rdpos = rdpos + sizeof(int);

		// local
		pbr.local = false;

		// size of constraints
		memcpy(&size, rdpos, sizeof(int));
		rdpos = rdpos + sizeof(int);
/*
		current = SDL_GetTicks();
		posix_print(" + %u", current - previous);
		previous = current;
*/
		for (idx = 0; idx < size; idx ++) {
			// terrain_constraint
			memcpy(&tmppair, rdpos, 8);
			rdpos = rdpos + 8;
			loc = map_location(tmppair.first, tmppair.second);

			// pbr.constraints[loc] = terrain_builder::terrain_constraint(loc);
			// constraint = pbr.constraints.find(loc);
			pbr.constraints.push_back(terrain_builder::terrain_constraint(loc));
			terrain_builder::terrain_constraint& constraint = pbr.constraints.back();

			//
			// t_mach
			//
			t_translation::t_match& match = constraint.terrain_types_match;

			// memcpy(&x, rdpos, sizeof(int));
			// memcpy(&y, rdpos, sizeof(int));
			// rdpos = rdpos + 2 * sizeof(int);

			// size of terrain in t_mach
			// constraints[loc].terrain_types_match = t_translation::t_terrain(x, y);

			memcpy(&size1, rdpos, sizeof(uint32_t));
			rdpos = rdpos + sizeof(uint32_t);
			for (idx1 = 0; idx1 < size1; idx1 ++) {
				memcpy(&tmppair, rdpos, 8);
				match.terrain.push_back(t_translation::t_terrain(tmppair.first, tmppair.second));
				rdpos = rdpos + 8;
			}
			for (idx1 = 0; idx1 < size1; idx1 ++) {
				memcpy(&tmppair, rdpos, 8);
				match.mask.push_back(t_translation::t_terrain(tmppair.first, tmppair.second));
				rdpos = rdpos + 8;
			}
			for (idx1 = 0; idx1 < size1; idx1 ++) {
				memcpy(&tmppair, rdpos, 8);
				match.masked_terrain.push_back(t_translation::t_terrain(tmppair.first, tmppair.second));
				rdpos = rdpos + 8;
			}
			memcpy(&tmppair, rdpos, 8);
			match.has_wildcard = tmppair.first? true: false;
			match.is_empty = tmppair.second? true: false;
			rdpos = rdpos + 8;

			// size of flags in set_flag
			memcpy(&size1, rdpos, sizeof(uint32_t));
			rdpos = rdpos + sizeof(uint32_t);
			for (idx1 = 0; idx1 < size1; idx1 ++) {
				memcpy(&len, rdpos, sizeof(uint32_t));
				memcpy(strbuf, rdpos + sizeof(uint32_t), len);
				strbuf[len] = 0;
				constraint.set_flag.push_back((char*)strbuf);
				rdpos = rdpos + sizeof(uint32_t) + len;				
			}
			// size of flags in no_flag
			memcpy(&size1, rdpos, sizeof(uint32_t));
			rdpos = rdpos + sizeof(uint32_t);
			for (idx1 = 0; idx1 < size1; idx1 ++) {
				memcpy(&len, rdpos, sizeof(uint32_t));
				memcpy(strbuf, rdpos + sizeof(uint32_t), len);
				strbuf[len] = 0;
				constraint.no_flag.push_back((char*)strbuf);
				rdpos = rdpos + sizeof(uint32_t) + len;				
			}
			// size of flags in has_flag
			memcpy(&size1, rdpos, sizeof(uint32_t));
			rdpos = rdpos + sizeof(uint32_t);
			for (idx1 = 0; idx1 < size1; idx1 ++) {
				memcpy(&len, rdpos, sizeof(uint32_t));
				memcpy(strbuf, rdpos + sizeof(uint32_t), len);
				strbuf[len] = 0;
				constraint.has_flag.push_back((char*)strbuf);
				rdpos = rdpos + sizeof(uint32_t) + len;				
			}

			// size of rule_image in rule_imagelist
			memcpy(&size1, rdpos, sizeof(uint32_t));
			rdpos = rdpos + sizeof(uint32_t);
			for (idx1 = 0; idx1 < size1; idx1 ++) {
				// struct terrain_builder::rule_image& ri = constraint->second.images[idx1];
				// rule_image
				int layer, center_x, center_y;
				bool global_image;

				memcpy(&layer, rdpos, sizeof(int));
				memcpy(&x, rdpos + 4, sizeof(int));
				memcpy(&y, rdpos + 8, sizeof(int));
				memcpy(&len, rdpos + 12, sizeof(int));
				global_image = len? true: false;
				memcpy(&center_x, rdpos + 16, sizeof(int));
				memcpy(&center_y, rdpos + 20, sizeof(int));
				rdpos = rdpos + 24;

				constraint.images.push_back(terrain_builder::rule_image(layer, x, y, global_image, center_x, center_y));

				// size of rule_image in rule_imagelist
				memcpy(&size2, rdpos, sizeof(uint32_t));
				rdpos = rdpos + sizeof(uint32_t);
				for (idx2 = 0; idx2 < size2; idx2 ++) {
					bool random_start;

					memcpy(&len, rdpos, sizeof(uint32_t));
					memcpy(strbuf, rdpos + sizeof(uint32_t), len);
					strbuf[len] = 0;
					rdpos = rdpos + sizeof(uint32_t) + len;

					// Adds the main (default) variant of the image, if present
					memcpy(&len, rdpos, sizeof(uint32_t));
					memcpy(variations, rdpos + sizeof(uint32_t), len);
					variations[len] = 0;
					rdpos = rdpos + sizeof(uint32_t) + len;

					memcpy(&len, rdpos, sizeof(int));
					random_start = len? true: false;
					rdpos = rdpos + sizeof(int);
					constraint.images.back().variants.push_back(terrain_builder::rule_image_variant((char*)strbuf, (char*)variations, random_start));
				}
			}
/*
			current = SDL_GetTicks();
			posix_print(" + %u", current - previous);
			previous = current;
*/
		}
/*
		stop = SDL_GetTicks();
		posix_print("), expend %u ms\n", stop - start);
*/
		rule_index ++;
	}

	if (fp != INVALID_FILE) {
		posix_fclose(fp);
	}
	if (data) {
		free(data);
	}
	if (strbuf) {
		free(strbuf);
	}
	if (variations) {
		free(variations);
	}

	if (rules_size_ptr) {
		*rules_size_ptr = rule_index;
	}

	posix_print("------<xwml.cpp>::wml_building_rules_from_file, restore %u rules, return\n", rule_index);
	return rules;
}

// @short_res_path: if null, dir in dirs will save directly. else prefix of res_path will replace by short_res_path.
void generate_topology_internal(const std::set<std::string>& dirs, const std::map<int, std::set<std::string> >& files, const std::map<int, int>& dir_index_map, const std::string& res_path, const std::string& short_res_path, const std::string& dir_sum, const std::string& file_sum)
{
	int len = 0;
	if (!dirs.empty()) {
		len = 0;
		int at = 0;
		tfile lock(res_path + "\\xwml\\" + dir_sum, GENERIC_WRITE, CREATE_ALWAYS);
		for (std::set<std::string>::const_iterator it = dirs.begin(); it != dirs.end(); ++ it, at ++) {
			std::stringstream prefix;
			prefix << at << "=";

			std::string dir = short_res_path.empty()? *it: (short_res_path + it->substr(res_path.size()));

			lock.resize_data(posix_align_ceil(len + 1 + prefix.str().size() + dir.size(), 1024), len);

			if (len) {
				lock.data[len ++] = '\n';
			}
			// 1=
			memcpy(lock.data + len, prefix.str().c_str(), prefix.str().size());
			len += prefix.str().size();

			memcpy(lock.data + len, dir.c_str(), dir.size());
			len += dir.size();
		}
		posix_fwrite(lock.fp, lock.data, len);
	}

	if (!files.empty()) {
		len = 0;
		tfile lock(res_path + "\\xwml\\" + file_sum, GENERIC_WRITE, CREATE_ALWAYS);
		for (std::map<int, std::set<std::string> >::const_iterator it = files.begin(); it != files.end(); ++ it) {
			std::stringstream prefix;
			prefix << dir_index_map.find(it->first)->second << "=";

			const std::set<std::string>& files = it->second;

			int len2 = prefix.str().size();
			for (std::set<std::string>::const_iterator it = files.begin(); it != files.end(); ++ it) {
				if (len2) {
					len2 ++;
				}
				const std::string& file = *it;
				len2 += file.size();
			}
			lock.resize_data(posix_align_ceil(len + 1 + len2, 2048), len);

			if (len) {
				lock.data[len ++] = '\n';
			}

			// 1=
			memcpy(lock.data + len, prefix.str().c_str(), prefix.str().size());
			len += prefix.str().size();

			for (std::set<std::string>::const_iterator it = files.begin(); it != files.end(); ++ it) {
				if (it != files.begin()) {
					lock.data[len ++] = ',';
				}
				const std::string& file = *it;
				memcpy(lock.data + len, file.c_str(), file.size());
				len += file.size();
			}
		}
		posix_fwrite(lock.fp, lock.data, len);
	}
}