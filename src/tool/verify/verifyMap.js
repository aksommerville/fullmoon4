/* verifyMap.js
 */
 
function readCommands(src, cb/*(lead,payp)*/) {
  for (let srcp=20*12; srcp<src.length; ) {
    const lead = src[srcp++];
    if (!lead) break;
    let paylen;
         if (lead < 0x20) paylen = 0;
    else if (lead < 0x40) paylen = 1;
    else if (lead < 0x60) paylen = 2;
    else if (lead < 0x80) paylen = 4;
    else if (lead < 0xa0) paylen = 6;
    else if (lead < 0xc0) paylen = 8;
    else if (lead < 0xe0) paylen = src[srcp++];
    else return;
    cb(lead, srcp);
    srcp += paylen;
  }
}
 
module.exports = function(src, id, resources, ref/*(type,id)*/) {
  if (src.length < 20 * 12) throw new Error(`Minimum length 20 * 12 = 240, found ${src.length}`);
  for (let srcp=20*12; srcp<src.length; ) {
  
    // Verify command length generically.
    const lead = src[srcp++];
    if (!lead) throw new Error(`Explicit EOF in map. Don't do this, it just wastes space.`);
    let paylen;
         if (lead < 0x20) paylen = 0;
    else if (lead < 0x40) paylen = 1;
    else if (lead < 0x60) paylen = 2;
    else if (lead < 0x80) paylen = 4;
    else if (lead < 0xa0) paylen = 6;
    else if (lead < 0xc0) paylen = 8;
    else if (lead < 0xe0) paylen = src[srcp++];
    else throw new Error(`Unknown map command ${lead}, no implicit length.`);
    if (srcp > src.length - paylen) throw new Error(`Map command ${lead} around ${srcp}/${src.length} overruns input, payload ${paylen} bytes`);
    
    // Check specific commands.
    switch (lead) {
      case 0x20: ref(0x02, src[srcp]); break; // SONG
      case 0x21: ref(0x01, src[srcp]); ref(0x04, src[srcp]); break; // IMAGE (image + tileprops)
      
      case 0x40:
      case 0x41:
      case 0x42:
      case 0x43: { // neighbors (w,e,n,s)
          const mapid = (src[srcp] << 8) | src[srcp+1];
          ref(0x03, mapid);
          const otherRes = resources.find(r => r.type === 0x03 && r.id === mapid);
          if (!otherRes) throw new Error(`Link to neighbor map ${mapid}, map not found.`);
          const other = otherRes.serial;
          let otherCommand = lead ^ 1; // heh heh
          // If we do like a Lost Woods someday, this assertion becomes invalid:
          readCommands(other, (lead, payp) => {
            if (lead === otherCommand) {
              const backref = (other[payp] << 8) | other[payp+1];
              if (backref !== id) throw new Error(`Broken neighbor link between ${id}, ${mapid}, and ${backref}`);
            }
          });
        } break;
        
      case 0x60: { // DOOR
          const cellp = src[srcp];
          const mapid = (src[srcp+1] << 8) | src[srcp+2];
          const dstcellp = src[srcp+3];
          ref(0x03, mapid);
          const otherRes = resources.find(r => r.type === 0x03 && r.id === mapid);
          if (!otherRes) throw new Error(`Door to neighbor map ${mapid}, map not found.`);
          const other = otherRes.serial;
          let found = false;
          readCommands(other, (lead, payp) => {
            if (lead === 0x60) {
              const backref = (other[payp+1] << 8) | other[payp+2];
              if (backref === id) {
                found = true;
                const ocellp = other[payp];
                const odstcellp = other[payp+3];
                if ((cellp !== odstcellp) || (dstcellp !== ocellp)) {
                  // This certainly doesn't need to be an error. But for now, I think doors will always land you exactly on the remote door.
                  throw new Error(`Door between ${id} and ${mapid}, entrances and exits are not in sync.`);
                }
              }
            }
          });
          if (!found) {
            // Doesn't have to be an error. Maybe the "door" is a hole in the floor you fall thru? Or some other one-way thing?
            // But we're not doing any of that yet, nor planning to.
            throw new Error(`Door from ${id} to ${mapid} has no return door.`);
          }
        } break;
        
      case 0x80: { // SPRITE
          const spriteid = (src[srcp+1] << 8) | src[srcp+2];
          ref(0x05, spriteid);
          // Opportunity to validate sprite args, if we ever feel a need.
        } break;
    }
    
    srcp += paylen;
  }
  return 0;
};
