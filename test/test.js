
navcodec = require('../');
console.log(navcodec);

options = {
  width:320,
  height:240,
  bit_rate_video:600000,
  bit_rate_audio:128000
};

navcodec.transcode("oceans-clip.mp4", "output.mp4",options, function(progress){
  console.log(progress);
});
