#pragma once

#include "../desktop_recorder.h"
#include <QDebug>

static void test_desktop_record()
{
	auto rec = desktop_recorder::getInstance();
	if (!rec->start("test_desktop_record.mp4", 10, 1920, 1080)) {
		qDebug() << "start failed";
		return;
	}

	std::this_thread::sleep_for(std::chrono::seconds(15));

	rec->stop();
}
