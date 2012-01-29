
navcodec = require('../');

var inputFile;
//inputFile  = "../the.walking.dead.s02e07.720p.hdtv.x264-orenji.mkv"; /x
inputFile = "assets/uncharted3.mp4"
//inputFile = "assets/oceans-clip.mp4"
//inputFile = "assets/MVI_3572.AVI" // incomplete sound
//inputFile = "assets/bars_100.avi"
//inputFile = "assets/byger-20030708-liten.avi" // crash
//inputFile = "assets/coals.mov"
//inputFile = "assets/griffon-dropout.m2t" // Broken
//inputFile = "assets/grill-mjpeg.mov" // Too fast
//inputFile = "assets/guywmic_ntsc.mov" // Produce error.
//inputFile = "assets/lys-20031106.avi" // crash
//inputFile = "assets/mars-20030905.avi"
//inputFile = "assets/morgen-20030714.avi"
//inputFile = "assets/motion_demo4.m2v"
// inputFile = "assets/prpol-rerender2.avi"
// inputFile = "assets/prpol-rerender2.mov"
//inputFile = "assets/rassegna2.avi" // error.
//inputFile = "assets/surfing-PP1-on-then-off.m2t" // Audio encode error
//inputFile = "assets/toy_plane_liftoff.avi"
//inputFile = "assets/wide-20040607-small.avi"
//inputFile = "assets/walk.flac"

navcodec.open(inputFile, function(err, media){
  if(err){
    console.log(err);
  }
  if(media){
    console.log('Width:'+media.width);
    console.log('Height:'+media.height);
    console.log('Video Bitrate:'+media.videoBitrate);
    console.log('Audio Bitrate:'+media.audioBitrate);
    console.log('Bitrate:'+media.bitrate);
    console.log('Sample Rate:'+media.samplerate);
    
    media.info();

    var options = {
      width:640,
      height:480,
      videoBitrate:600000,
//      videoCodec:'vp8',
      keepAspectRatio:true,
 //     audioBitrate:48000,
      //sampleRate:10000,
      channels:2,
      audioCodec:'aac'
    };
  
    media.transcode('output.mp4', options, function(err, progress){
      console.log(progress); 
      if(progress === 1){
        // Finished! 
      }
    });
  }
});
