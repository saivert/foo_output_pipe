#include "stdafx.h"
#include "../../SDK/component.h"
#include "ConIo.h"

DECLARE_COMPONENT_VERSION(
COMPONENT_NAME,
"0.0.1",
"foo_output_pipe\n"
"\n"
"pipes audio via a console app\n"
"Fork me @ https://github.com/saivert/foo_output_pipe\n"
"\n"
"Copyright (c) 2016 Nicolai Syvertsen\n"
);

VALIDATE_COMPONENT_FILENAME("foo_output_pipe.dll");

CConIo * g_conio=nullptr;
CConIo * g_coniopbstream = nullptr;

extern cfg_int cfg_captureplaybackstream;
extern cfg_int cfg_showconsolewindow;
extern cfg_string cfg_cmdline;

class mycapturestream : public playback_stream_capture_callback, public service_base {
public:
	
	void on_chunk(const audio_chunk &d) {

		if (!g_coniopbstream) {
			pfc::string8 s, b;
			WCHAR buf[MAX_PATH];
			s = cfg_cmdline;
			
			s.replace_string("%samplerate%",  pfc::format_int(d.get_sample_rate()));
			s.replace_string("%channels%", pfc::format_int(d.get_channels()));

			console::printf(COMPONENT_NAME " Executing: %s", s.toString());
			pfc::stringcvt::convert_utf8_to_wide(buf, sizeof(buf), s.toString(), s.get_length());
			g_coniopbstream = new CConIo(buf, d.get_sample_rate(), d.get_channels(), cfg_showconsolewindow == 1);

			g_coniopbstream->Write(d);
		}
		else {
			g_coniopbstream->Write(d);

			if (!g_coniopbstream->isReady()) {
				console::printf(COMPONENT_NAME "Write aborted. Shutting down.");
				delete g_coniopbstream;
				g_coniopbstream = nullptr;
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
		if (!cfg_captureplaybackstream) return;
		if (!p_state) {
			api->add_callback(&g_mycapturestream);
		} else {
			if (g_coniopbstream) {
				delete g_coniopbstream;
				g_coniopbstream = nullptr;
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

// {B9C1343E-FD31-40AD-80FD-E1913D1A126E}
static const GUID myoutputclass_GUID =
{ 0xb9c1343e, 0xfd31, 0x40ad,{ 0x80, 0xfd, 0xe1, 0x91, 0x3d, 0x1a, 0x12, 0x6e } };
#define OUTPUT_NAME "foo_output_pipe"

class myoutputclass : public output {
private:
	double buffer_length;
	t_uint32 bitdepth;
	double vol;
public:

	myoutputclass(const GUID & p_device, double p_buffer_length, bool p_dither, t_uint32 p_bitdepth) : buffer_length(p_buffer_length), bitdepth(p_bitdepth) {
		if (!g_conio) {
			pfc::string8 s, b;
			WCHAR buf[MAX_PATH];
			s = cfg_cmdline;

			//s.replace_string("%samplerate%", pfc::format_int(p_chunk.get_sample_rate()));
			//s.replace_string("%channels%", pfc::format_int(p_chunk.get_channels()));

			console::printf(COMPONENT_NAME " Executing: %s", s.toString());
			pfc::stringcvt::convert_utf8_to_wide(buf, sizeof(buf), s.toString(), s.get_length());
			g_conio = new CConIo(buf, 44100, 2, cfg_showconsolewindow == 1);

		}
	}

	~myoutputclass() {
		if (g_conio) {
			delete g_conio;
			g_conio = nullptr;
		}
	}

	//! Retrieves amount of audio data queued for playback, in seconds.
	double get_latency() {
		return 1;
	}
	//! Sends new samples to the device. Allowed to be called only when update() indicates that the device is ready.
	void process_samples(const audio_chunk & p_chunk) {
		if (g_conio) g_conio->Write(p_chunk);
	}
	//! Updates playback; queries whether the device is ready to receive new data.
	//! @param p_ready On success, receives value indicating whether the device is ready for next process_samples() call.
	void update(bool & p_ready) {
		p_ready = g_conio && g_conio->isReady();
	}
	//! Pauses/unpauses playback.
	void pause(bool p_state) {
	}
	//! Flushes queued audio data. Called after seeking.
	void flush() {
		if (g_conio) g_conio->Flush();
	}
	//! Forces playback of queued data. Called when there's no more data to send, to prevent infinite waiting if output implementation starts actually playing after amount of data in internal buffer reaches some level.
	void force_play() {
		g_conio->Abort();
	}

	//! Sets playback volume.
	//! @p_val Volume level in dB. Value of 0 indicates full ("100%") volume, negative values indciate different attenuation levels.
	void volume_set(double p_val) {
		if (g_conio) g_conio->SetVol(pow(10.0, p_val / 20.0));
	}

	static void g_enum_devices(output_device_enum_callback & p_callback) {
		p_callback.on_device(myoutputclass_GUID, OUTPUT_NAME, sizeof(OUTPUT_NAME));
	}

	static GUID g_get_guid() {
		return myoutputclass_GUID;
	}

	static const char * g_get_name() {
		return OUTPUT_NAME;
	}

	static void g_advanced_settings_popup(HWND p_parent, POINT p_menupoint) {
	}

	static bool g_advanced_settings_query() {
		return false;
	}

	static bool g_needs_bitdepth_config() {
		return false;
	}
	static bool g_needs_dither_config() {
		return false;
	}
	static bool g_needs_device_list_prefixes() {
		return false;
	}
	
};

static output_factory_t<myoutputclass> g_myoutputclass;
