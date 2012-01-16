def set_options(opt):
  opt.tool_options("compiler_cxx")

def configure(conf):
  conf.check_tool("compiler_cxx")
  conf.check_tool("node_addon")
  conf.check_cfg(package='libavcodec', args='--cflags --libs', uselib_store='libavcodec')
  conf.check_cfg(package='libavformat', args='--cflags --libs', uselib_store='libavformat')

def build(bld):
  obj = bld.new_task_gen("cxx", "shlib", "node_addon") 
  obj.cxxflags = ["-g", "-D__STDC_CONSTANT_MACROS", "-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE", "-Wall"]
  obj.linkflags = ["-lavcodec","-lavformat"]
  obj.target = "navcodec"
  obj.source = ["src/navcodec.cpp", "src/avcodeccontext.cpp", "src/avframe.cpp", "src/avformat.cpp", "src/avstream.cpp" ]
  obj.uselib = ['libavcodec', 'libavformat']

