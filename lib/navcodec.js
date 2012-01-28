native = require('../build/Release/navcodec.node');
fs = require('fs');

function open(input, cb){
  fs.stat(input, function(err, stats){      
    if(err){
      cb && cb(err);
      return;
    }

    // TODO: this function will be async in the future.
    var inputFormat = new native.AVFormat(input);
  
    var media = new Media(inputFormat, stats.size);
  
    cb(null, media);
  });
}

function stringToCodec(str){
  var codecId = native.CodecId['CODEC_ID_'+str.toUpperCase()];
  return codecId;
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
  this.fileSize = fileSize;
  this.videoStreams = videoStreams;
  this.audioStreams = audioStreams;
}

Media.prototype.width = function(){
  if(this.videoStreams[0]){
    return this.videoStreams[0].codec.width;
  }else{
    return 0; 
  }
}

Media.prototype.height = function(){
  if(this.videoStreams[0]){
    return this.videoStreams[0].codec.height;
  }else{
    return 0; 
  }
}

Media.prototype._bitrate = function(streams){
  var bitrate = 0;
  for(var i=0,len=streams.length;i<len;i++){
    bitrate += streams[i].codec.bit_rate;
  }
  return bitrate;
}

Media.prototype.videoBitrate = function(){
  return this._bitrate(this.videoStreams);
}

Media.prototype.audioBitrate = function(){
  return this._bitrate(this.audioStreams);
}

Media.prototype.bitrate = function(){
  return this.videoBitrate() + this.audioBitrate();
}

Media.prototype.samplerate = function(){
  if(this.audioStreams[0]){
    return this.audioStreams[0].codec.sample_rate;
  }else{
    return 0;
  }
}

Media.prototype.info = function(){
  this.inputFormat.dump();
}

Media.prototype.transcode = function(output, options, cb){
  var inputVideo = null, 
      inputAudio = null,
      outputVideo = null,
      outputAudio = null, 
      streams,
      inputFormat,
      outputFormat,
      counter,
      time,
      time_base,
      fileSize;
      
  inputVideo = this.videoStreams[0];
  inputAudio = this.audioStreams[0];
  
  counter = 0;
  time = Date.now();

  // Create Output Video
  outputFormat = new native.NAVOutputFormat(output)

  if(inputVideo){
    time_base = inputVideo.codec.time_base;
    time_base.num *= inputVideo.codec.ticks_per_frame;
    
    while(time_base.den > 65535){
      time_base.den /= 2;
      time_base.num /= 2;
    }
  
    time_base.num = Math.round(time_base.num);
    time_base.den = Math.round(time_base.den);
  
    var settings = {
      time_base: time_base,
      ticks_per_frame: 1,
      pix_fmt:inputVideo.codec.pix_fmt,
      width:inputVideo.codec.width,
      height:inputVideo.codec.height,
      bit_rate:inputVideo.codec.bit_rate
    };
  
    options.width && (settings.width = options.width);
    options.height && (settings.height = options.height);
    options.videoBitrate && (settings.bit_rate = options.videoBitrate);
    options.videoCodec && (settings.codec = stringToCodec(options.videoCodec));
  
    console.log(settings);
  
    outputVideo = outputFormat.addStream("Video", settings);
    
    // Create a converter (Only usefull if changing dimensions 
    // and/or pixel format. Otherwise it will act as a passthrough filter.
    converter = new native.NAVSws(inputVideo, outputVideo);
  }

  if(inputAudio){
    var settings = {      
      bit_rate:inputAudio.codec.bit_rate,
      sample_rate:inputAudio.codec.sample_rate
    };
  
    settings.bit_rate = options.audioBitrate || 128000;
    options.audioCodec && (settings.codec = stringToCodec(options.audioCodec));
    options.sampleRate && (settings.sample_rate = options.sampleRate);
    options.channels && (settings.channels = options.channels);

    console.log(settings);
  
    outputAudio = outputFormat.addStream("Audio", settings);
    
    resampler = new native.NAVResample(inputAudio, outputAudio);
  }

  //
  // Start Encoder
  //
  outputFormat.begin();
  
  streams = [];
  inputAudio && streams.push(inputAudio);
  inputVideo && streams.push(inputVideo);

  fileSize = this.fileSize;
  this.inputFormat.decode(streams, function(stream, frame, pos){
    counter++;
    if(frame){
      if(counter%100 === 0){
        cb && cb(null, pos / fileSize);
      }
      if (outputVideo && (stream.codec.codec_type === outputVideo.codec.codec_type)){
        frame = converter.convert(frame);
        outputFormat.encode(outputVideo, frame);
      }   
      if (outputAudio && (stream.codec.codec_type === outputAudio.codec.codec_type)){
        frame = resampler.convert(frame);
        outputFormat.encode(outputAudio, frame);
      }  
    }else{
     // No more frames, end encoding.
      outputFormat.end();
      cb && cb(null, 1);
    }
  });

  console.log("Duration: "+(Date.now()-time));
  console.log(counter);
}
           
module.exports.open = open;
module.exports.CodecId = native.CodecId;
