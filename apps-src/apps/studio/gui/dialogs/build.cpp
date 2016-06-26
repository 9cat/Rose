#define GETTEXT_DOMAIN "studio-lib"

#include "gui/dialogs/build.hpp"

#include "display.hpp"
#include "game_config.hpp"
#include "preferences.hpp"
#include "gettext.hpp"
#include "gui/auxiliary/timer.hpp"
#include "gui/dialogs/message.hpp"
#include "gui/widgets/button.hpp"
#include "gui/widgets/label.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/toggle_button.hpp"
#include "gui/widgets/track.hpp"
#include "gui/widgets/window.hpp"
#include "preferences_display.hpp"
#include "help.hpp"
#include "filesystem.hpp"
#include "loadscreen.hpp"
#include <time.h>

#include <boost/bind.hpp>

#include <algorithm>

namespace gui2 {

tbuild::tbuild()
	: editor_(game_config::path)
	, build_ctx_(*this)
	, task_thread_(NULL)
	, exit_task_(false)
{
}

tbuild::~tbuild()
{
	if (task_thread_) {
		exit_task_ = true;
		delete task_thread_;
		task_thread_ = NULL;
		exit_task_ = false;
	}
}

void tbuild::pre_show(ttrack& track)
{
	track.set_canvas_variable("background_image", variant(null_str));
	track.set_callback_timer(boost::bind(&tbuild::task_status_callback, this, _1, _2, _3, _4, _5, false));
	track.set_enable_drag_draw_coordinate(false);
	track.set_timer_interval(100);
	task_status_ = &track;
}

static void increment_progress_cb(std::string const &name, uint32_t param1, void* param2)
{
	tbuild::tbuild_ctx* ctx = (tbuild::tbuild_ctx*)param2;
	ctx->name = name;
	ctx->nfiles ++;
}

static int process_build(void* param)
{
	tbuild* build = reinterpret_cast<tbuild*>(param);
	build->do_build_internal();

	return 0;
}

void tbuild::do_build2()
{
	on_start_build();

	VALIDATE(!task_thread_, null_str);
	task_thread_ = new threading::thread(process_build, this);
}

void tbuild::do_build_internal()
{
	// this is in thread. don't call any operator aboult dialog.
	const std::vector<std::pair<editor::BIN_TYPE, editor::wml2bin_desc> >& descs = editor_.wml2bin_descs();
	set_increment_progress progress(increment_progress_cb, &build_ctx_);

	int count = (int)descs.size();
	for (int at = 0; at < count && !exit_task_; at ++) {
		const std::pair<editor::BIN_TYPE, editor::wml2bin_desc>& desc = descs[at];
		if (!desc.second.require_build) {
			continue;
		}
		{
			threading::lock lock(mutex_);
			tasks_.push_back(ttask(ttask::started, at, true));
			build_ctx_.reset(&desc);
		}

		bool ret = false;
		try {
			ret = editor_.load_game_cfg(desc.first, desc.second.bin_name, desc.second.app, true, desc.second.wml_nfiles, desc.second.wml_sum_size, (uint32_t)desc.second.wml_modified);
		} catch (twml_exception& e) {
			e.show();
		}
		{
			threading::lock lock(mutex_);
			tasks_.push_back(ttask(ttask::stopped, at, ret));
		}
		
	}

	build_ctx_.status = tbuild_ctx::stopping;
}

void tbuild::task_status_callback(ttrack& widget, surface& frame_buffer, const tpoint& offset, int state2, bool from_timer, bool ing)
{
	const SDL_Rect widget_rect = widget.get_rect();
	clip_rect_setter clip(frame_buffer, &widget_rect);

	const int xsrc = offset.x + widget_rect.x;
	const int ysrc = offset.y + widget_rect.y;

	SDL_Rect dst = widget_rect;

	if (!build_ctx_.desc) {
		if (!from_timer) {
			sdl_fill_rect(frame_buffer, &dst, 0xfffefefe);

			surface text_surf = font::get_rendered_text2(editor_.working_dir(), INT32_MAX, 12 * twidget::hdpi_scale, font::BLACK_COLOR);
			dst = ::create_rect(xsrc + 4 * twidget::hdpi_scale, ysrc + (widget_rect.h - text_surf->h) / 2, 0, 0);
			sdl_blit(text_surf, NULL, frame_buffer, &dst);
		}
		return;
	}

	if (from_timer) {
		sdl_blit(widget.background_surf(), NULL, frame_buffer, &dst);
	}
	if (!tasks_.empty()) {
		threading::lock lock(mutex_);
		handle_task();
	}
	if (build_ctx_.status == tbuild_ctx::stopping && build_ctx_.desc) {
		build_ctx_.reset(NULL);
		on_stop_build();
		widget.set_dirty();

		VALIDATE(task_thread_, null_str);
		delete task_thread_;
		task_thread_ = NULL;
		return;
	}

	dst.w = build_ctx_.nfiles * widget_rect.w / build_ctx_.desc->second.wml_nfiles;
	sdl_fill_rect(frame_buffer, &dst, 0xff00ff00);

	std::stringstream ss;
	ss << build_ctx_.nfiles << "/" << build_ctx_.desc->second.wml_nfiles;
	if (!build_ctx_.name.empty()) {
		ss << "    " << build_ctx_.name.substr(editor_.working_dir().size());
	}
	surface text_surf = font::get_rendered_text2(ss.str(), INT32_MAX, 12 * twidget::hdpi_scale, font::BLACK_COLOR);
	dst = ::create_rect(xsrc + 4 * twidget::hdpi_scale, ysrc + (widget_rect.h - text_surf->h) / 2, 0, 0);
	sdl_blit(text_surf, NULL, frame_buffer, &dst);
}

} // namespace gui2

