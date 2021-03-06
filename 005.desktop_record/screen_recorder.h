#pragma once

#include <thread>
#include <mutex>
#include <atomic>
#include <list>
#include <string>

class screen_recorder
{
public:
	struct bgra {
		char* data = nullptr;
		size_t len = 0;
	};

	enum class recorder_type {
		directx,
		gdigrab,
	};

	static screen_recorder* getInstance(recorder_type type = recorder_type::directx);

	virtual ~screen_recorder() { stop(); }

	virtual bool start(int outFPS = 10, int maxCachedBgra = 10);
	virtual void stop();
	virtual bgra getBGRA(bool block = false);
	virtual int getWidth() const { return outWidth_; }
	virtual int getHeight() const { return outHeight_; }

protected:
	virtual void run() = 0;

protected:
	int outFPS_ = 10;
	int outWidth_ = 800;
	int outHeight_ = 600;
	int maxCachedBgra_ = 10;

	std::atomic_bool running_ = false;
	std::list<bgra> bgras_ = {};
	std::mutex mutex_ = {};
	std::thread thread_ = {};


	screen_recorder() {}
};
