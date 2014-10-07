"use strict";
var 
  native = require('../build/Release/navcodec.node'),
  avconv = require('./avconv'),
  fs = require('fs'),
  _ = require('underscore'),
  async = require('async'),
  when = require('when'),
  printf = require('printf');

var THUMBNAIL_DEFAULTS = {
  pix_fmt:native.PixelFormat.PIX_FMT_YUVJ420P,
  width:64,
  height:64,
  bit_rate:1000
}

function open(input, options, cb) {
  var defer = when.defer();
  
  // Take first frame if the input is a sequence
  var pinput = printf(input, 0);
  
  if (typeof options === 'function') {
    cb = options;
    options = {};
  }

  fs.stat(pinput, function(err, stats){      
    if(err){
      defer.reject(err);
      return;
    }
    
    try{
      var 
        inputFormat = new native.NAVFormat(pinput),
        media = new Media(input, inputFormat, options, stats.size);
      defer.resolve(media);
    }catch(err){
      defer.reject(err);
    }
  });
  
  if(cb){
    defer.promise.then(function(media){
      cb(null, media);
    }, function(err){
      cb(err);
    })
  }
  
  return defer.promise;
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

var Media = function(inputFile, inputFormat, inputOptions, fileSize){
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

  this.inputOptions = _.extend({"y" : true, "stats" : true}, inputOptions);

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
  try{
    var inputVideo = null, 
        inputAudio = null,
        outputs = this._outputs,
        numOutputs = outputs.length,
        thumbnails = this._thumbnails,
        numThumbnails = thumbnails.length,
        hasVideo = false,
        hasAudio = false;

    var time = Date.now();
  
    inputVideo = this.videoStreams[0];
    inputAudio = this.audioStreams[0];
  
    var numFrames;
    if(inputVideo){
      numFrames = inputVideo.numFrames;
      if(!numFrames && inputVideo.frameRate){
        numFrames =
          (this.duration * inputVideo.frameRate.num) / inputVideo.frameRate.den;
      }
    }

    if(inputAudio){
      numFrames = numFrames || inputAudio.numFrames;
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
    
      outputFiles[outputs[i].filename] = convertOptions(opts, inputVideo);
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
          outputFiles[filename] = convertOptions(thumbOpts, inputVideo)
          outputFiles[filename].vframes = 1;
        }
      }
    }
  
    var inputFiles = {};
    inputFiles[this.inputFile] = convertOptions(this.inputOptions, inputVideo);

    avconv(inputFiles, outputFiles, convertOptions({}, inputVideo), function(err, finished, status){
      if(finished){
        cb && cb(err, 100, true, Date.now()-time);
      }else{
        cb && cb(err, ((status.frames / numFrames) * 100) % 101, false);
      }
    });
  }catch(err){
    cb && cb(err);
  }
}

function convertOptions(opts, inputVideo){
  var result = {};
  var tmp;
  
  if (opts.width && opts.height){
    if(opts.rotate){
      tmp = opts.width;
      opts.width = opts.height;
      opts.height = tmp;
    }
    result.s = opts.width + 'x' + opts.height;
  }
  
  _.each(opts, function(value, key){
    switch(key){
      case 'y':
        result['y'] = true;
        break;
      case 'stats':
        result['stats'] = true;
        break;
      case 'sampleRate':
        result['ar'] = value;
        break;
      case 'videoBSF':
        result['bsf'] = value;
        break;
      case 'streamCopy':
        result['codec'] = "copy";
        break;
      case 'loglevel':
        if (value == 'error' || value == 'quiet' || value == 'verbose' || value == 'debug')
          result['loglevel'] = value;
        break;
      case 'itsoffset':
        result['itsoffset'] = value;
        break;
      case 'format':
        result['f'] = value;
        break;
      case 'fps':
        result.r = value;
        break;
      case 'minFps':
        if(inputVideo.frameRate){
          if((inputVideo.frameRate.num / inputVideo.frameRate.den) < value){
            result.r = value;
          }
        }else{
          result.r = value;
        }
        break;
      case 'videoBitrate': 
        if(opts.crf){
          result['maxrate'] = result['maxrate'] || value;
          result['bufsize'] = result['bufsize'] || '1000k'; 
        }else{
          result['b:v'] = value;
        }
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
      case 'rotate': 
        if(value === 'clock'){
          result.vf = 'transpose=1';
        }else if(value === 'cclock'){
          result.vf = 'transpose=2';
        }
        break;
      case 'crf':
      case 'preset':
        result[key] = value;
        break;
      case 'videoProfile':
        result['profile:v'] = value;
        break;
    }
  });
  
  return result;
}

Media.prototype._bitrate = function(streams){
  var bitrate = 0;
  for(var i=0,len=streams.length;i<len;i++){
    bitrate += streams[i].codec.bit_rate;
  }
  return bitrate;
}
           
module.exports.open = open;
module.exports.relocateMoov = function relocateMoov(input, output, cb){
  var defer = when.defer();
  native.relocateMoov(input, output, function(err){
    if(err) defer.reject(err);
    else defer.resolve();
  });
  
  if(cb){
    defer.promise.then(cb, cb);
  }
  
  return defer.promise;
}
  
module.exports.CodecId = native.CodecId;
