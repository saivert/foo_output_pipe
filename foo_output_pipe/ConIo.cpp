#include "stdafx.h"
#include "ConIo.h"
#include <iterator>
#include <vector>
#include <iostream>
#include <fstream>
CConIo::CConIo(LPWSTR child) : isRunning(true), child_input_write(NULL), child_input_read(NULL)
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
	startup_info.wShowWindow = SW_HIDE;
#endif
	CreateProcess(NULL, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &startup_info, &process_info);
	CloseHandle(child_input_read);

	DWORD bytes_written;
	while (isRunning) {
		while (m_queue.size()) {
			std::vector<char> v;
			v = m_queue.front();
			isRunning = WriteFile(child_input_write, v.data(), v.size(), &bytes_written, NULL)>0;
			m_queue.pop();
		}
	}

}

void CConIo::Write(void* data, DWORD len)
{
	std::vector<char> v;
	v.resize(len);
	memcpy(&v[0], data, len);
	m_queue.push(v);
}

bool CConIo::Read(void* data, DWORD len, LPDWORD bytes_read)
{
	return ReadFile(child_output_read, data, len, bytes_read, NULL)>0;
}


CConIo::~CConIo()
{
	isRunning = false;
	if (child_input_write) CloseHandle(child_input_write);
	if (process_info.hProcess) WaitForSingleObject(process_info.hProcess, 5000);
	TerminateProcess(process_info.hProcess, 1);


	waitTillDone();
}

