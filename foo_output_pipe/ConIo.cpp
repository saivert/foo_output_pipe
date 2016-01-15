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

#if 0
	if (child_input_write != INVALID_HANDLE_VALUE) {
		int file_descriptor = _open_osfhandle((intptr_t)child_input_write, 0);

		if (file_descriptor != -1) {
			FILE* file = _fdopen(file_descriptor, "wb");

			if (file != NULL) {
				file_stream = new std::ofstream(file);
			}
		}
	}
#endif
	
	file_stream = file_win32_wrapper_t<false, true>::g_create_from_handle(child_input_write);

	_WriteWavHeader();

	while (isRunning) {
		if (m_queue.size()) {
			audio_chunk_impl v = m_queue.front();
			mem_block_container_impl_t<> out;

			out.set_size(v.get_used_size());

			v.toFixedPoint(out, 16, 16);
			// Check if file stream is still open and that our child process is still running.
			isRunning = file_stream.get_ptr() && (WaitForSingleObject(process_info.hProcess, 0) == WAIT_TIMEOUT);

			try {
				file_stream->write((char*)out.get_ptr(), out.get_size(), abrt);
			}
			catch (...) {
				isRunning = false;
			}
			m_queue.pop();
		}
		else WaitForSingleObject(winThreadHandle(), 100);
	}

}

void CConIo::Write(const audio_chunk &d)
{
	m_queue.push(d);
}

CConIo::~CConIo()
{
	isRunning = false;
	file_stream.release();
	if (process_info.hProcess) WaitForSingleObject(process_info.hProcess, 5000);
	TerminateProcess(process_info.hProcess, 1);


	waitTillDone();
}



void CConIo::_MakeWavHeader(WaveHeader &hdr, uint32_t sample_rate, uint16_t bit_depth, uint16_t channels)
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
	WaveHeader hdr;
	_MakeWavHeader(hdr, samplerate, 16, channels);
	hdr.file_size = UINT_MAX;

	file_stream->write(hdr.RIFF_marker, 4, abrt);
	file_stream->write_lendian_t<uint32_t>(hdr.file_size, abrt);
	file_stream->write(hdr.filetype_header, 4, abrt);
	file_stream->write(hdr.format_marker, 4, abrt);
	file_stream->write_lendian_t<uint32_t>(hdr.data_header_length, abrt);
	file_stream->write_lendian_t<uint16_t>(hdr.format_type, abrt);
	file_stream->write_lendian_t<uint16_t>(hdr.number_of_channels, abrt);
	file_stream->write_lendian_t<uint32_t>(hdr.sample_rate, abrt);
	file_stream->write_lendian_t<uint32_t>(hdr.bytes_per_second, abrt);
	file_stream->write_lendian_t<uint16_t>(hdr.bytes_per_frame, abrt);
	file_stream->write_lendian_t<uint16_t>(hdr.bits_per_sample, abrt);
	file_stream->write("data", 4, abrt);

	uint32_t data_size = hdr.file_size - 36;
	file_stream->write_lendian_t<uint32_t>(data_size, abrt);
}
