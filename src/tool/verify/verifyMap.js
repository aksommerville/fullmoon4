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
 
module.exports = function(src, id, resources, ref/*(type,id)*/, spriteMetadata) {
  if (src.length < 20 * 12) throw new Error(`Minimum length 20 * 12 = 240, found ${src.length}`);
  let songId = 0;
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
      case 0x20: ref(0x02, src[srcp]); songId = src[srcp]; break; // SONG
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
          let hasWerewolf = false;
          readCommands(other, (lead, payp) => {
            if (lead === 0x60) { // DOOR
              const backref = (other[payp+1] << 8) | other[payp+2];
              if (backref === id) {
                found = true;
                // We used to check that doors point to where their countpart lives.
                // That breaks at double-wide doors (eg church and castle).
                // Also I think no point checking: It's easy to see that mismatch in the editor.
              }
            } else if (lead === 0x81) { // BURIED_DOOR
              const backref = (other[payp+3] << 8) | other[payp+4];
              if (backref === id) {
                found = true;
              }
            } else if (lead === 0x80) { // SPRITE
              const spriteId = (other[payp+1] << 8) | other[payp+2];
              if (spriteId === 38) hasWerewolf = true;
            }
          });
          if (!found) {
            // Doesn't have to be an error. Maybe the "door" is a hole in the floor you fall thru? Or some other one-way thing?
            if (hasWerewolf) {
              // Entering the werewolf's room is supposed to be one-way, all good here.
            } else if (songId === 9) {
              // Map with the dangling reference has song "choose_a_door"; it's the warp zone.
              // Note that this requires "song" to appear before "door" in the map's commands.
            } else {
              throw new Error(`Door from ${id} to ${mapid} has no return door.`);
            }
          }
        } break;
        
      case 0x80: { // SPRITE
          const spriteid = (src[srcp+1] << 8) | src[srcp+2];
          ref(0x05, spriteid);
          const metadata = spriteMetadata.byId[spriteid];
          if (metadata) {
            const argv = [
              [src[srcp+3], metadata.argv[0]],
              [src[srcp+4], metadata.argv[1]],
              [src[srcp+5], metadata.argv[2]],
            ];
            for (const [v, def] of argv) {
              if (!def) continue; // no definition, that's fine, it's a plain scalar
              if (!v) continue; // zero always means "none", even for contexts like gsbit where it's a valid value.
              switch (def.type) {
                case "string": ref(0x06, v); break;
              }
            }
          }
        } break;
        
      case 0x81: { // BURIED_DOOR
          const mapid = (src[srcp+3] << 8) | src[srcp+4];
          ref(0x03, mapid);
        } break;
    }
    
    srcp += paylen;
  }
  return 0;
};
