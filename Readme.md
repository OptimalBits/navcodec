Node Video Transcoding Library [![BuildStatus](https://secure.travis-ci.org/OptimalBits/navcodec.png?branch=master)](http://travis-ci.org/optimalbits/navcodec)
=

*navcodec* provides a high level and flexible javascript API for video and audio transcoding based on the excelent [libavcodec](http://libav.org) library.

The library can transcode almost any known video and audio format efficiently, it can generate thumbnails or output video in multiple resolutions in parallel, and also optimize the output video for web reproduction.


Follow [optimalbits](http://twitter.com/optimalbits) for news and updates regarding this library.

Transcoding
-

	navcodec = require('navcodec');
	
	navcodec.open('myinput.mov', function(err, media){
	  if(media){
	    media.addOutput('myoutput.mp4', {
	    	width:640,
	    	height:480,
	    	audioBitrate:128000,
	    	videoBitrate:500000
	    });
	    
	    media.transcode(function(err, progress, time){
	      console.log(progress);
	      if(progress === 100){
	        console.log('total transcoding time:'+time);
	      }
	    }
	  }
	});


This will transcode myinput.mov into myoutput.mp4 according to the given options. The callback will be called with a progress variable between 0 and 100.

Since the transcode uses mostly *libavcodec* optimized functions, the above example will run really fast.

Available options (and their defaults):

    width ( input width )
    height (input height )
    videoBitrate (input bitrate if available, 0 otherwise)
    videoCodec ( standard codec for current file container)
    keepAspectRatio (true)
    audioBitrate (input bitrate if available, 0 otherwise)
    sampleRate (44100)
    channels (2),
    audioCodec (standard codec for current file container)
    skipAudio (false)
    skipVideo (false)
    maxVideoFrames (inf)
    videoFrameInterval (1)
    


Multiple outputs
-

Several outputs can be added to the media object, and when transcoding the data will be processed in parallel. This is quite convenient when generating thumbnails (which will be very cheap to generate), or if several output formats are required (will only require one decoding process of the input file):

    navcodec.open('myinput.mov', function(err, media){
      if(media){
        media.addOutput('myoutput.mp4', {
    	    width:640,
    	    height:480,
    	    audioBitrate:128000,
    	    videoBitrate:500000
        });
    
        media.addOutput('thumbnail.mp4', {
    	    width:80,
    	    height:60,
    	    skipAudio:true,
    	    videoBitrate:50000
        });
    
        media.transcode(function(err, progress, time){
          console.log(progress);
          if(progress === 100){
            console.log('total transcoding time:'+time);
          }
        });
      }
    });


Jpeg Thumbnails
-

A very common use case is the generation of a jpeg thumbnail to represent some video sequence. This can be easily accomplished by calling the function *thumbnail*. It can be called once or several times if different options are required. See the following example, where for every generated thumbnail a time offset is choosen:

    
    navcodec.open('myinput.mov', function(err, media){
      if(media){
        media.thumbnail([{'first128.jpg':1, 'secong128.jpg:100.5'},{width:128,height:128});
        media.thumbnail([{'first64.jpg':1, 'secong64.jpg:100.5'},{width:64,height:64});
        
        media.transcode(function(err, progress, time){
          console.log(progress);
          if(progress === 100){
            console.log('total transcoding time:'+time);
          }
        });
      }
    });

Note that generating thumbnails at the same time as transcoding a full video is a extreamly cheap operation.


Metadata
-

Metadata is available after opening the media file. Its just a javascript object with keys and values:

    navcodec.open('walk.flac', function(err, media){
      if(media){
        console.log(media.metadata);
      }
    });

The previous example would result in the following output:

    {
      ARTIST: 'Foo Fighters',
      TITLE: 'Walk',
      ALBUM: 'Wasting Light',
      DATE: '2011',
      track: '11',
      TOTALTRACKS: '11',
      GENRE: 'Rock',
      album_artist: 'Foo Fighters',
      'ALBUM ARTIST': 'Foo Fighters',
      COMMENT: 'EAC V1.0 beta 1, Secure Mode, Test & Copy, AccurateRip, FLAC 1.2.1b -8' 
    }

Besides the 'metadata' property, other convenient properties are also available in the media object:

    width
    height
    videoBitrate
    audioBitrate
    bitrate
    samplerate
    duration

Optimize for Web
-

h.264 files created by libavcodec include the moov atom block at the end of the file ([Understanding the MPEG-4 movie atom](http://www.adobe.com/devnet/video/articles/mp4_movie_atom.html)).

navcodec provides a function called *relocateMoov* that will move the moov atom at the begining of the file, thus making it better for seeking in a web based player. Example:

    navcodec.relocateMoov('myinput.mp4','myoutput.mp4', function(err){
      if(err){
        console.log(err);
      }
    });


Install
-

navcodec depends on [libavcodec](http://libav.org), and this library
must be installed before you can install this module using npm. 

The library needed is libavcodec version 0.9.x and above. Version 0.9.x includes some new APIs, that are used by this extension.

For most unixes there are packages available, in ubuntu for example use *libavcodec-dev*. If you want you can also compile the source code with your preferred settings, this can be useful if you want to get the maximum performance from the library.

For Mac OSX use brew and install the ffmpeg package.

	brew install ffmpeg

Note that version 0.9.x of libavcodec is brand new and the afore mentioned package managers may not have yet updated it.


When the libavcodec dependencies are fulfilled, just use npm to install the package:

	npm install navcodec


  
Compiling libavcodec (Recommended)
-

- Download the latest stable 0.9.x release from [libav.org](http://libav.org/download.html)

- Uncompress the tarball, Ex:

    tar -xvf libav-0.9.1.tar.gz
    cd libav-0.9.1
  
- Configure your makefile, example:

		./configure  --prefix=/usr/local --enable-shared --enable-gpl --enable-version3 --enable-nonfree --enable-hardcoded-tables --cc=/usr/bin/clang --enable-libfaac --enable-libmp3lame --enable-libtheora --enable-libvorbis --enable-libvpx --enable-libxvid --enable-libx264
		
You may want to add some hardware acceleration flags for your architecture, for example:

    --enable-vda  (Mac OS)
    --enable-vaapi (Linux/Unix Intel)
    --enable-vdpau (Linux/Unix Nvidia)
  
you can get a list of other configuration options using:

	./configure --help
  
- Compile the libraries:

		make
  
- Install

		make install

- Notes

Mac users will need to install Xcode in order to be able to compile libavcodec by themselves. 

It may be needed to install libvpx separatelly if you need WebM support (in Mac):

    brew install libvpx
	

References
-

[Ubuntu HOWTO: Install and use the latest FFmpeg and x264](http://ubuntuforums.org/showthread.php?t=786095)

Roadmap
-

- Multiple pass encoding.
- Add support for libavfilter.

License
-

Copyright(c) 2012 Optimal Bits Sweden AB. All rights reserved.
 
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
 

