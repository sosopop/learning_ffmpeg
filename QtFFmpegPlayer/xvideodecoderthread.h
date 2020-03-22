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

	//! ��ʼ���롣�����ͷ�param
	bool open(AVCodecParameters* param, IVideoCall* call, int width, int height);

	void setSyncPts(int64_t pts) { syncPts_ = pts; }

	//! ��pkt->pts >= seekPtsʱ���벢��ʾ����
	bool repaintPts(AVPacket* pkt, int64_t seekPts);

	virtual void run() override;

protected:
	IVideoCall* call_{ nullptr };
	std::mutex vmutex_{};
	std::atomic<int64_t> syncPts_{ 0 };
};