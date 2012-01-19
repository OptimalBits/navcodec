
navcodec = require('../build/Release/navcodec.node');
console.log(navcodec);

//
// Transcoding Example
//

// Open Input Video.
avformat = new navcodec.AVFormat("uncharted3.mp4");

inputVideoStream = avformat.streams[1];

console.log(avformat.dump());

var counter = 0;
var time = Date.now();

// Create Output Video
outFormat = new navcodec.NAVOutputFormat("myfilename.mov")

// audioStream = outformat.addStream("Audio", codecId);

// Add a Video stream to the output
videoStream = outFormat.addStream("Video", 
  {width:320, 
   height:240,
   bit_rate:500000,
   frame_rate:inputVideoStream.codec.framerate,
   pix_fmt:navcodec.PixelFormat.PIX_FMT_YUV420P
  });

// Create a converter (Only usefull if changing dimensions 
// and/or pixel format. Otherwise it will act as a passthrough filter.
converter = new navcodec.NAVSws(avformat.streams[1], videoStream)

// Start Encoder
outFormat.begin();

// Transcode!
avformat.decode([avformat.streams[1]], function(stream, frame){
 counter++;
 if(frame){
   if(stream.codec.codec_type === videoStream.codec.codec_type){
     // Convert and output video frame.
     frame = converter.convert(frame);
     outFormat.encode(videoStream, frame);
   }
 }else{
   // No more frames, end encoding.
   outFormat.end();
 }
});

console.log("Duration: "+(Date.now()-time));
console.log(counter);
