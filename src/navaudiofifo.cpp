
#include "navaudiofifo.h"

#define FIFO_SIZE 32*1024

int NAVAudioFifo::resetFrame(int size){
  if (pOutFrame->extended_data != pOutFrame->data){
    av_freep(&(pOutFrame->extended_data));
  }
  
  avcodec_get_frame_defaults(pOutFrame);
  pOutFrame->quality = 1;
  
  pOutFrame->format = pEncoder->sample_fmt;
  pOutFrame->owner = pEncoder;
  
  pOutFrame->nb_samples = size / (pEncoder->channels * outputSampleSize);
  
  // Hack! but avoids certain assertions...
  pEncoder->frame_size = pOutFrame->nb_samples;
  
  return avcodec_fill_audio_frame(pOutFrame, 
                                  pEncoder->channels, 
                                  pEncoder->sample_fmt,
                                  pFrameData, 
                                  size, 
                                  1);
}
  
NAVAudioFifo::NAVAudioFifo(AVCodecContext *pEncoder){
  capacity = FIFO_SIZE;
  pFifo = av_fifo_alloc(capacity);
    
  this->pEncoder = pEncoder;
    
  outputSampleSize = av_get_bytes_per_sample(pEncoder->sample_fmt);
    
  frameBytesSize = pEncoder->frame_size * pEncoder->channels * outputSampleSize;
    
  pOutFrame = avcodec_alloc_frame();
  pOutFrame->extended_data = NULL;
    
  pFrameData = (uint8_t*) av_malloc(frameBytesSize);
}
  
NAVAudioFifo::~NAVAudioFifo(){
  printf("Called NAVAudioFifo destructor");

  av_fifo_free(pFifo);
  
  if (pOutFrame->extended_data != pOutFrame->data){
    av_free(pOutFrame->extended_data);
  }
  
  av_free(pOutFrame);
  av_free(pFrameData);
}
  
int NAVAudioFifo::put(AVFrame *pFrame){
  int ret;
  
  AVCodecContext *pDecoder = pFrame->owner;
  
  int sampleSize = av_get_bytes_per_sample(pDecoder->sample_fmt);
  
  int dataSize = pFrame->nb_samples * pDecoder->channels * sampleSize;
  int space = av_fifo_space(pFifo);
    
  if(space < dataSize){
    ret = av_fifo_realloc2(pFifo, capacity+(dataSize-space));
    if(ret<0){
      return ret;
    }
  }
    
  ret = av_fifo_generic_write(pFifo, pFrame->data[0], dataSize, NULL);
  if(ret<dataSize){
    return -1;
  }
  /*
  printf("IN FIFO:%i\n", av_fifo_size(pFifo));
  printf("NUM SAMPLES:%i\n", pFrame->nb_samples);
  printf("SAMPLE SIZE:%i\n", sampleSize);
  printf("DATA SIZE:%i\n", dataSize);
  */
  return 0;
}
  
AVFrame *NAVAudioFifo::get(){  
  if(moreFrames()){    
    av_fifo_generic_read(pFifo, pFrameData, frameBytesSize, NULL);
    
    int ret = resetFrame(frameBytesSize);
    if(ret<0){
      return NULL; 
    }
      
    return pOutFrame;
  }else {
    return NULL;
  }
}

AVFrame *NAVAudioFifo::getLast(){
  int bytesLeft = av_fifo_size(pFifo);
  av_fifo_generic_read(pFifo, pFrameData, bytesLeft, NULL);
  
  int ret = resetFrame(bytesLeft);
  if(ret<0){
    return NULL; 
  }
  
  return pOutFrame;
}
  
bool NAVAudioFifo::moreFrames(){
  if(av_fifo_size(pFifo) >= frameBytesSize){
    return true;
  }else{
    return false;
  }
}

bool NAVAudioFifo::dataLeft(){
  if(av_fifo_size(pFifo) > 0){
    return true;
  }else{
    return false;
  }
}


