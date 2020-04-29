#include "pch.h"
#include "concat.h"

using namespace std;

DWORD WINAPI reader(LPVOID lpParam) {

	HANDLE hWrite = *(HANDLE*)lpParam;

	HANDLE hFind;
	HANDLE hFile;
	WIN32_FIND_DATA FindFileData;

	int read;
	const int bufSize = 100;
	char buf[bufSize];
	char separator = '\n';

	HANDLE finished;
	finished = CreateEvent(NULL, TRUE, FALSE, "finished");
	if (!finished) {
		cout << "Failed to create finished Event!" << endl;
		return 0;
	}

	HANDLE readyToRead;
	readyToRead = CreateEvent(NULL, TRUE, FALSE, "readyToRead");
	if (!readyToRead) {
		cout << "Failed to create readyToRead Event!" << endl;
		return 0;
	}

	HANDLE fileReader;
	fileReader = CreateEvent(NULL, TRUE, FALSE, "fileReader");
	if (!fileReader) {
		cout << "Failed to create fileReader Event!" << endl;
		return 0;
	}

	OVERLAPPED file = { 0 };
	file.hEvent = fileReader;

	HANDLE pipeWriter;
	pipeWriter = CreateEvent(NULL, TRUE, FALSE, "pipeWriter");
	if (!pipeWriter) {
		cout << "Failed to create pipeWriter Event!" << endl;
		return 0;
	}

	OVERLAPPED pipe = { 0 };
	pipe.hEvent = pipeWriter;

	char searchDir[MAX_PATH];
	GetModuleFileName(NULL, searchDir, MAX_PATH);

	int i = MAX_PATH - 1;
	int j = 0;

	while (i) {

		if (searchDir[i - 1] == '\\') {

			if (j) {
				char searchAttr[] = "*.txt";
				for (j = 0; j < 5; j++)
					searchDir[i + j] = searchAttr[j];

				searchDir[i + 5] = 0;
				break;
			}
			else j++;
		}
		i--;
	}

	Sleep(100);

	HANDLE readyToWrite = OpenEvent(EVENT_ALL_ACCESS, FALSE, "readyToWrite");
	if (!readyToWrite) {
		cout << "Failed to open readyToWrite Event!" << endl;
		return 0;
	}

	if ((hFind = FindFirstFile(searchDir, &FindFileData)) != INVALID_HANDLE_VALUE) {

		do {

			if (!strcmp(FindFileData.cFileName, "output.txt")) continue;

			hFile = CreateFile(FindFileData.cFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
			if (!hFile) {
				cout << "Failed to open a file!" << endl;
				return 0;
			}

			file.Offset = 0;

			do {

				ReadFile(hFile, &buf, bufSize, NULL, &file);
				WaitForSingleObject(fileReader, INFINITE);

				read = file.InternalHigh;
				file.Offset += read;

				WaitForSingleObject(readyToWrite, INFINITE);
				ResetEvent(readyToWrite);

				if (!read) {

					WriteFile(hWrite, &separator, 1, NULL, &pipe);
					WaitForSingleObject(pipeWriter, INFINITE);

					SetEvent(readyToRead);
					break;
				}

				WriteFile(hWrite, buf, read, NULL, &pipe);
				WaitForSingleObject(pipeWriter, INFINITE);

				SetEvent(readyToRead);

			} while (1);

			CloseHandle(hFile);

		} while (FindNextFile(hFind, &FindFileData));

		FindClose(hFind);
	}
	else {

		cout << "No txt files found in exe directory." << endl;

	}

	WaitForSingleObject(readyToWrite, INFINITE);

	SetEvent(finished);
	SetEvent(readyToRead);

	CloseHandle(hWrite);
	CloseHandle(finished);
	CloseHandle(readyToRead);
	CloseHandle(fileReader);
	CloseHandle(pipeWriter);
	CloseHandle(readyToWrite);

	return 0;
}

DWORD WINAPI writer(LPVOID lpParam) {

	HANDLE hRead = *(HANDLE*)lpParam;

	int read;
	const int bufSize = 100;
	char buf[bufSize];

	HANDLE readyToWrite;
	readyToWrite = CreateEvent(NULL, TRUE, FALSE, "readyToWrite");
	if (!readyToWrite) {
		cout << "Failed to create Event!" << endl;
		return 0;
	}

	HANDLE fileWriter;
	fileWriter = CreateEvent(NULL, TRUE, TRUE, "fileWriter");
	if (!fileWriter) {
		cout << "Failed to create Event!" << endl;
		return 0;
	}

	OVERLAPPED file = { 0 };
	file.hEvent = fileWriter;

	HANDLE pipeReader;
	pipeReader = CreateEvent(NULL, TRUE, TRUE, "pipeReader");
	if (!pipeReader) {
		cout << "Failed to create Event!" << endl;
		return 0;
	}

	OVERLAPPED pipe = { 0 };
	pipe.hEvent = pipeReader;

	HANDLE hFile;
	hFile = CreateFile("output.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_OVERLAPPED, NULL);
	if (!hFile) {
		cout << "Failed to open a file!" << endl;
		return 0;
	}

	Sleep(100);

	HANDLE readyToRead = OpenEvent(EVENT_ALL_ACCESS, FALSE, "readyToRead");
	if (!readyToRead) {
		cout << "Failed to open readyToRead Event!" << endl;
		return 0;
	}

	HANDLE finished = OpenEvent(SYNCHRONIZE, FALSE, "finished");
	if (!finished) {
		cout << "Failed to open finished Event!" << endl;
		return 0;
	}

	do {

		SetEvent(readyToWrite);

		WaitForSingleObject(readyToRead, INFINITE);

		if (WaitForSingleObject(finished, 0) == WAIT_OBJECT_0) break;

		ResetEvent(readyToRead);

		ReadFile(hRead, &buf, bufSize, NULL, &pipe);

		WaitForSingleObject(pipeReader, INFINITE);

		read = pipe.InternalHigh;

		WriteFile(hFile, buf, read, NULL, &file);

		WaitForSingleObject(fileWriter, INFINITE);

		file.Offset += read;

	} while (1);

	CloseHandle(hFile);
	CloseHandle(hRead);
	CloseHandle(readyToWrite);
	CloseHandle(fileWriter);
	CloseHandle(pipeReader);
	CloseHandle(readyToRead);
	CloseHandle(finished);

	return 0;
}

extern "C" __declspec(dllexport) void concat(void) {

	HANDLE hWrite, hRead;
	if (!CreatePipe(&hRead, &hWrite, NULL, 0)) {
		cout << "Failed to create pipe!" << endl;
		return;
	}

	HANDLE hWriter;
	hWriter = CreateThread(NULL, 0, &writer, &hRead, 0, NULL);

	if (!hWriter) {
		cout << "Failed to create writer thread" << endl;
		return;
	}

	HANDLE hReader;
	hReader = CreateThread(NULL, 0, &reader, &hWrite, 0, NULL);

	if (!hReader) {
		cout << "Failed to create reader thread" << endl;
		TerminateThread(hWriter, 0);
		return;
	}


	WaitForSingleObject(hWriter, INFINITE);

	CloseHandle(hWrite);
	CloseHandle(hRead);
	CloseHandle(hReader);
	CloseHandle(hWriter);

	return;
}
