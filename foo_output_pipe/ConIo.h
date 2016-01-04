#pragma once
class CConIo : public pfc::thread
{
private:
	WCHAR cmdline[MAX_PATH];
	HANDLE child_input_read;
	HANDLE child_input_write;
	HANDLE child_output_read;
	HANDLE child_output_write;
	PROCESS_INFORMATION process_info;
	STARTUPINFO startup_info;
	SECURITY_ATTRIBUTES security_attributes;
	BYTE buffer[4096];
	bool isRunning;

public:
	CConIo(LPWSTR child);
	void threadProc(void);
	bool Write(void* data, DWORD len, LPDWORD bytes_written);
	bool Read(void* data, DWORD len, LPDWORD bytes_read);
	~CConIo();
};

