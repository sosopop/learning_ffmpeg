#include "../ffmpeg.h"
#include "../sdl.h"
#include <assert.h>

static constexpr int SDL_AUDIO_BUFFER_SIZE = 1024;
static constexpr int MAX_AUDIO_FRAME_SIZE = 19200;

struct PacketQ {
	AVPacketList* head = nullptr;
	AVPacketList* tail = nullptr;
	int nb_packets = 0;
	int size = 0;
	SDL_mutex* mutex = nullptr;
	SDL_cond* cond = nullptr;
	int quit = 0;

	void init() {
		mutex = SDL_CreateMutex();
		cond = SDL_CreateCond();
	}

	int put(AVPacket* pktSrc) {
		AVPacket pktCopy;
		if (av_packet_ref(&pktCopy, pktSrc) < 0) {
			fprintf(stderr, "Cound not duplicate AVPacket\n");
			return -1;
		}

		AVPacket* pkt = &pktCopy;

		AVPacketList* pktList = (AVPacketList*)av_malloc(sizeof(AVPacketList));
		pktList->pkt = *pkt;
		pktList->next = nullptr;

		SDL_LockMutex(mutex);

		if (!tail) {
			head = pktList;
		} else {
			tail->next = pktList;
		}

		tail = pktList;
		nb_packets++;
		size += pktList->pkt.size;

		SDL_CondSignal(cond);
		SDL_UnlockMutex(mutex);

		return 0;
	}

	int get(AVPacket* pkt, int block) {
		int ret = -1;
		SDL_LockMutex(mutex);

		while (1) {
			if (quit) { ret = -1; break; }
			AVPacketList* pktList = head;
			if (pktList) {
				head = pktList->next;
				if (!head) {
					tail = nullptr;
				}
				nb_packets--;
				size -= pktList->pkt.size;
				*pkt = pktList->pkt;
				av_free(pktList);
				ret = 1;
				break;
			} else if (!block) {
				ret = 0;
				break;
			} else {
				SDL_CondWait(cond, mutex);
			}
		}

		SDL_UnlockMutex(mutex);
		return ret;
	}
} audioQ{};

int audioDecodeFrame(AVCodecContext* aCodecContext, uint8_t* audioBuff, int buffSize)
{
	static AVPacket pkt;
	static AVFrame frame;
	static uint8_t* audioPktData = nullptr;
	static int audioPktSize = 0;

	int len1 = 0, dataSize = 0;
	while (1) {
		while (audioPktSize > 0) {
			int gotFrame = 0;
			len1 = avcodec_decode_audio4(aCodecContext, &frame, &gotFrame, &pkt);
			if (len1 < 0) {
				// error, skip frame
				audioPktSize = 0;
				break;
			}

			audioPktData += len1;
			audioPktSize -= len1;
			dataSize = 0;
			if (gotFrame) {
				dataSize = av_samples_get_buffer_size(nullptr,
													  frame.channels,
													  frame.nb_samples,
													  aCodecContext->sample_fmt,
													  1);
				assert(dataSize <= buffSize);
				memcpy(audioBuff, frame.data[0], dataSize);
			}

			if (dataSize <= 0) {
				// no data yet, get more frames
				continue;
			}

			return dataSize;
		}

		if (pkt.data) {
			av_packet_unref(&pkt);
		}

		if (audioQ.quit) {
			return -1;
		}

		if (audioQ.get(&pkt, 1) < 0) {
			return -1;
		}

		audioPktData = pkt.data;
		audioPktSize = pkt.size;
	}
}

void audio_callback(void* userdata, Uint8* stream, int len)
{
	static uint8_t audioBuff[(MAX_AUDIO_FRAME_SIZE * 3) / 2];
	static unsigned int audioBuffSize = 0;
	static unsigned int audioBuffIdx = 0;

	AVCodecContext* aCodecContext = (AVCodecContext*)userdata;
	while (len > 0) {
		if (audioBuffIdx >= audioBuffSize) {
			// We've already sent all our data; get more
			int audioSize = audioDecodeFrame(aCodecContext, audioBuff, sizeof(audioBuff));
			if (audioSize < 0) {
				// if error, output silence
				audioBuffSize = 1024;
				memset(audioBuff, 0, audioBuffSize);
			} else {
				audioBuffSize = audioSize;
			}
			audioBuffIdx = 0;
		}

		int len1 = audioBuffSize - audioBuffIdx;
		if (len1 > len) {
			len1 = len;
		}
		memcpy(stream, (const uint8_t*)audioBuff + audioBuffIdx, len1);
		len -= len1;
		stream += len1;
		audioBuffIdx += len1;
	}
}

int main()
{
	const char* file_path = R"(Z:\BodyCombat20171007200236.mp4)";

	//av_register_all();

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
		exit(1);
	}

	// open video file
	AVFormatContext* fmtContext = nullptr;
	if (avformat_open_input(&fmtContext, file_path, nullptr, nullptr) != 0) {
		fprintf(stderr, "Could not open source file %s\n", file_path);
		exit(1);
	}

	// retrieve stream information
	if (avformat_find_stream_info(fmtContext, nullptr) < 0) {
		fprintf(stderr, "Could not find stream information\n");
		exit(1);
	}

	int videoStream = -1, audioStream = -1;
	AVCodecContext* codecContext = openCodecContext(fmtContext, AVMEDIA_TYPE_VIDEO, videoStream);
	if (!codecContext) {
		fprintf(stderr, "Could not find video stream\n");
		exit(1);
	}
	AVCodecContext* aCodecContext = openCodecContext(fmtContext, AVMEDIA_TYPE_AUDIO, audioStream);
	if (!aCodecContext) {
		fprintf(stderr, "Could not find audio stream\n");
		exit(1);
	}

	av_dump_format(fmtContext, 0, file_path, 0);

	// set audio settings from codec info
	SDL_AudioSpec wantedSpec, spec;
	wantedSpec.freq = aCodecContext->sample_rate;
	wantedSpec.format = AUDIO_S16SYS;
	wantedSpec.channels = aCodecContext->channels;
	wantedSpec.silence = 0;
	wantedSpec.samples = SDL_AUDIO_BUFFER_SIZE;
	wantedSpec.callback = audio_callback;
	wantedSpec.userdata = aCodecContext;

	if (SDL_OpenAudio(&wantedSpec, &spec) < 0) {
		fprintf(stderr, "SDL_OpenAudio failed: %s\n", SDL_GetError());
		exit(1);
	}

	audioQ.init();
	SDL_PauseAudio(0);

	// allocate video frame
	AVFrame* frame = av_frame_alloc();
	AVFrame* frameYUV = av_frame_alloc();

	// determine required buffer size and allocate buffer
	int nbytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, codecContext->width, codecContext->height, 1);
	uint8_t* buffer = (uint8_t*)av_malloc(nbytes * sizeof(uint8_t));

	// assign appropriate parts of buffer to image planes in frameYUV
	// Note that frameYUV is an AVFrame, but AVFrame is a superset of AVPicture
	av_image_fill_arrays(frameYUV->data, frameYUV->linesize, buffer, AV_PIX_FMT_YUV420P, codecContext->width, codecContext->height, 1);

	// initialize SWS context for software scaling
	SwsContext* swsContext = sws_getContext(codecContext->width,
											codecContext->height,
											codecContext->pix_fmt,
											codecContext->width,
											codecContext->height,
											AV_PIX_FMT_YUV420P,
											SWS_BICUBIC,
											nullptr,
											nullptr,
											nullptr);

	SDL_Window* screen = SDL_CreateWindow("outputing to screen",
										  SDL_WINDOWPOS_UNDEFINED,
										  SDL_WINDOWPOS_UNDEFINED,
										  codecContext->width,
										  codecContext->height,
										  SDL_WINDOW_OPENGL);
	if (!screen) {
		fprintf(stderr, "SDL: could not create window - exiting:%s\n", SDL_GetError());
		exit(1);
	}

	SDL_Renderer* renderer = SDL_CreateRenderer(screen, -1, 0);
	SDL_Texture* texture = SDL_CreateTexture(renderer,
											 SDL_PIXELFORMAT_IYUV,
											 SDL_TEXTUREACCESS_STREAMING,
											 codecContext->width,
											 codecContext->height);

	SDL_Rect rect;
	rect.x = rect.y = 0;
	rect.w = codecContext->width;
	rect.h = codecContext->height;

	auto decodePacket = [codecContext, swsContext, frame, frameYUV, renderer, texture, &rect](AVPacket* packet) {
		// decode video frame
		int ret = avcodec_send_packet(codecContext, packet);
		if (ret < 0) {
			fprintfAVErrorString(ret, "Error while sending a packet to the decoder");
			return ret;
		}

		while (ret >= 0) {
			ret = avcodec_receive_frame(codecContext, frame);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
				break;
			} else if (ret < 0) {
				fprintfAVErrorString(ret, "Error while receiving a frame from the decoder");
				break;
			}

			if (ret >= 0) {
				// convert the image from its native format to YUV
				sws_scale(swsContext,
						  frame->data, frame->linesize,
						  0, codecContext->height,
						  frameYUV->data, frameYUV->linesize);

				SDL_UpdateYUVTexture(texture, &rect,
									 frameYUV->data[0], frameYUV->linesize[0],
									 frameYUV->data[1], frameYUV->linesize[1],
									 frameYUV->data[2], frameYUV->linesize[2]);
				SDL_RenderClear(renderer);
				SDL_RenderCopy(renderer, texture, nullptr, &rect);
				SDL_RenderPresent(renderer);
				SDL_Delay(33);
				//av_frame_unref(frame);
			}
		}

		return 0;
	};

	AVPacket packet;
	SDL_Event ev;
	while (av_read_frame(fmtContext, &packet) >= 0) {
		if (packet.stream_index == videoStream) {
			if (decodePacket(&packet) < 0) {
				break;
			}
			av_packet_unref(&packet); // or av_free_packet
		} else if (packet.stream_index == audioStream) {
			audioQ.put(&packet);
		} else {
			av_packet_unref(&packet); // or av_free_packet
		}
		if (SDL_PollEvent(&ev) && ev.type == SDL_QUIT) {
			audioQ.quit = 1;
			SDL_CondSignal(audioQ.cond);
			break;
		}
	}

	// flush cached frames
	decodePacket(nullptr);

	SDL_Quit();

	av_free(buffer);
	av_frame_free(&frameYUV);
	av_frame_free(&frame);
	avcodec_free_context(&codecContext);
	avcodec_free_context(&aCodecContext);
	avformat_close_input(&fmtContext);
}