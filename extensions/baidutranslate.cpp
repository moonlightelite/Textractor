#include "extension.h"
#include "network.h"
#include <windows.h> 
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <QStringList>

extern const wchar_t* TRANSLATION_ERROR;

extern Synchronized<std::wstring> translateTo;
extern HANDLE hTranslatorPipe;
extern int messageCount;

#define NAMEDPIPE_BUFSIZE 8192

const char* TRANSLATION_PROVIDER = "Baidu Translate";
const char* GET_API_KEY_FROM;
QStringList languages
{
	"Chinese: zh",
	"English: en",
	"Japanese: ja",
};

bool translateSelectedOnly = false, rateLimitAll = true, rateLimitSelected = false, useCache = true;
int tokenCount = 30, tokenRestoreDelay = 60000, maxSentenceSize = 500;

std::pair<bool, std::wstring> Translate(const std::wstring& text)
{
	TCHAR  chBuf[NAMEDPIPE_BUFSIZE];
	DWORD  cbRead, cbToWrite, cbWritten, dwMode;
	BOOL   justConnected = false;
	BOOL   fSuccess = FALSE;
	if (hTranslatorPipe == INVALID_HANDLE_VALUE) {
		LPTSTR lpszPipename = TEXT("\\\\.\\pipe\\BAIDU_TRANSLATE");
		hTranslatorPipe = CreateFile(
			lpszPipename,   // pipe name 
			GENERIC_READ |  // read and write access 
			GENERIC_WRITE,
			0,              // no sharing 
			NULL,           // default security attributes
			OPEN_EXISTING,  // opens existing pipe 
			FILE_ATTRIBUTE_NORMAL,
			NULL);          // no template file
		justConnected = true;
	}
	if (hTranslatorPipe != INVALID_HANDLE_VALUE) {
		dwMode = PIPE_READMODE_MESSAGE;
		fSuccess = SetNamedPipeHandleState(
			hTranslatorPipe,    // pipe handle 
			&dwMode,  // new pipe mode 
			NULL,     // don't set maximum bytes 
			NULL);    // don't set maximum time 
		if (!fSuccess)
		{
			return { false, FormatString(L"%s: %s", TRANSLATION_ERROR, "Named pipe set state failed.") };
		}
		std::wstring pipeMessage = FormatString(L"[Message #%d,to=%s,from=jp]%s", messageCount++, translateTo.Copy(), Escape(text));
		cbToWrite = pipeMessage.size() * sizeof(wchar_t);

		fSuccess = WriteFile(
			hTranslatorPipe,                  // pipe handle 
			pipeMessage.c_str(),             // message 
			cbToWrite,              // message length 
			&cbWritten,             // bytes written 
			NULL);                  // not overlapped 

		if (!fSuccess)
		{
			return { false, FormatString(L"%s: %s", TRANSLATION_ERROR, "Failed to send message thru named pipe.") };
		}

		fSuccess = ReadFile(
			hTranslatorPipe,    // pipe handle 
			chBuf,    // buffer to receive reply 
			NAMEDPIPE_BUFSIZE * sizeof(TCHAR),  // size of buffer 
			&cbRead,  // number of bytes read 
			NULL);    // not overlapped 

		if (!fSuccess)
		{
			return { false, FormatString(L"%s: %s", TRANSLATION_ERROR, "Failed to read message from named pipe.") };
		}
		return { true, ((std::wstring) chBuf).substr(0, cbRead / sizeof(wchar_t)) };
		//std::wstring temp_wstringBuf = ((std::wstring) chBuf).substr(0, cbRead / sizeof(wchar_t));
        //if (std::wsmatch results; std::regex_search(temp_wstringBuf, results, std::wregex(L"^\\[Message #\\d+\\](.+)$"))) return { true, results[1] };
		//else return { false, FormatString(L"%s: %s", TRANSLATION_ERROR, "Failed to parse received message.") };
	}
	else if (justConnected && GetLastError() != ERROR_PIPE_BUSY) {
		return { false, FormatString(L"%s: %s", TRANSLATION_ERROR, "Named pipe initialization failed.") };
	}
	else {
		return { false, FormatString(L"%s: %s", TRANSLATION_ERROR, "Unknown error?") };
	}
}
