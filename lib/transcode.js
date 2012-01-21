
navcodec = require('../build/Release/navcodec.node');

function transcode(input, output, preset, cb){
  var inputVideo = null, 
      inputAudio = null,
      outputVideo = null,
      outputAudio = null, 
      streams,
      inputFormat,
      outputFormat,
      counter,
      time,
      time_base;
      
  // Open Input Video.
  inputFormat = new navcodec.AVFormat(input);
  
  console.log(inputFormat);
  
  streams = inputFormat.streams;

  // Find first video and first audio streams  
  for(var i=0,len=streams.length;i<len;i++){
    if((inputVideo === null) && (streams[i].codec.codec_type === 'Video')) {
      inputVideo = streams[i];
    }
    
    if((inputAudio === null) && (streams[i].codec.codec_type === 'Audio')) {
      inputAudio = streams[i];
    }
  }

  console.log(inputVideo);
  console.log(inputAudio);

  counter = 0;
  time = Date.now();

  // Create Output Video
  
  outputFormat = new navcodec.NAVOutputFormat(output)

  if(inputVideo){
    time_base = inputVideo.codec.time_base;
    time_base.num *= inputVideo.codec.ticks_per_frame;

    var options = {
      time_base: time_base,
      ticks_per_frame: 1,
      pix_fmt:navcodec.PixelFormat.PIX_FMT_YUV420P
    };
  
    preset.width && (options.width = preset.width);
    preset.height && (options.height = preset.height);
    preset.bit_rate_video && (options.bit_rate = preset.bit_rate_video);
  
    outputVideo = outputFormat.addStream("Video", options);
    
    // Create a converter (Only usefull if changing dimensions 
    // and/or pixel format. Otherwise it will act as a passthrough filter.
    converter = new navcodec.NAVSws(inputVideo, outputVideo);
  }

  if(inputAudio){
    var options = {};
  
    preset.bit_rate_audio && (options.bit_rate = preset.bit_rate_audio);
  
    outputAudio = outputFormat.addStream("Audio", options);
  }

  //
  // Start Encoder
  //
  outputFormat.begin();
  
  streams = [];
  inputAudio && streams.push(inputAudio);
  inputVideo && streams.push(inputVideo);

  inputFormat.decode(streams, function(stream, frame, progress){
    counter++;
    if(frame){
      if(counter%100 === 0){
        cb && cb(progress);
      }
      if (outputVideo && (stream.codec.codec_type === outputVideo.codec.codec_type)){
        frame = converter.convert(frame);
        outputFormat.encode(outputVideo, frame);
      }
      
      if (outputAudio && (stream.codec.codec_type === outputAudio.codec.codec_type)){
        outputFormat.encode(outputAudio, frame);
      }
    }else{
     // No more frames, end encoding.
      outputFormat.end();
    }
  });

  console.log("Duration: "+(Date.now()-time));
  console.log(counter);  

}

module.exports = transcode;
