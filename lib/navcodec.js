
var native = require('../build/Release/navcodec.node'),
    fs = require('fs'),
    _ = require('underscore'),
    async = require('async');

function open(input, cb) {
  fs.stat(input, function(err, stats){      
    if(err){
      cb && cb(err);
      return;
    }
    // TODO: this function will be asynchronous in the future.
    var inputFormat = new native.NAVFormat(input),
      media = new Media(inputFormat, stats.size);
  
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
	while (y !== 0) {
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
  this.duration = inputFormat.duration;
  
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
  this._thumbnails = [];
}

Media.prototype.info = function(){
  this.inputFormat.dump();
  return this;
}

Media.prototype.addOutput = function(filename, options){
  this._outputs.push({filename:filename, options:options});
  return this;
}

Media.prototype.thumbnail = function(filenames, options){
  this._thumbnails.push({filenames:filenames, options:options});
  return this;
}

Media.prototype.transcode = function(cb){
  var inputVideo = null, 
      inputAudio = null,
      outputs = this._outputs,
      numOutputs = outputs.length,
      thumbnails = this._thumbnails,
      numThumbnails = thumbnails.length,
      streams,
      counter,
      time,
      time_base,
      fileSize,
      output,
      pts,
      codecType,
      hasVideo = false,
      hasAudio = false;

  time = Date.now();    
  
  inputVideo = this.videoStreams[0];
  inputAudio = this.audioStreams[0];
  
  if(inputVideo){
    if(inputVideo.frameRate){
      time_base = {};
      time_base.num = inputVideo.frameRate.den;
      time_base.den = inputVideo.frameRate.num;
    } else {
      time_base = inputVideo.codec.time_base;
      time_base.num *= inputVideo.codec.ticks_per_frame;
    }
    
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
  
  for(var i=0;i<numThumbnails;i++){
    if(inputVideo){
      var 
        options = _.defaults(thumbnails[i].options,{
          pix_fmt:native.PixelFormat.PIX_FMT_YUVJ420P,
          width:64,
          height:64,
          bit_rate:1000});
        
      if(options.keepAspectRatio){
        var dims = fitKeepingRatio(inputVideo.codec.width/inputVideo.codec.height, 
                                   {w:options.width,h:options.height});
        options.width = dims.w;
        options.height = dims.h;
      }
                
      thumbnails[i].encoder = new native.NAVThumbnail(options);
      thumbnails[i].converter = new native.NAVSws(inputVideo, options);
    }
  }
  
  for(var i=0;i<numOutputs;i++){
    var outputFormat = new native.NAVOutputFormat(outputs[i].filename),
      options = outputs[i].options;
    
    outputs[i].outputFormat = outputFormat;

    if(inputVideo){
      if(!options.skipVideo){
        var settings = _.defaults(options, {
          time_base: '('+time_base.num+'/'+time_base.den+')',
          ticks_per_frame: 1,
          width: inputVideo.codec.width,
          height: inputVideo.codec.height,
          b: options.videoBitrate || inputVideo.codec.bit_rate,
          //g:12, // gop size
          keepAspect: true,
          skipVideoFrames: 0,
        });
  
        options.videoCodec && (settings.codec = stringToCodec(options.videoCodec));
        
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

  this.inputFormat.decode(streams, function(stream, frame, pos, notifier){
    if(frame){
      counter++;
      pts = counter * time_base.num / time_base.den;
      
      if(counter % 200 === 0){
        pos && cb && cb(null, ((100*pos) / fileSize), Date.now()-time);
      }
      codecType = stream.codec.codec_type;

      async.forEach(outputs, function(output, done){      
        if (output.outputVideo && (codecType === output.outputVideo.codec.codec_type)){
          output.outputFormat.encode(output.outputVideo, output.converter.convert(frame), function(){
            done();
          });
        } else if (output.outputAudio && (codecType === output.outputAudio.codec.codec_type)){
          output.outputFormat.encode(output.outputAudio, output.resampler.convert(frame), function(){
            done();
          });
        } else{
//          console.log("Unsupported codec type: " +codecType);
          done();
        }
      }, function(err){
        if(codecType === 'Video'){
          for(var i=0;i<numThumbnails;i++){
            var filenames = thumbnails[i].filenames;
            for(var filename in filenames){
              if(filenames[filename] <= pts){
                var scaledFrame = thumbnails[i].converter.convert(frame);
                thumbnails[i].encoder.write(scaledFrame, filename, function(err){
                  if(err){
                    console.log("Error writting thumbnail!:"+err);
                  } // TODO: Handle error.
                });
                delete filenames[filename];
              }
            }
          }
        }
        notifier.done();
      });
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
module.exports.relocateMoov = native.relocateMoov;
module.exports.CodecId = native.CodecId;
