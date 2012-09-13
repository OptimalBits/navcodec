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
	          ['OS=="linux"', {
		    "cflags" : ["-g", "-D__STDC_CONSTANT_MACROS", "-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE", "-Wall"],
	    	    "libraries" : ["-lavcodec","-lavformat","-lswscale","-lavutil"]

          	  }],
           ],
	}
    ]
}

