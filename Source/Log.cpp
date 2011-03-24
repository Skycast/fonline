#include "StdAfx.h"
#include "Log.h"
#include "Common.h"
#include "Timer.h"
#include <Richedit.h>
#pragma comment(lib,"User32.lib")

Mutex LogLocker;
int LoggingType=0;
void* LogFileHandle=NULL;
LogFuncPtr LogFunction=NULL;
HWND LogDlgItem=NULL;
std::string LogBufferStr;
HANDLE LogBufferEvent;
bool LoggingWithTime=false;
bool LoggingWithThread=false;
THREAD char LogThreadName[64]={0};
uint StartLogTime=Timer::FastTick();

void LogToFile(const char* fname)
{
	LogFinish(LOG_FILE);
	if(!fname) return;

	LogFileHandle=FileOpen(fname,true);
	if(!LogFileHandle)
	{
		LogFileHandle=NULL;
		return;
	}
	LoggingType|=LOG_FILE;
}

void LogToFunc(LogFuncPtr func_ptr)
{
	LogFinish(LOG_FUNC);
	LogFunction=func_ptr;
	LoggingType|=LOG_FUNC;
}

void LogToDlg(void* dlg_item)
{
	LogFinish(LOG_DLG);
	if(!dlg_item) return;
	LogDlgItem=*(HWND*)dlg_item;
	if(!LogDlgItem) return;
	LoggingType|=LOG_DLG;
}

void LogToBuffer(void* event)
{
	LogFinish(LOG_BUFFER);
	LogBufferStr.reserve(MAX_LOGTEXT*2);
	LogBufferEvent=*(HANDLE*)event;
	LoggingType|=LOG_BUFFER;
}

void LogToDebugOutput()
{
	LogFinish(LOG_DEBUG_OUTPUT);
	LoggingType|=LOG_DEBUG_OUTPUT;
}

void LogSetThreadName(const char* name)
{
	StringCopy(LogThreadName,name);
}

int LogGetType()
{
	return LoggingType;
}

void LogFinish(int log_type)
{
	log_type&=LoggingType;
	if(log_type&LOG_FILE)
	{
		if(LogFileHandle) FileClose(LogFileHandle);
		LogFileHandle=NULL;
	}
	if(log_type&LOG_FUNC) LogFunction=NULL;
	if(log_type&LOG_DLG) LogDlgItem=NULL;
	if(log_type&LOG_BUFFER)
	{
		LogBufferStr.clear();
		LogBufferEvent=NULL;
	}
	LoggingType^=log_type;
}

void WriteLog(const char* func, const char* frmt, ...)
{
	if(!LoggingType) return;

	LogLocker.Lock();

	char str_tid[64]={0};
	if(LoggingWithThread)
	{
		if(LogThreadName[0]) sprintf(str_tid,"[%s]",LogThreadName);
		else sprintf(str_tid,"[%04u]",GetCurrentThreadId());
	}

	char str_time[64]={0};
	if(LoggingWithTime)
	{
		uint delta=Timer::FastTick()-StartLogTime;
		uint seconds=delta/1000;
		uint minutes=seconds/60%60;
		uint hours=seconds/60/60;
		if(hours) sprintf(str_time,"[%03u:%02u:%02u:%03u]",hours,minutes,seconds%60,delta%1000);
		else if(minutes) sprintf(str_time,"[%02u:%02u:%03u]",minutes,seconds%60,delta%1000);
		else sprintf(str_time,"[%02u:%03u]",seconds%60,delta%1000);
	}

	char str[MAX_LOGTEXT]={0};
	if(str_tid[0]) StringAppend(str,str_tid);
	if(str_time[0]) StringAppend(str,str_time);
	if(str_tid[0] || str_time[0]) StringAppend(str," ");
	if(func) StringAppend(str,func);

	size_t len=strlen(str);
	va_list list;
	va_start(list,frmt);
#ifdef FO_MSVC
	vsprintf_s(&str[len],MAX_LOGTEXT-len,frmt,list);
#else
    vsprintf(&str[len],frmt,list);
#endif
	va_end(list);

	if(LoggingType&LOG_FILE)
	{
		FileWrite(LogFileHandle,str,strlen(str));
	}
	if(LoggingType&LOG_FUNC)
	{
		(*LogFunction)(str);
	}
	if(LoggingType&LOG_DLG)
	{
		SendMessage(LogDlgItem,EM_SETSEL,-1,-1);
		SendMessage(LogDlgItem,EM_REPLACESEL,0,(LPARAM)str);
	}
	if(LoggingType&LOG_BUFFER)
	{
		LogBufferStr+=str;
		SetEvent(LogBufferEvent);
	}
	if(LoggingType&LOG_DEBUG_OUTPUT)
	{
		OutputDebugString(str);
	}

	LogLocker.Unlock();
}

void LogWithTime(bool enabled)
{
	LoggingWithTime=enabled;
}

void LogWithThread(bool enabled)
{
	LoggingWithThread=enabled;
}

void LogGetBuffer(std::string& buf)
{
	SCOPE_LOCK(LogLocker);

	buf=LogBufferStr;
	LogBufferStr.clear();
}
