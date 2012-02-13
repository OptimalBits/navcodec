
native = require('../build/Release/navcodec.node');
fs = require('fs');

function open(input, cb){
  fs.stat(input, function(err, stats){      
    if(err){
      cb && cb(err);
      return;
    }
    // TODO: this function will be asynchronous in the future.
    var inputFormat = new native.NAVFormat(input);
    var media = new Media(inputFormat, stats.size);
  
    cb(null, media);
  });
}

function stringToCodec(str){
  var codecId = native.CodecId['CODEC_ID_'+str.toUpperCase()];
  if(!codecId){
    throw new Error("Invalid codec id");
  }
  return codecId;
}

function gcd(x, y) {
	while (y != 0) {
		var z = x % y;
		x = y;
		y = z;
	}
	return x;
}

function fitKeepingRatio (aspectRatio, newSize) {
  var dstSize = {}
  if (aspectRatio > newSize.w/newSize.h) {
    dstSize.w = newSize.w
    dstSize.h = Math.round(newSize.w * (1/aspectRatio))
  } else {
    dstSize.w = Math.round(newSize.h * aspectRatio)
    dstSize.h = newSize.h
  }
  if (dstSize.w % 2 == 1) {
    dstSize.w -= 1;
  }
  if (dstSize.h % 2 == 1) {
    dstSize.h -= 1;
  }
  return dstSize
}

var Media = function(inputFormat, fileSize){
  var streams = inputFormat.streams,
      videoStreams = [],
      audioStreams = [];

  // Find video and audio streams
  for(var i=0,len=streams.length;i<len;i++){  
    if(streams[i].codec.codec_type === 'Video') {
      videoStreams.push(streams[i]);
    } else if(streams[i].codec.codec_type === 'Audio') {
      audioStreams.push(streams[i]);
    }
  }
  
  this.inputFormat = inputFormat;
  this.metadata = inputFormat.metadata;
  this.fileSize = fileSize;
  this.videoStreams = videoStreams;
  this.audioStreams = audioStreams;
  
  this.width = this.videoStreams[0] && this.videoStreams[0].codec.width;
  this.height = this.videoStreams[0] && this.videoStreams[0].codec.height;
  this.videoBitrate = this._bitrate(videoStreams);
  this.audioBitrate = this._bitrate(audioStreams);
  this.bitrate = this.audioBitrate + this.videoBitrate;
  this.samplerate = this.audioStreams[0] && this.audioStreams[0].codec.sample_rate;
  
  this._outputs = [];
}

Media.prototype.info = function(){
  this.inputFormat.dump();
}

Media.prototype.addOutput = function(filename, options){
  this._outputs.push({filename:filename, options:options});
}

Media.prototype.transcode = function(cb){
  var inputVideo = null, 
      inputAudio = null,
      outputs = this._outputs,
      numOutputs = outputs.length,
      streams,
      counter,
      time,
      time_base,
      fileSize,
      output,
      codecType,
      hasVideo = false,
      hasAudio = false;

  time = Date.now();    
  
  inputVideo = this.videoStreams[0];
  inputAudio = this.audioStreams[0];
  
  if(inputVideo){
    time_base = inputVideo.codec.time_base;
    time_base.num *= inputVideo.codec.ticks_per_frame;
    
    while(time_base.den > 65535){
      time_base.den /= 2;
      time_base.num /= 2;
    }
  
    time_base.num = Math.round(time_base.num);
    time_base.den = Math.round(time_base.den);
    
    var d = gcd(time_base.num, time_base.den);
    if(d>1){
      time_base.num = time_base.num/d;
      time_base.den = time_base.den/d;
    }
  }
    
  for(var i=0;i<numOutputs;i++){
    var outputFormat = new native.NAVOutputFormat(outputs[i].filename),
      options = outputs[i].options;
    
    outputs[i].outputFormat = outputFormat;

    if(inputVideo){
      if(!options.skipVideo){
        var settings = {
          time_base: '('+time_base.num+'/'+time_base.den+')',
          ticks_per_frame: 1,
          pix_fmt:inputVideo.codec.pix_fmt,
          width:inputVideo.codec.width,
          height:inputVideo.codec.height,
          b:inputVideo.codec.bit_rate,
        //  g:12, // gop size
          pix_fmt:native.PixelFormat.PIX_FMT_YUV420P,
          keepAspect:true
        };
  
        options.width && (settings.width = options.width);
        options.height && (settings.height = options.height);
        options.videoBitrate && (settings.b = options.videoBitrate);
        options.videoCodec && (settings.codec = stringToCodec(options.videoCodec));
        options.keepAspect && (settings.keepAspect = options.keepAspect);
  
        if(settings.keepAspect){
          var dims = fitKeepingRatio(inputVideo.codec.width/inputVideo.codec.height, 
                                   {w:options.width,h:options.height});
          settings.width = dims.w;
          settings.height = dims.h;
        }
          
        outputs[i].outputVideo = outputFormat.addStream("Video", settings);
        outputs[i].converter = new native.NAVSws(inputVideo, outputs[i].outputVideo);
        hasVideo = true;
      }
    }

    if(inputAudio){
      if(!options.skipAudio){
          var settings = {      
          b:inputAudio.codec.bit_rate,
          ar:inputAudio.codec.sample_rate,
          ac:2,
          channels:inputAudio.codec.channels
        };
  
        settings.b = options.audioBitrate || 128000;
        options.audioCodec && (settings.codec = stringToCodec(options.audioCodec));
        options.sampleRate && (settings.ar = options.sampleRate);
        options.channels && (settings.ac = options.channels);

        outputs[i].outputAudio = outputFormat.addStream("Audio", settings);
        outputs[i].resampler = new native.NAVResample(inputAudio, outputs[i].outputAudio);
        hasAudio = true;
      }
    }

    if(outputs[i].outputAudio || outputs[i].outputVideo){
      outputFormat.begin();
    }
  }
  
  streams = [];
  hasAudio && inputAudio && streams.push(inputAudio);
  hasVideo && inputVideo && streams.push(inputVideo);

  fileSize = this.fileSize;
  counter = 0;
  
  this.inputFormat.decode(streams, function(stream, frame, pos){
    if(frame){
      counter++;
      if(counter%200 === 0){
        pos && cb && cb(null, ((100*pos) / fileSize), Date.now()-time);
      }
      codecType = stream.codec.codec_type;
      for(var i=0;i<numOutputs;i++){
        output = outputs[i];
      
        if (output.outputVideo && (codecType === output.outputVideo.codec.codec_type)){
          output.outputFormat.encode(output.outputVideo, output.converter.convert(frame));
        }   
        if (output.outputAudio && (codecType === output.outputAudio.codec.codec_type)){
          output.outputFormat.encode(output.outputAudio, output.resampler.convert(frame));
        }
      }
    }else{
      // No more frames, end encoding.
      for(var i=0;i<numOutputs;i++){
        if(outputs[i].outputAudio || outputs[i].outputVideo){
          outputs[i].outputFormat.end();
          outputs[i].outputFormat = null;
          outputs[i].converter = null;
          outputs[i].resampler = null;
        }
      }
      cb && cb(null, 100, Date.now()-time);
    }
  });
}

Media.prototype._bitrate = function(streams){
  var bitrate = 0;
  for(var i=0,len=streams.length;i<len;i++){
    bitrate += streams[i].codec.bit_rate;
  }
  return bitrate;
}
           
module.exports.open = open;
module.exports.CodecId = native.CodecId;
