#include "pch.h"
#include <iostream>
#include <conio.h>
#include <windows.h>
#include <string>

using namespace std;

HANDLE hThread;
HANDLE hEvent;
HANDLE ready_to_exit;
HANDLE ready_to_transfer;
OVERLAPPED ovl_for_pipe;
CRITICAL_SECTION ovl;
CRITICAL_SECTION terminal;

string read_file(string path, OVERLAPPED& ovl);
bool write_file(string path, string str, OVERLAPPED& ovl);
int str_len(string str);
void thread(void);

int main()
{
	const int fnum = 3;
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);
	string path = "C:\\учеба\\4 семестр\\лабы СПОВМ\\lab 5\\Debug\\";
	string name[fnum] = { "1.txt","2.txt","3.txt"};
	DWORD IDthread;
	HANDLE hNamedPipe;
	OVERLAPPED ovl_for_read;
	ovl_for_pipe.Offset = 0;
	ovl_for_read.OffsetHigh = ovl_for_pipe.OffsetHigh = 0;
	ovl_for_read.hEvent = 0;
	ovl_for_pipe.hEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	ready_to_transfer = CreateEvent(NULL, TRUE, TRUE, NULL);//сигнализирует о считывании нового файла и его завершении.
	ready_to_exit = CreateEvent(NULL, TRUE, FALSE, NULL);//сигнализирует о выходе
	InitializeCriticalSection(&ovl);
	InitializeCriticalSection(&terminal);
	hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);// сигнализирует о очередной записи строки в канал
	if (hEvent == NULL)
		cout<<"ERROR\n";
	else
	{
		hNamedPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\my_pipe"), PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED, PIPE_WAIT | PIPE_TYPE_BYTE, 1, 0, 0, INFINITE, NULL);
		if (hNamedPipe == INVALID_HANDLE_VALUE)
			cout<<"ERROR\n";
		else
		{
			hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)thread, NULL, 0, &IDthread);
			if (hThread == NULL)
				cout<<"ERROR\n";
			else
			{
				if (!ConnectNamedPipe(hNamedPipe, &ovl_for_pipe) && GetLastError() != ERROR_IO_PENDING)
				{
					cout<<"ERROR\n";
					SetEvent(ready_to_exit);
				}
				else
				{
					SetEvent(hEvent);
					string result;
					for (int i = 0; i < fnum; i++)
					{
						WaitForSingleObject(ready_to_transfer, INFINITE);
						ovl_for_read.Offset = 0;
						do
						{
							result = read_file(path + name[i], ovl_for_read);
							int size = str_len(result);
							if (result != "")
							{
								if (!WriteFile(hNamedPipe, result.c_str(), size + 1, NULL, &ovl_for_pipe))
								{
									DWORD error = GetLastError();
									if (error != ERROR_IO_PENDING)
									{
										cout<<"ERROR\n";
										break;
									}
								}
								EnterCriticalSection(&ovl);
								ovl_for_pipe.Offset += size;
								LeaveCriticalSection(&ovl);
								SetEvent(hEvent);//уведомление о записи строки в канал
							}
						} while (result != "");
						ResetEvent(ready_to_transfer); //файл полностью считан и передан через канал
					}
					SetEvent(hEvent);
					SetEvent(ready_to_exit);
					WaitForSingleObject(hThread, INFINITE);
					DisconnectNamedPipe(hNamedPipe);
				}
				CloseHandle(hThread);
			}
			CloseHandle(hNamedPipe);
		}
		CloseHandle(hEvent);
	}
	DeleteCriticalSection(&terminal);
	DeleteCriticalSection(&ovl);
	CloseHandle(ready_to_transfer);
	CloseHandle(ovl_for_pipe.hEvent);
	CloseHandle(ready_to_exit);
	//}
	//if (!FreeLibrary(hDll))
		//cout << "Поток-читатель: Ошибка отключения DLL.Код ошибки" << GetLastError() << endl;
//}
	return 0;
}

string read_file(string path, OVERLAPPED& ovl)
{
	HANDLE file;
	file = CreateFileA(path.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (file == INVALID_HANDLE_VALUE)
	{
		cout << "ERROR/n";
		return "";
	}
	char symbol;
	string str;
	do
	{
		symbol = '\0';
		if (!ReadFile(file, &symbol, sizeof(char), NULL, &ovl))
		{
			DWORD error = GetLastError();
			switch (error)
			{
			case ERROR_IO_PENDING:
			{
				break;
			}
			case ERROR_HANDLE_EOF:
			{
				CloseHandle(file);
				return str;
			}
			default:
			{
				cout<<"ERROR\n";
				CloseHandle(file);
				return 0;
			}
			}
		}
		WaitForSingleObject(file, INFINITE);
		if (symbol != '\0')
		{
			str += symbol;
			ovl.Offset++;
		}

	} while (symbol != '\n' && symbol != '\0');
	CloseHandle(file);
	return str;
}

bool write_file(string path, string str, OVERLAPPED& ovl)
{
	HANDLE file;
	file = CreateFileA(path.c_str(), GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_FLAG_OVERLAPPED, NULL);
	if (file == INVALID_HANDLE_VALUE)
	{
		cout<<"ERROR\n";
		return false;
	}
	int length = str_len(str);
	DWORD counter;
	if (!WriteFile(file, str.c_str(), DWORD(length + 1), &counter, &ovl))
	{
		DWORD error = GetLastError();
		switch (error)
		{
		case ERROR_IO_PENDING:
		{
			break;
		}
		default:
		{
			cout<<"ERROR\n";
			CloseHandle(file);
			return false;
		}
		}
	}
	WaitForSingleObject(file, INFINITE);
	ovl.Offset += length;
	CloseHandle(file);
	return true;
}

int str_len(string str)
{
	int i = 0;
	while (str[i] != '\0')
		i++;
	return i;
}
void thread(void)
{

	OVERLAPPED ovl_for_write;
	ovl_for_write.Offset = 0;
	ovl_for_write.OffsetHigh = 0;
	ovl_for_write.hEvent = 0;
	string path_for_output = "C:\\учеба\\4 семестр\\лабы СПОВМ\\lab 5\\Debug\\result.txt";
	HANDLE hNamedPipes;
	hNamedPipes = CreateFile(TEXT("\\\\.\\pipe\\my_pipe"), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (hNamedPipes == INVALID_HANDLE_VALUE)
	{
		return;
	}
	char symbol;
	WaitForSingleObject(hEvent, INFINITE);
	ResetEvent(hEvent);
	while (WaitForSingleObject(ready_to_exit, 0) == WAIT_TIMEOUT || ovl_for_pipe.Offset != 0)
	{
		WaitForSingleObject(hEvent, INFINITE);
		if (ovl_for_pipe.Offset != 0)
		{
			string str;
			do
			{
				if (!ReadFile(hNamedPipes, &symbol, sizeof(char), NULL, &ovl_for_pipe))
				{
					DWORD error = GetLastError();
					switch (error)
					{
					case ERROR_IO_PENDING:
					{
						break;
					}
					case ERROR_HANDLE_EOF:
					{
						CloseHandle(hNamedPipes);
						break;
					}
					default:
					{
						cout<<"ERROR\n";
						CloseHandle(hNamedPipes);
						break;
					}
					}
				}
				if (symbol != '\0')
				{
					str += symbol;
					EnterCriticalSection(&ovl);
					ovl_for_pipe.Offset -= 1;
					LeaveCriticalSection(&ovl);
				}
			} while (symbol != '\0');
			if (!write_file(path_for_output, str, ovl_for_write))
				cout<<"ERROR\n";
		}
		else
		{
			ResetEvent(hEvent);
			if (WaitForSingleObject(ready_to_transfer, 0) == WAIT_TIMEOUT)
			{
				SetEvent(ready_to_transfer);
			}
		}
	}
	CloseHandle(hNamedPipes);
	//}
	//if(!FreeLibrary(hDll_t))
		//cout << "Поток-писатель: Ошибка отключения DLL.Код ошибки" << GetLastError() << endl;
//}
}
