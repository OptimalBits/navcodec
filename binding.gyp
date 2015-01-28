{
  "targets": [
    {
      "target_name": "navcodec", 
      "sources": [
        "src/navcodec.cpp", 
        "src/navcodeccontext.cpp", 
        "src/navframe.cpp", 
        "src/navformat.cpp", 
        "src/navstream.cpp", 
		    "src/navoutformat.cpp",
        "src/navpixformat.cpp", 
        "src/navsws.cpp", 
        "src/navcodecid.cpp", 
        "src/navresample.cpp",
        "src/navaudiofifo.cpp", 
        "src/navdictionary.cpp", 
        "src/navthumbnail.cpp", 
        "src/relocatemoov.cpp"
        ],
      "conditions": [
        ['OS=="mac"', {
		      "cflags" : [
            "-g", 
            "-D__STDC_CONSTANT_MACROS",
            "-D_FILE_OFFSET_BITS=64",
            "-D_LARGEFILE_SOURCE",
            "-O3",
            "-Wall"],
	    	    "libraries" : [
              "-lavcodec",
              "-lavformat",
              "-lswscale",
              "-lavresample",
              "-lavutil"
          ]
        }],
        ['OS=="linux"', {
		      "cflags" : [
            "-g", 
            "-D__STDC_CONSTANT_MACROS",
            "-D_FILE_OFFSET_BITS=64",
            "-D_LARGEFILE_SOURCE",
            "-O3",
            "-Wall"],
	    	    "libraries" : [
              "-lavcodec",
              "-lavformat",
              "-lswscale",
              "-lavresample",
              "-lavutil"
          ]
        }],
        ['OS=="win"', {
            "include_dirs": [
                "$(LIBAV_PATH)include"
                ],
            "libraries" : [
                  "-l$(LIBAV_PATH)avcodec",
                  "-l$(LIBAV_PATH)avformat",
                  "-l$(LIBAV_PATH)swscale",
                  "-l$(LIBAV_PATH)avresample",
                  "-l$(LIBAV_PATH)avutil"
                ]
        }]
      ],
	  }
  ]
}
