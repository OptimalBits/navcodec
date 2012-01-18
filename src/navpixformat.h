
#ifndef _NAV_PIX_FORMAT_H
#define _NAV_PIX_FORMAT_H

#include "avstream.h"

#include <v8.h>
#include <node.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

using namespace v8;

Handle<Object> CreatePixelFormatsEnum();


#endif // _NAV_PIX_FORMATS_H
