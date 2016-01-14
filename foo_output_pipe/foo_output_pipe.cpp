#include "stdafx.h"
#include "../../SDK/component.h"
#include "ConIo.h"
#define COMPONENT_NAME "foo_output_pipe"

DECLARE_COMPONENT_VERSION(
COMPONENT_NAME,
"0.0.1",
"pipes audio via a console app"
);

VALIDATE_COMPONENT_FILENAME("foo_output_pipe.dll");

CConIo * g_conio=nullptr;

extern cfg_int cfg_enable;
extern cfg_int cfg_showconsolewindow;
extern cfg_string cfg_cmdline;

class mycapturestream : public playback_stream_capture_callback, public service_base {
private:
	int count;
public:
	
	void on_chunk(const audio_chunk &d) {
#ifdef _DEBUG
		console::printf(COMPONENT_NAME " got chunk #%d: samplecount=%d used size=%d", ++count, d.get_sample_count(), d.get_used_size() );
#endif
		mem_block_container_impl_t<> out;

		out.set_size(d.get_used_size());

		d.toFixedPoint(out, 16, 16);
		if (!g_conio) {
			pfc::string8 s, b;
			WCHAR buf[MAX_PATH];
			s = cfg_cmdline;
			
			s.replace_string("%samplerate%",  pfc::format_int(d.get_sample_rate()));
			s.replace_string("%channels%", pfc::format_int(d.get_channels()));

			console::printf(COMPONENT_NAME " Executing: %s", s.toString());
			pfc::stringcvt::convert_utf8_to_wide(buf, sizeof(buf), s.toString(), s.get_length());
			g_conio = new CConIo(buf, d.get_sample_rate(), d.get_channels());
			g_conio->showconsole = cfg_showconsolewindow==1;
			g_conio->start();
		}
		else {
			g_conio->Write(out.get_ptr(), out.get_size());

			if (!g_conio->GetRunning()) {
				console::printf(COMPONENT_NAME "Write aborted. Shutting down.");
				delete g_conio;
				g_conio = nullptr;
				static_api_ptr_t<playback_stream_capture> api;
				api->remove_callback(this);
			}
		}
	}
};

static service_impl_single_t< mycapturestream >g_mycapturestream;

class vstream_play_callback_ui :public play_callback_static{
public:
	virtual unsigned get_flags(){
		return play_callback::flag_on_playback_dynamic_info_track |
			play_callback::flag_on_playback_new_track |
			play_callback::flag_on_playback_starting |
			play_callback::flag_on_playback_stop |
			play_callback::flag_on_playback_pause |
			play_callback::flag_on_playback_edited;
	}

	virtual void on_playback_pause(bool p_state){
		static_api_ptr_t<playback_stream_capture> api;
		if (!cfg_enable) return;
		if (!p_state) {
			api->add_callback(&g_mycapturestream);
		} else {
			if (g_conio) {
				delete g_conio;
				g_conio = nullptr;
			}
			api->remove_callback(&g_mycapturestream);
		}
	}

	virtual void on_playback_new_track(metadb_handle_ptr p_track) { }
	virtual void on_playback_edited(metadb_handle_ptr p_track){ }
	virtual void on_playback_dynamic_info_track(const file_info&p_info){ }

	virtual void on_playback_starting(play_control::t_track_command p_command, bool p_paused){ on_playback_pause(p_paused); }
	virtual void on_playback_stop(play_control::t_stop_reason p_reason){
		if (!(p_reason == play_control::stop_reason_starting_another))
			this->on_playback_pause(true);
	}


	virtual void on_playback_seek(double p_time){}
	virtual void on_playback_dynamic_info(const file_info & p_info){}
	virtual void on_playback_time(double p_time){}
	virtual void on_volume_change(float p_new_val){}
};

static service_factory_single_t<vstream_play_callback_ui>vstream_callback;
