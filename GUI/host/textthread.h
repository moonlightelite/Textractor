#pragma once

// textthread.h
// 8/23/2013 jichi
// Branch: ITH/TextThread.h, rev 120

#include "common.h"
#include "types.h"

class TextThread
{
	typedef std::function<std::wstring(TextThread*, std::wstring)> ThreadOutputCallback;
public:
	TextThread(ThreadParam tp, DWORD status);
	~TextThread();

	std::wstring GetStore();

	void RegisterOutputCallBack(ThreadOutputCallback cb) { Output = cb; }

	void AddText(const BYTE* data, int len);
	void AddSentence(std::wstring sentence);	

	const std::wstring name;
	const ThreadParam tp;

private:
	void Flush();

	std::vector<char> buffer;
	std::wstring storage;
	std::recursive_mutex ttMutex;

	HANDLE deletionEvent = CreateEventW(nullptr, FALSE, FALSE, NULL);
	std::thread flushThread = std::thread([&] { while (WaitForSingleObject(deletionEvent, 100) == WAIT_TIMEOUT) Flush(); });
	DWORD timestamp = GetTickCount();

	ThreadOutputCallback Output;
	DWORD status;
};

// EOF
