#pragma once
#include <queue>

class CConIo : public pfc::thread
{
private:
	WCHAR cmdline[MAX_PATH];
	HANDLE child_input_read;
	HANDLE child_input_write;
	service_ptr_t<file> file_stream;
	PROCESS_INFORMATION process_info;
	STARTUPINFO startup_info;
	SECURITY_ATTRIBUTES security_attributes;
	volatile bool isRunning;
	std::queue< audio_chunk_fast_impl > m_queue;
	int samplerate;
	int channels;

	void _WriteWavHeader();

	typedef struct
	{
		char RIFF_marker[4];
		uint32_t file_size;
		char filetype_header[4];
		char format_marker[4];
		uint32_t data_header_length;
		uint16_t format_type;
		uint16_t number_of_channels;
		uint32_t sample_rate;
		uint32_t bytes_per_second;
		uint16_t bytes_per_frame;
		uint16_t bits_per_sample;
	} WaveHeader;

	void _MakeWavHeader(WaveHeader &hdr, uint32_t sample_rate, uint16_t bit_depth, uint16_t channels);
	abort_callback_dummy abrt;
public:
	CConIo(LPWSTR child, int samplerate, int channels);
	void threadProc(void);
	void Write(const audio_chunk &d);
	bool GetRunning() { return isRunning; }
	bool showconsole;
	~CConIo();
};

