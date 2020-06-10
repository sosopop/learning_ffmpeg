#/bin/bash

mkdir -p build/linux
cd build/linux
clang++ -o 006.record_pcm ../../006.record_pcm/006.record_pcm.cpp -I/usr/local/ffmpeg/include -L/usr/local/ffmpeg/lib -lavcodec -lavdevice -lavformat -lavutil 
./006.record_pcm alsa hw:0 006.out.pcm
