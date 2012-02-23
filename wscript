def set_options(opt):
  opt.tool_options("compiler_cxx")

def configure(conf):
  conf.check_tool("compiler_cxx")
  conf.check_tool("node_addon")
  conf.check_cfg(package='libavcodec', args='--cflags --libs', uselib_store='libavcodec')
  conf.check_cfg(package='libavformat', args='--cflags --libs', uselib_store='libavformat')
  conf.check_cfg(package='libswscale', args='--cflags --libs', uselib_store='libswscale')
  conf.check_cfg(package='libavutil', args='--cflags --libs', uselib_store='libavutil')

def build(bld):
  obj = bld.new_task_gen("cxx", "shlib", "node_addon") 
  obj.cxxflags = ["-g", "-D__STDC_CONSTANT_MACROS", "-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE", "-Wall"]
  obj.linkflags = ["-lavcodec","-lavformat","-lswscale","-lavutil"]
  obj.target = "navcodec"
  obj.source = ["src/navcodec.cpp", "src/navcodeccontext.cpp", "src/navframe.cpp", "src/navformat.cpp", "src/navstream.cpp", "src/navoutformat.cpp","src/navpixformat.cpp", "src/navsws.cpp", "src/navcodecid.cpp", "src/navresample.cpp", "src/navaudiofifo.cpp", "src/navdictionary.cpp", "src/navthumbnail.cpp", "src/relocatemoov.cpp"]
  obj.uselib = ['libavcodec', 'libavformat', 'libswscale','-lavutil']
