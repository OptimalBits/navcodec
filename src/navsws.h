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
#ifndef _NAVSWS_H
#define _NAVSWS_H

#include "navcodec.h"

#include <v8.h>
#include <node.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

using namespace v8;

class NAVSws : node::ObjectWrap {
private:
  struct SwsContext *pContext;
  
  Persistent<Object> frame;
  AVFrame *pFrame;
  uint8_t *pFrameBuffer;
  bool passthrough;
  
  int width;
  int height;
  PixelFormat pix_fmt;
  
public:
  NAVSws();
  ~NAVSws();
  
  static Persistent<FunctionTemplate> templ;
  
  static void Init(Handle<Object> target);
  
  // (srcStream, dstStream)
  static Handle<Value> New(const Arguments& args);
    
  // (srcFrame) -> dstFrame
  static Handle<Value> Convert(const Arguments& args);
};

#endif // _NAVSWS_H
