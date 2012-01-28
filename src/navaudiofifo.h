
#ifndef _NAVAUDIOFIFO_H
#define _NAVAUDIOFIFO_H

extern "C" {
#include <libavutil/fifo.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#define FIFO_SIZE 32*1024

class NAVAudioFifo  {
private:
  AVFrame *pOutFrame;
  AVFifoBuffer *pFifo;
  uint8_t *pFrameData;
  
  int outputSampleSize;
  int capacity;
  int frameBytesSize;
  
  int resetFrame(int size);
  
public:
  AVCodecContext *pEncoder;

  NAVAudioFifo(AVCodecContext *pEncoder);  
  ~NAVAudioFifo();
  
  int put(AVFrame *pFrame);
  
  AVFrame *get();
  
  AVFrame *getLast();
  
  bool moreFrames();
  bool dataLeft();
};

#endif // _NAVAUDIOFIFO_H
