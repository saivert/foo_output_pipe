#pragma once
#include <queue>

class CConIo : public pfc::thread
{
private:
	WCHAR cmdline[MAX_PATH];
	PROCESS_INFORMATION process_info;
	volatile bool isRunning;
	std::queue< audio_chunk_fast_impl > m_queue;
	const int samplerate;
	const int channels;
	pfc::readWriteLock rwl;
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
public:
	CConIo(LPWSTR child, int samplerate, int channels, bool showconsole);
	void threadProc(void);
	void Write(const audio_chunk &d);
	bool isReady();
	void Flush();
	void SetVol(double p_vol) {
		curvol = p_vol;
	}
	bool GetRunning() { return isRunning; }
	~CConIo();
};

