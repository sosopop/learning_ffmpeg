#pragma once

#include "xdecoderthread.h"
#include "IVideoCall.h"
#include <atomic>

struct AVPacket;
struct AVCodecParameters;
class xdecoder;

class xvideodecoderthread : public xdecoderthread
{
public:
	xvideodecoderthread();
	virtual ~xvideodecoderthread();

	//! 开始解码。总是释放param
	bool open(AVCodecParameters* param, IVideoCall* call, int width, int height);

	void setSyncPts(int64_t pts) { syncPts_ = pts; }

	//! 当pkt->pts >= seekPts时解码并显示画面
	bool repaintPts(AVPacket* pkt, int64_t seekPts);

	virtual void run() override;

protected:
	IVideoCall* call_{ nullptr };
	std::mutex vmutex_{};
	std::atomic<int64_t> syncPts_{ 0 };
};
