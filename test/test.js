
navcodec = require('../');
console.log(navcodec);

navcodec.transcode("uncharted3.mp4", "output.mp4", {
  width:320,
  height:240,
  bit_rate_video:600000,
  bit_rate_audio:128000});
