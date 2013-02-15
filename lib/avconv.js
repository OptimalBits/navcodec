
var child = require('child_process');
var _ = require('underscore');

// infiles = {filename: {opts}}
// outfiles: {filename: {opts}}
function AVConv(infiles, outfiles, options, cb)
{
  _.extend(options, {
    stats: true,
    y: true
  })
  
  var args = optionsToArgs(options);
  
  _.each(infiles, function(opts, filename){
    if(!_.isEmpty(opts)){
      args.push.apply(args, optionsToArgs(opts));
    }
    args.push('-i', filename);
  });
  
  _.each(outfiles, function(opts, filename){
    if(!_.isEmpty(opts)){
      args.push.apply(args, optionsToArgs(opts));
    }
    args.push('-strict', 'experimental', filename);
  });
  
  console.log(args.join(' '));
  var avconv = child.spawn('avconv', args);

  avconv.stdout.on('data', function (data) {
    console.log('stdout: ' + data);
  });

  avconv.stderr.on('data', function (data) {
    var status = parseStatus('' + data);
    if(status){
      cb(null, false, status);
    }else{
      console.log(""+data);
    }
  });

  avconv.on('exit', function (code) {
    cb(null, true);
  });
}

function optionsToArgs(opts){
  var args = [];
  
  opts && _.each(opts, function(value, key){
   args.push ('-'+key);
    if(value !== true){
      args.push(value);
    }
  })
  return args;
}

var statusRegExp = /frame=\s*([0-9]+)\s*fps=\s*([0-9]+)\s*/

function parseStatus(stderrString, totalDurationSec) {
    
    // get last stderr line
    var lines = stderrString.split(/\r\n|\r|\n/g);
    var lastLine = lines[lines.length - 2];
    var ret;
    
    if(lastLine) {
      var match = lastLine.match(statusRegExp);
      if(match){      
        var ret = {
          frames: match[1],
          fps: match[2]
        }
      }
    }
    return ret;
    
    /*
      if (progress && progress.length > 10) {
        // build progress report object
        var ret = {
          frames: parseInt(progress[1], 10),
          currentFps: parseInt(progress[2], 10),
          currentKbps: parseFloat(progress[10]),
          targetSize: parseInt(progress[5], 10),
          timemark: progress[6]
          };

          // calculate percent progress using duration
      
          if (totalDurationSec && totalDurationSec > 0) {
            ret.percent = (this.ffmpegTimemarkToSeconds(ret.timemark) / totalDurationSec) * 100;
          }

      this.options.onProgress(ret);
    */
    //return ret;
}

module.exports = AVConv;
