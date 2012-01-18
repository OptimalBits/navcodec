
navcodec = require('../build/Release/navcodec.node');
console.log(navcodec);

// Transcoding Example
avformat = new navcodec.AVFormat("uncharted3.mp4");

//console.log(avformat);
//console.log(avformat.dump());

console.log(avformat.streams);

var counter = 0;
var time = Date.now();

outFormat = new navcodec.NAVOutputFormat("myfilename.mov")

// addStream(type, [codec_id], [options]);
videoStream = outFormat.addStream("Video", 
  {width:320, 
   height:240,
   bit_rate:600000,
   frame_rate:25,
   pix_fmt:navcodec.PixelFormat.PIX_FMT_YUV420P
  });

converter = new navcodec.NAVSws(avformat.streams[1], videoStream)
console.log(converter);

outFormat.begin();

avformat.decode([avformat.streams[1]], function(stream, frame){
 counter++;
 if(frame){
   if(stream.codec.codec_type === videoStream.codec.codec_type){
     frame = converter.convert(frame);
     outFormat.encode(videoStream, frame);
   }
 }else{
   outFormat.end();
 }
});

console.log("Duration: "+(Date.now()-time));
console.log(counter);
 
// audioStream = outformat.addStream("Audio", codecId);
