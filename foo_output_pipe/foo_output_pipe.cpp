#include "stdafx.h"
//#include "wavefile.h"
#include "ConIo.h"
#define COMPONENT_NAME "foo_output_pipe"

DECLARE_COMPONENT_VERSION(
COMPONENT_NAME,
"0.0.1",
"pipes audio via a console app"
);

VALIDATE_COMPONENT_FILENAME("foo_output_pipe.dll");

class myinitquit : public initquit {
public:
	void on_init() {
		console::print(COMPONENT_NAME " component: on_init()");
		
	}
	void on_quit() {
		console::print(COMPONENT_NAME " component: on_quit()");
	}
};

static initquit_factory_t<myinitquit> g_myinitquit_factory;

//CWaveFile wf;
int g_iswriting=0;
CConIo * g_conio;

class mycapturestream : public playback_stream_capture_callback, public service_base {
private:
	int count;
public:
	
	void on_chunk(const audio_chunk &d) {
		//if (d.get_used_size() == 0) return;
		console::printf(COMPONENT_NAME " got chunk #%d: samplecount=%d used size=%d", ++count, d.get_sample_count(), d.get_used_size() );
		//WAVEFORMATEX wfx;
		mem_block_container_impl_t<> out;
		DWORD written;

		out.set_size(d.get_used_size());
		
		d.toFixedPoint(out, 16, 16);
		if (!g_iswriting) {
			g_iswriting = 1;
			pfc::string_formatter s;
			WCHAR buf[256];
			s << "D:\\ffmpeg32\\bin\\ffmpeg.exe -f s16le -ar " << d.get_sample_rate() << " -ac " << d.get_channels() << " -i - -y d:\\out.mp3";
			//DeleteFile(L"d:\\out.mp3");
			//s << "D:\\lame\\lame.exe -r  - d:\\out.mp3";
			console::printf(COMPONENT_NAME " %s", s.toString());
			pfc::stringcvt::convert_utf8_to_wide(buf, sizeof(buf), s.toString(), s.get_length());
			g_conio = new CConIo(buf);
			g_conio->start();
		}
		if (g_conio->Write(out.get_ptr(), out.get_size(), &written)) {
			console::printf(COMPONENT_NAME " written %d bytes to file", written);
		}
		else {
			console::printf(COMPONENT_NAME "Unable to write to pipe");
		}
#if 0
		if (!g_iswriting) {
			g_iswriting = 1;
			wfx.cbSize = sizeof(wfx);
			wfx.nChannels = d.get_channels();
			wfx.nSamplesPerSec = d.get_sample_rate();
			wfx.wBitsPerSample = 16;
			wfx.wFormatTag = WAVE_FORMAT_PCM;
			wfx.nBlockAlign = (16 * 2) / 8;
			wfx.nAvgBytesPerSec = wfx.nBlockAlign*wfx.nSamplesPerSec;
			wf.Open(L"D:\\out.wav", &wfx, NULL);
		}
		wf.Write(out.get_size(), (BYTE*)out.get_ptr(), &written);
#endif
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
		console::print(COMPONENT_NAME " on_playback_pause");
		if (!p_state) {
			api->add_callback(&g_mycapturestream);
		} else {
			//wf.Close();
			g_iswriting = 0;
			delete g_conio;
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
