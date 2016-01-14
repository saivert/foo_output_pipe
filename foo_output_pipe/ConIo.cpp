#include "stdafx.h"
#include "ConIo.h"
#include <fstream>
#include <iostream>
#include <vector>
#include "io.h"

namespace little_endian_io
{
	template <typename Word>
	std::ostream& write_word(std::ostream& outs, Word value, unsigned size = sizeof(Word))
	{
		for (; size; --size, value >>= 8)
			outs.put(static_cast <char> (value & 0xFF));
		return outs;
	}
}
using namespace little_endian_io;

CConIo::CConIo(LPWSTR child, int samplerate, int channels) : isRunning(true), child_input_write(NULL), child_input_read(NULL), samplerate(samplerate), channels(channels)
{
	lstrcpy(cmdline, child);

}

void CConIo::threadProc(void)
{

	// Set the security attributes for the pipe handles created 
	security_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
	security_attributes.bInheritHandle = TRUE;
	security_attributes.lpSecurityDescriptor = NULL;
	CreatePipe(&child_input_read, &child_input_write, &security_attributes, 0);
	SetHandleInformation(child_input_write, HANDLE_FLAG_INHERIT, 0);

	// Create the child process
	ZeroMemory(&process_info, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&startup_info, sizeof(STARTUPINFO));
	startup_info.cb = sizeof(STARTUPINFO);
	startup_info.hStdInput = child_input_read;
	startup_info.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
#ifdef _DEBUG
	startup_info.wShowWindow = SW_SHOWDEFAULT;
#else
	startup_info.wShowWindow = showconsole ? SW_SHOWDEFAULT : SW_HIDE;
#endif
	CreateProcess(NULL, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &startup_info, &process_info);
	CloseHandle(child_input_read);

	if (child_input_write != INVALID_HANDLE_VALUE) {
		int file_descriptor = _open_osfhandle((intptr_t)child_input_write, 0);

		if (file_descriptor != -1) {
			FILE* file = _fdopen(file_descriptor, "wb");

			if (file != NULL) {
				file_stream = new std::ofstream(file);
			}
		}
	}

	_WriteWavHeader();

	while (isRunning) {
		if (m_queue.size()) {
			std::vector<char> v;
			v = m_queue.front();
			// Check if file stream is still open and that our child process is still running.
			isRunning = file_stream->is_open() && (WaitForSingleObject(process_info.hProcess, 0) == WAIT_TIMEOUT);
			if (!isRunning) break;

			file_stream->write(v.data(), v.size());
			file_stream->flush();
			m_queue.pop();
		}
		else WaitForSingleObject(winThreadHandle(), 100);
	}

}

void CConIo::Write(void* data, DWORD len)
{
	std::vector<char> v;
	v.resize(len);
	memcpy(&v[0], data, len);
	m_queue.push(v);
}

CConIo::~CConIo()
{
	isRunning = false;
	//if (child_input_write) CloseHandle(child_input_write);
	file_stream->close();
	delete file_stream;
	if (process_info.hProcess) WaitForSingleObject(process_info.hProcess, 5000);
	TerminateProcess(process_info.hProcess, 1);


	waitTillDone();
}

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

void init_genericWAVHeader(WaveHeader &hdr, uint32_t sample_rate, uint16_t bit_depth, uint16_t channels)
{
    memcpy(&hdr.RIFF_marker, "RIFF", 4);
    memcpy(&hdr.filetype_header, "WAVE", 4);
    memcpy(&hdr.format_marker, "fmt ", 4);
    hdr.data_header_length = 16;
    hdr.format_type = 1;
    hdr.number_of_channels = channels;
    hdr.sample_rate = sample_rate;
    hdr.bytes_per_second = sample_rate * channels * bit_depth / 8;
    hdr.bytes_per_frame = channels * bit_depth / 8;
    hdr.bits_per_sample = bit_depth;
}

void CConIo::_WriteWavHeader()
{
	std::ofstream &f = *file_stream;

	WaveHeader hdr;
	init_genericWAVHeader(hdr, samplerate, 16, channels);
	hdr.file_size = UINT_MAX;

	f.write(hdr.RIFF_marker, 4);
	write_word(f, hdr.file_size, 4);
	f.write(hdr.filetype_header, 4);
	f.write(hdr.format_marker, 4);
	write_word(f, hdr.data_header_length, 4);
	write_word(f, hdr.format_type, 2);
	write_word(f, hdr.number_of_channels, 2);
	write_word(f, hdr.sample_rate, 4);
	write_word(f, hdr.bytes_per_second, 4);
	write_word(f, hdr.bytes_per_frame, 2);
	write_word(f, hdr.bits_per_sample, 2);
	f << "data";

	uint32_t data_size = hdr.file_size - 36;
	write_word(f, data_size, 4);

	f.flush();
}
