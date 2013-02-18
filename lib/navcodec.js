
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
      cb && cb(err, 100, true, Date.now()-time);
    }else{
      cb && cb(err, ((status.frames / numFrames) * 100) % 101, false);
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
