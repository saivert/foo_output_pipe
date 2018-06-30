#include "stdafx.h"
#include "ConIo.h"
#include <fstream>
#include <iostream>
#include <vector>
#include "io.h"

CConIo::CConIo(LPWSTR child, int samplerate, int channels, bool showconsole):
	samplerate(samplerate), channels(channels), showconsole(showconsole), curvol(1)
{
	lstrcpy(cmdline, child);

	HANDLE child_input_read;

	STARTUPINFO startup_info;
	SECURITY_ATTRIBUTES security_attributes;
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
	try {
		WIN32_OP(CreateProcess(NULL, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &startup_info, &process_info));

		CloseHandle(child_input_read);

		file_stream = file_win32_wrapper_t<false, true>::g_create_from_handle(child_input_write);

		_WriteWavHeader(file_stream);

	}
	catch (const exception_win32 e) {
		console::printf(COMPONENT_NAME " Unable to create process: %s", e.what());
	}
}

bool CConIo::isReady()
{
	return file_stream->is_eof(abrt);
}

void CConIo::Flush()
{
	FlushFileBuffers(child_input_write);
}

void CConIo::Write(const audio_chunk &d)
{
	mem_block_container_impl out;

	out.set_size(d.get_used_size());
	d.toFixedPoint(out, 16, 16, true, curvol);


	file_stream->write((char*)out.get_ptr(), out.get_size(), abrt);
}

CConIo::~CConIo()
{
	file_stream.release();
	if (WaitForSingleObject(process_info.hProcess, 5000) == WAIT_TIMEOUT) {
		TerminateProcess(process_info.hProcess, 1);
	}
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

void CConIo::_WriteWavHeader(service_ptr_t<file> file_stream)
{
	WaveHeader hdr;
	_MakeWavHeader(hdr, samplerate, 16, channels);
	hdr.file_size = UINT_MAX;

	try {
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
	catch (const exception_io e) {
		console::printf(COMPONENT_NAME " WriteWavHeader failed: %s", e.what());
	}
}
