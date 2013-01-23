/* Copyright(c) 2012 Optimal Bits Sweden AB. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef _NAV_OUTPUT_FORMAT_H
#define _NAV_OUTPUT_FORMAT_H

#include "navstream.h"
#include "navaudiofifo.h"

#include <v8.h>
#include <node.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
}

using namespace v8;

class NAVOutputFormat : node::ObjectWrap {
private:
  AVOutputFormat *pOutputFormat;

  char *filename;
  
  uint8_t *pVideoBuffer;
  int videoBufferSize;

  uint8_t *pAudioBuffer;
  int audioBufferSize;
  AVStream *pAudioStream;
  
  int64_t videoFrame;

public:
  NAVOutputFormat();
  ~NAVOutputFormat();
  
  // --
  AVFormatContext *pFormatCtx;
  NAVAudioFifo *pFifo;
  
  int outputAudio(AVFormatContext *pFormatContext,
                  AVStream *pStream,
                  AVFrame *pFrame);
  // --
  const char *EncodeVideoFrame(AVStream *pStream,
                               AVFrame *pFrame,
                               int *outSize);
  
  static Persistent<FunctionTemplate> templ;
  
  static void Init(Handle<Object> target);
  
  static Handle<Value> New(const Arguments& args);
  
  static Handle<Value> New(AVFormatContext *pContext);
  
  // (stream_type::String, [codecId::String])
  static Handle<Value> AddStream(const Arguments& args);
  
  // ()
  static Handle<Value> Begin(const Arguments& args);
  
  // (stream::AVStream, frame::AVFrame)
  static Handle<Value> Encode(const Arguments& args);

  // ()
  static Handle<Value> End(const Arguments& args);

};

#endif // _NAV_OUTPUT_FORMAT_H
