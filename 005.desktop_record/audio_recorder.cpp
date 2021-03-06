#include "audio_recorder.h"
#include "audio_recorder_qt.h"

audio_recorder* audio_recorder::getInstance(recorder_type type)
{	
	switch (type) {
	case recorder_type::rec_qt:
	{
		static audio_recorder_qt rec;
		return &rec;
	}
	default:return nullptr;
	}
	
}

bool audio_recorder::start(int sample_rate, int channels, int max_cached_pcms)
{
	stop();
	sample_rate_ = sample_rate;
	channels_ = channels;
	max_cached_pcms_ = max_cached_pcms;
	return true;
}

void audio_recorder::stop()
{
	if (!running_) { return; }
	running_ = false;
	std::lock_guard<std::mutex> lg(mutex_);
	if (thread_.joinable()) {
		thread_.join();
	}
	for (auto p : pcms_) {
		delete[] p.data;
	}
	pcms_.clear();
}

audio_recorder::pcm audio_recorder::getPCM(bool block)
{
	bool shouldSleep = false;
	while (running_) {
		if (shouldSleep) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		std::lock_guard<std::mutex> lg(mutex_);
		if (!pcms_.empty()) {
			auto p = pcms_.front();
			pcms_.pop_front();
			return p;
		} else if (block) {
			shouldSleep = true;
			continue;
		} else {
			break;
		}		
	}
	return pcm();
}

