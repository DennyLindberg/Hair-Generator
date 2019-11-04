#pragma once
#include <atomic>
#include <string>
#include <functional>
#include <filesystem>
#include <thread>
#include <deque>
#include "clock.h"

using FileCallbackSignature = std::function<void(std::filesystem::path)>;

struct OnFileChangeCallback
{
	std::wstring fileName;
	double lastModifiedTimestamp = 0.0;
	double lastCallbackTimestamp = 0.0;
	FileCallbackSignature callback;
};

class FileListener
{
protected:
	std::filesystem::path rootFolder;
	std::atomic_bool stopThread = false;
	std::thread listenerThread;
	std::vector<OnFileChangeCallback> callbacks;
	std::deque<std::atomic_bool>* fileModifiedStates;

public:
	FileListener();
	~FileListener();

	FileListener(const FileListener &other) = delete;

	void Bind(std::wstring filename, FileCallbackSignature callback);

	void StartThread(std::filesystem::path listenToFolder);

	void ProcessCallbacksOnMainThread();
};