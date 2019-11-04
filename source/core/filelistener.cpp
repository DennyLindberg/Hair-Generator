#include "filelistener.h"
#include <Windows.h>
#include "application.h"

void ListenToFileChange(std::atomic_bool* stopThread, std::filesystem::path folder, std::vector<OnFileChangeCallback>* fileCallbacks, std::deque<std::atomic_bool>* fileModifiedStates)
{
	/*
		Setup the listener
	*/
	HANDLE hDir = CreateFile(
		folder.c_str(),
		FILE_LIST_DIRECTORY,
		FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		NULL
	);

	if (hDir == INVALID_HANDLE_VALUE)
	{
		printf("\r\nInvalid handle");
		return;
	}

	/*
		Start listener loop
	*/
	BYTE buffer[4096];
	DWORD dwBytesReturned = 0;

	while (!(*stopThread))
	{
		bool retrievedChanges = ReadDirectoryChangesW(
			hDir,
			buffer, sizeof(buffer),
			FALSE,
			FILE_NOTIFY_CHANGE_LAST_WRITE,
			&dwBytesReturned, NULL, NULL
		);

		if (!retrievedChanges)
		{
			printf("\r\nFile listener failed...");
			break;
		}

		/*
			Process changes
		*/
		BYTE* p = buffer;
		std::vector<std::wstring> detectedFilenames;
		for (;;)
		{
			FILE_NOTIFY_INFORMATION* info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(p);

			int stringLength = info->FileNameLength / sizeof(WCHAR);
			std::wstring filename = std::wstring((WCHAR*)&info->FileName, stringLength);

			for (int i = 0; i < (*fileCallbacks).size(); ++i)
			{
				if (filename == (*fileCallbacks)[i].fileName)
				{
					(*fileCallbacks)[i].lastModifiedTimestamp = GetThreadedTime();
					(*fileModifiedStates)[i] = true;
					break;
				}
			}


			if (!info->NextEntryOffset) break;
			p += info->NextEntryOffset;
		}
	}
}

FileListener::FileListener()
{
	fileModifiedStates = new std::deque<std::atomic_bool>{};
}

FileListener::~FileListener()
{
	stopThread = true;
	if (listenerThread.joinable())
	{
		listenerThread.join();
	}
	delete fileModifiedStates;
}

void FileListener::Bind(std::wstring filename, FileCallbackSignature callback)
{
	callbacks.emplace_back(OnFileChangeCallback{
		filename,
		0.0, 0.0,
		callback
		});
	fileModifiedStates->emplace_back(false);
}

void FileListener::StartThread(std::filesystem::path listenToFolder)
{
	rootFolder = listenToFolder;
	listenerThread = std::thread(ListenToFileChange, &stopThread, rootFolder, &callbacks, fileModifiedStates);
}

void FileListener::ProcessCallbacksOnMainThread()
{
	auto& modified = *fileModifiedStates;
	double currentTime = GetThreadedTime();

	for (int i = 0; i < callbacks.size(); ++i)
	{
		if (modified[i] && callbacks[i].callback)
		{
			// FULHACK: Add delay to read, sometimes the notification is too fast and the
			// file is actually not ready to be read.
			double timeSinceModification = currentTime - callbacks[i].lastModifiedTimestamp;
			if (timeSinceModification < 0.2)
			{
				continue;
			}

			modified[i] = false;

			// Avoid double notification spam
			double timeSinceLastCallback = currentTime - callbacks[i].lastCallbackTimestamp;
			if (timeSinceLastCallback > 0.2)
			{
				callbacks[i].callback(rootFolder / callbacks[i].fileName);
				callbacks[i].lastCallbackTimestamp = currentTime;
			}
		}
	}
}