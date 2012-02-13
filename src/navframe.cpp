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

#include "navframe.h"

using namespace v8;

Persistent<ObjectTemplate> NAVFrame::templ;

NAVFrame::NAVFrame(AVFrame *pFrame){
  pContext = pFrame;
}
  
NAVFrame::~NAVFrame(){
  printf("NAVFrame destructor\n");
  av_free(pContext);
}
  
void NAVFrame::Init(){
  HandleScope scope;
  
  Local<ObjectTemplate> templ = ObjectTemplate::New();
  templ = ObjectTemplate::New();
  templ->SetInternalFieldCount(1);
  
  NAVFrame::templ = Persistent<ObjectTemplate>::New(templ);
}
  
Handle<Object> NAVFrame::New(AVFrame *pFrame) {
  HandleScope scope;
  
  NAVFrame *instance = new NAVFrame(pFrame);

  Handle<Object> obj = NAVFrame::templ->NewInstance();
  instance->Wrap(obj);

  return scope.Close(obj);
}
