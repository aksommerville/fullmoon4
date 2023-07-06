/* verifySprite.js
 */
 
module.exports = function(src, resources, ref/*(type,id)*/, metadata) {
  for (let srcp=0; srcp<src.length; ) {
  
    const lead = src[srcp++];
    let paylen;
         if (lead < 0x20) paylen = 0;
    else if (lead < 0x40) paylen = 1;
    else if (lead < 0x60) paylen = 2;
    else if (lead < 0x80) paylen = 3;
    else if (lead < 0xa0) paylen = 4;
    else if (lead < 0xd0) paylen = src[srcp++];
    else throw new Error(`Unknown sprite command ${lead}`);
    if (srcp > src.length - paylen) throw new Error(`Sprite command ${lead} overruns input (${paylen}-byte payload)`);
    
    switch (lead) {
    
      case 0x20: { // IMAGE
          const imageid = src[srcp];
          ref(0x01, imageid);
        } break;
    
    }
    srcp += paylen;
  }
  return 0;
};
