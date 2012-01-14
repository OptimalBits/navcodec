
navcodec = require('../build/Release/navcodec.node');
console.log(navcodec);

avformat = new navcodec.avformat();

console.log(avformat.open("uncharted3.mp4"));

