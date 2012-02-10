
var navcodec = require('../'),
      should = require('should');

var inputs = ["assets/uncharted3.mp4", 
              "assets/oceans-clip.mp4",
              "assets/bars_100.avi",
              "assets/coals.mov"];
              
var audios = ["assets/walk.flac"];

//inputFile  = "../the.walking.dead.s02e07.720p.hdtv.x264-orenji.mkv"; /x
//inputFile = 
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

describe('Video', function(){
	describe('open', function(){
		it('should return a media object', function(){
		  navcodec.open(inputs[0], function(err, media){
		    should.not.exist(err);
		    should.exist(media);
		    should.exist(media.width);
		    should.exist(media.height);
		    should.exist(media.videoBitrate);
		    should.exist(media.audioBitrate);
		    should.exist(media.bitrate);
		    should.exist(media.samplerate);
		    should.exist(media.metadata);
		    
		    media = null;
  	  });
  	})
	})
	
	describe('transcode', function(){
	  it('should transcode to a mp4 video', function(done){
  	  navcodec.open(inputs[0], function(err, media){
	      should.not.exist(err);
	      should.exist(media);
	    
	      var options = {
	        width:640,
	        height:480,
	        videoBitrate:600000,
	        keepAspectRatio:true,
	        channels:2,  
	      };
	    
	      media.addOutput('output.mp4', options);
	    
	      media.transcode(function(err, progress, time){
	        should.not.exist(err);
	      
	        if(progress === 100){
	          done();
	        }
	      });
	      media = null;
	    });
	  });
	});
	
	describe('thumbnail', function(){
	  it('should transcode to a mp4 video and create a thumbnail', function(done){
	    navcodec.open(inputs[1], function(err, media){
	      should.not.exist(err);
	      should.exist(media);
	    
	      var options = {
	        width:640,
	        height:480,
	        videoBitrate:600000,
	        audioBitrate:128000,
	        keepAspectRatio:true,
	        channels:2,  
	      };
	    
	      media.addOutput('output2.mp4', options);
	    
	      options = {
	        width:64,
	        height:64,
	        videoBitrate:100000,
	        keepAspectRatio:true,
	        skipAudio:true,
	        maxVideoFrames:20,
	        videoFrameInterval:5
	      };
	    
	      media.addOutput('thumbnail.mp4', options);
	    
	      media.transcode(function(err, progress, time){
	        should.not.exist(err);
	      
	        if(progress === 100){
	          done();
	        }
	      });
	      media = null;
	    });
	  });
	});
	
	describe('Audio', function(){
		it('open should return a media object', function(){
		  navcodec.open(audios[0], function(err, media){
		    should.not.exist(err);
		    should.exist(media);
		    should.exist(media.width);
		    should.exist(media.height);
		    should.exist(media.videoBitrate);
		    should.exist(media.audioBitrate);
		    should.exist(media.bitrate);
		    should.exist(media.samplerate);
		    should.exist(media.metadata);
		  });
		})
	})
	
})
	   	   
/*
describe('Audio', function(){
	describe('open', function(){
		it('should return a media object', function(){
		  navcodec.open(audios[0], function(err, media){
		    should.not.exist(err);
		    should.exist(media);
		    should.exist(media.width);
		    should.exist(media.height);
		    should.exist(media.videoBitrate);
		    should.exist(media.audioBitrate);
		    should.exist(media.bitrate);
		    should.exist(media.samplerate);
		    should.exist(media.metadata);
  	  });
  	})
	})
	describe('transcode', function(){
		it('should return a media object', function(done){
		  navcodec.open(audios[0], function(err, media){
		    should.not.exist(err);
		    should.exist(media);
		    
		    var options = {
		      width:640,
		      height:480,
		      audioBitrate:64000,
		      channels:2,  
		    };
		    
		    media.addOutput('audioOut.mp4', options);
		    
		    media.transcode(function(err, progress, time){
		      should.not.exist(err);
		    
		      if(progress === 100){
		        done();
		      }
		    });
		    
		  });
		})
	})
});
*/    
/*
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
    
    console.log("Metadata:");
    console.log(media.metadata);
    
    
  //  media.info();

    var options = {
      width:640,
      height:480,
      videoBitrate:600000,
//      videoCodec:'vp8',
      keepAspectRatio:true,
 //     audioBitrate:48000,
      //sampleRate:10000,
      channels:2,
      audioCodec:'aac',
   //   skipAudio:true,
      maxVideoFrames:20,
      videoFrameInterval:5
    };
  
    media.addOutput('output.mp4', {
      width:640,
      height:480,
      videoBitrate:600000,
      //videoCodec:'vp8',
      keepAspectRatio:true,
     //audioBitrate:48000,
      //sampleRate:10000,
      channels:2,
      audioCodec:'aac',
     //   skipAudio:true,
      maxVideoFrames:20,
      videoFrameInterval:5
    });
  
    media.addOutput('thumbnail.mp4', {
      width:160, 
      height:120, 
      skipAudio:true
    });
  
    media.transcode(function(err, progress, time){
      console.log("Progress:"+progress); 
      if(progress === 100){
        // Finished! 
        console.log(time);
      }
    });
  }
});
*/
