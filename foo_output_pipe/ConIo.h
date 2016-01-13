#pragma once
#include <stdlib.h>
#include <queue>

class CConIo : public pfc::thread
{
private:
	WCHAR cmdline[MAX_PATH];
	HANDLE child_input_read;
	HANDLE child_input_write;
	std::ofstream *file_stream;
	PROCESS_INFORMATION process_info;
	STARTUPINFO startup_info;
	SECURITY_ATTRIBUTES security_attributes;
	volatile bool isRunning;
	std::queue< std::vector<char> > m_queue;
	int samplerate;
	int channels;

	void _WriteWavHeader();

public:
	CConIo(LPWSTR child, int samplerate, int channels);
	void threadProc(void);
	void Write(void* data, DWORD len);
	bool GetRunning() { return isRunning; }
	bool showconsole;
	~CConIo();
};

