
var 
  native = require('../build/Release/navcodec.node'),
  avconv = require('./avconv'),
  fs = require('fs'),
  _ = require('underscore'),
  async = require('async');

var THUMBNAIL_DEFAULTS = {
  pix_fmt:native.PixelFormat.PIX_FMT_YUVJ420P,
  width:64,
  height:64,
  bit_rate:1000
}

function open(input, cb) {
  fs.stat(input, function(err, stats){      
    if(err){
      cb && cb(err);
      return;
    }

    try{
      var 
        inputFormat = new native.NAVFormat(input),
        media = new Media(input, inputFormat, stats.size);
        cb(null, media);
    }catch(err){
      cb(err);
    }
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

function calculateVideoBitrate(options){
  var 
    width = options.width || 640,
    height = options.height || width,
    fps = 30;
    
    return width*height*fps*0.075;
}

function fitKeepingRatio (aspectRatio, newSize) {
  var dstSize = {}
  if (aspectRatio > newSize.width /newSize.heigth) {
    dstSize.width = newSize.width
    dstSize.height = Math.round(newSize.width * (1/aspectRatio))
  } else {
    dstSize.width = Math.round(newSize.height * aspectRatio)
    dstSize.height = newSize.height
  }
  if (dstSize.width % 2 == 1) {
    dstSize.width -= 1;
  }
  if (dstSize.height % 2 == 1) {
    dstSize.height -= 1;
  }
  return dstSize
}

var Media = function(inputFile, inputFormat, fileSize){
  var streams = inputFormat.streams || [],
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
  
  this.inputFile = inputFile;
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
  options = options || {};
  this._outputs.push({filename:filename, options:options});
  return this;
}

Media.prototype.thumbnail = function(filenames, options){
  this._thumbnails.push({filenames:filenames, options:options});
  return this;
}

Media.prototype.addPyramid = function(filename, maxSize, minSize, options){
  var
    inputVideo = this.videoStreams[0],
    outputs = [];
  
  options = options || options;
  maxSize.width = maxSize.width || maxSize.height;
  minSize.width = minSize.width || minSize.height;

  if(inputVideo){
    var
      aspectRatio = inputVideo.codec.width / inputVideo.codec.height,
      size = maxSize,
      bitrate;
      
    if(options.videoBitrate){
      if(_.isString(options.videoBitrate)){
        var match = options.videoBitrate.match(/([0-9]+)k/);
        if(match){
          bitrate = parseFloat(match[1]) * 1000;
        }else{
          bitrate = options.videoBitrate;
        }
      }
    } else {
      bitrate = calculateVideoBitrate(options);
    }
  
    while(size.width >= minSize.width || size.height >= minSize.height){
      var 
        output = {options: _.clone(options)};
      
        if(options.keepAspectRatio){
          var aspectRatioSize = fitKeepingRatio(aspectRatio, size);
          _.extend(output.options, aspectRatioSize);
        }else{
          _.extend(output.options, size);
        }
        
        output.options.videoBitrate = bitrate;    
        output.filename = output.options.height + '-' + filename;
        
        bitrate = bitrate / 4;
        size.width = size.width / 2;
        size.height = size.height / 2;
        
        outputs.push(output);
    }
  }
  
  var _this = this;
  _.each(outputs, function(output){
    _this.addOutput(output.filename, output.options);
  })
  
  return outputs;
}

Media.prototype.transcode = function(cb){
  var inputVideo = null, 
      inputAudio = null,
      outputs = this._outputs,
      numOutputs = outputs.length,
      thumbnails = this._thumbnails,
      numThumbnails = thumbnails.length,
      hasVideo = false,
      hasAudio = false;

  time = Date.now();
  
  inputVideo = this.videoStreams[0];
  inputAudio = this.audioStreams[0];
  
  var numFrames;
  if(inputVideo){
    numFrames = inputVideo.numFrames;
  }
  if(inputAudio){
    numFrames = numFrames || inputAudio.numFrames;
  }
  console.log(inputVideo)
  
  if(!numFrames){
    numFrames = 
      (this.duration * inputVideo.frameRate.num) / inputVideo.frameRate.den;
  }
  
  numFrames = numFrames || 1000;
  
  var outputFiles = {};
  for(var i=0; i<numOutputs; i++){
    var opts = outputs[i].options;
    if(opts.keepAspectRatio){
      var dims = fitKeepingRatio(inputVideo.codec.width / 
                                 inputVideo.codec.height, opts);  
      opts.width = dims.width;
      opts.height = dims.height;
    }
    
    // Force dimensions to multiple of 16 x multiple of 2
    opts.width -= opts.width % 16;
    opts.height-= opts.height % 2;
    
    outputFiles[outputs[i].filename] = convertOptions(opts)
  }
  
  if(inputVideo){
    for(var i=0;i<numThumbnails;i++){
      var options = _.defaults(thumbnails[i].options, THUMBNAIL_DEFAULTS);
        
      if(options.keepAspectRatio){
        var dims = fitKeepingRatio(inputVideo.codec.width/inputVideo.codec.height, 
                                   options);
        options.width = dims.width;
        options.height = dims.height;
      }
      
      var filenames = thumbnails[i].filenames;
      for(var filename in filenames){
        var thumbOpts = _.clone(options);
        thumbOpts.offset = filenames[filename];
        outputFiles[filename] = convertOptions(thumbOpts)
        outputFiles[filename].vframes = 1;
      }
    }
  }
  
  var inputFiles = {};
  inputFiles[this.inputFile] = convertOptions({});
  
  avconv(inputFiles, outputFiles, convertOptions({}), function(err, finished, status){
    if(finished){
      cb && cb(err, 100, Date.now()-time);
    }else{
      cb && cb(err, ((status.frames / numFrames) * 100) % 100);
    }
  });
}

function convertOptions(opts){
  var result = {};
  
  if (opts.width && opts.height){
    result.s = opts.width + 'x' + opts.height;
  }
  
  _.each(opts, function(value, key){
    switch(key){
      case 'videoBitrate': 
        result['b:v'] = value;
        break;
      case 'videoCodec': 
        result.vcodec = value;
        break;
      case 'keepAspectRatio': 
        break;
      case 'offset':
        result.ss = value;
        break;
      case 'skipVideo': 
        result.vn = true;
        break;
      case 'audioBitrate':
        result['b:a'] = value;
        break;
      case 'audioCodec': 
        result.acodec = value;
        break;
      case 'channels': 
        result.ac = value;
        break;
      case 'skipAudio': 
        result.an = true;
      break;
    }
  });
  
  return result;
}

/*
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
  }else{
    time_base = {num: 1, den: 1}
  }
  
  for(var i=0;i<numThumbnails;i++){
    if(inputVideo){
      var 
        options = _.defaults(thumbnails[i].options, THUMBNAIL_DEFAULTS);
        
      if(options.keepAspectRatio){
        var dims = fitKeepingRatio(inputVideo.codec.width/inputVideo.codec.height, 
                                   options);
        options.width = dims.width;
        options.height = dims.height;
      }
                
      thumbnails[i].encoder = new native.NAVThumbnail(options);
      thumbnails[i].converter = new native.NAVSws(inputVideo, options);
    }
  }
  
  for(var i=0;i<numOutputs;i++){
    var 
      outputFormat = new native.NAVOutputFormat(outputs[i].filename),
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
                                     options);
          settings.width = dims.width;
          settings.height = dims.height;
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

    if(outputs[i].outputAudio || outputs[i].outputVideo){
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
      
      async.forEachSeries(outputs, function(output, done){
        if (output.outputVideo && (codecType === output.outputVideo.codec.codec_type)){
          try{
            var convertedFrame = output.converter.convert(frame);
            output.outputFormat.encode(output.outputVideo, convertedFrame, done);
          }catch(err){
            done(err);
          }
        } else if(output.outputAudio && (codecType === output.outputAudio.codec.codec_type)){
          try{
            var resampledFrame = output.resampler.convert(frame);
            output.outputFormat.encode(output.outputAudio, resampledFrame, done);
          }catch(err){
            done(err);
          }
        } else {
          done(err);
        }
      }, function(err){
        if(err){
          notifier.done(err);
          cb && cb(err);
        }else if(codecType === 'Video'){
          // TODO: Handle thumbnail creation with async.forEachSeries
          for(var i=0;i<numThumbnails;i++){
            var filenames = thumbnails[i].filenames;
            for(var filename in filenames){
              if(filenames[filename] <= pts){
                var scaledFrame = thumbnails[i].converter.convert(frame);
                thumbnails[i].encoder.write(scaledFrame, filename, function(err){
                  if(err){
                    console.log("Error writting thumbnail!:"+err);
                  }
                });
                delete filenames[filename];
              }
            }
          }
          notifier.done();
        }else{  
          notifier.done();
        }
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
*/
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
