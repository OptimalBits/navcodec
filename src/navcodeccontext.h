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

#ifndef _NAVCODECCONTEXT_H
#define _NAVCODECCONTEXT_H

#include <v8.h>
#include <node.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

using namespace v8;

void NoopSet(Local<String> property, Local<Value> value, 
             const AccessorInfo& info);

class NAVCodecContext : node::ObjectWrap {
private:
  
public:
  AVCodecContext *pContext;
  
  NAVCodecContext(AVCodecContext *pContext);
  ~NAVCodecContext();
  
  static Persistent<ObjectTemplate> templ;
  
  static void Init();
  
  static Handle<Object> New(AVCodecContext *pContext);
  
  static Handle<Value> Open(const Arguments& args);
  
  static Handle<Value> GetType(Local<String> property, 
                               const AccessorInfo& info);
  
  static Handle<Value> GetWidth(Local<String> property, 
                                const AccessorInfo& info);
  
  static Handle<Value> GetHeight(Local<String> property, 
                                 const AccessorInfo& info);

  static Handle<Value> GetBitRate(Local<String> property, 
                                  const AccessorInfo& info);

  static Handle<Value> GetSampleFmt(Local<String> property, 
                                    const AccessorInfo& info);

  static Handle<Value> GetSampleRate(Local<String> property, 
                                    const AccessorInfo& info);

  static Handle<Value> GetChannels(Local<String> property, 
                                    const AccessorInfo& info);
  
  static Handle<Value> GetFramerate(Local<String> property, 
                                    const AccessorInfo& info);
};


#endif // _NAVCODECCONTEXT_H