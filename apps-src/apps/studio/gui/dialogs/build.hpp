#ifndef GUI_DIALOGS_BUILD_HPP_INCLUDED
#define GUI_DIALOGS_BUILD_HPP_INCLUDED

#include "gui/dialogs/dialog.hpp"
#include "editor.hpp"
#include "thread.hpp"

class display;

namespace gui2 {
class ttrack;

class tbuild
{
public:
	tbuild();
	virtual ~tbuild();

	void do_build_internal();
	struct tbuild_ctx {
		enum {stopped, building, stopping};
		tbuild_ctx(tbuild& owner)
			: owner(owner)
			, nfiles(0)
			, desc(NULL)
			, status(stopped)
		{}
		void reset(const std::pair<editor::BIN_TYPE, editor::wml2bin_desc>* _desc)
		{
			nfiles = 0;
			name.clear();
			desc = _desc;
			if (_desc) {
				if (status != building) {
					status = building;
				}
			} else {
				if (status != stopped) {
					status = stopped;
				}
			}
		}

		size_t nfiles;
		std::string name;
		const std::pair<editor::BIN_TYPE, editor::wml2bin_desc>* desc;
		tbuild& owner;
		int status;
	};

	struct ttask {
		enum {started, stopped};
		ttask(int type, int at, bool ret)
			: type(type)
			, at(at)
			, ret(ret)
		{}
		int type;
		int at;
		bool ret;
	};

protected:
	void pre_show(ttrack& track);
	void do_build2();

private:
	virtual void on_start_build() = 0;
	virtual void on_stop_build() = 0;
	virtual void handle_task() = 0;

	void task_status_callback(ttrack& widget, surface& frame_buffer, const tpoint& offset, int state2, bool from_timer, bool ing);

protected:
	tbuild_ctx build_ctx_;
	threading::thread* task_thread_;
	threading::mutex mutex_;
	bool exit_task_;
	std::vector<ttask> tasks_;

	bool select_all_;
	ttrack* task_status_;
	editor editor_;
};

} // namespace gui2

#endif
