#pragma once
#include <queue>

class CConIo
{
private:
	WCHAR cmdline[MAX_PATH];
	PROCESS_INFORMATION process_info;
	const int samplerate;
	const int channels;
	void _WriteWavHeader(service_ptr_t<file> file_stream);

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
	abort_callback_impl abrt;
	const bool showconsole;
	double curvol;
	HANDLE child_input_write;
	service_ptr_t<file> file_stream;
public:
	CConIo(LPWSTR child, int samplerate, int channels, bool showconsole);
	void Write(const audio_chunk &d);
	bool isReady();
	void Flush();
	void SetVol(double p_vol) {
		curvol = p_vol;
	}
	void Abort() {
		abrt.abort();
	}
	~CConIo();
};

