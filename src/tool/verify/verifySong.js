/* verifySong.js
 */
 
function verifyMThd(src) {
  if (src.length !== 6) throw new Error(`Unexpected length ${src.length} for MThd, expected 6`);
  const format = (src[0] << 8) | src[1];
  const trackCount = (src[2] << 8) | src[3];
  const division = (src[4] << 8) | src[5];
  if (format > 1) throw new Error(`MIDI format should be 0 or 1, I'm not sure which. Found ${format}`);
  // trackCount, whatever.
  if (division & 0x8000) throw new Error(`MIDI indicates SMPTE timing. We don't support that.`);
  if (!division) throw new Error(`Zero division in MIDI file.`);
}

function verifyMTrk(src, ref, index) {
  const pidByChannel = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
  let status = 0;
  for (let srcp=0; srcp<src.length; ) {
    
    // Delay (VLQ).
    let delay = src[srcp++];
    if (delay & 0x80) {
      delay &= 0x7f;
      delay <<= 7;
      delay |= src[srcp++] & 0x7f;
      if (src[srcp-1] & 0x80) {
        delay <<= 7;
        delay |= src[srcp++] & 0x7f;
        if (src[srcp-1] & 0x80) {
          delay <<= 7;
          delay |= src[srcp++];
          if (src[srcp-1] & 0x80) throw new Error(`Malformed VLQ around ${srcp-3}/${src.length} in MTrk ${index}`);
        }
      }
    }
    
    // Status byte.
    if (srcp >= src.length) throw new Error(`MTrk ${index} terminated expecting status byte.`);
    if (src[srcp] & 0x80) status = src[srcp++];
    else if (status) ;
    else throw new Error(`Expected status byte around ${srcp}/${src.length} in MTrk ${index}`);
    
    // Require channels other than 15 to appear in their own track, at the same index.
    // This is most definitely not a requirement of MIDI, and technically we don't have to care either.
    // But I do it for cleanliness's sake, and I want this support in case I miss one.
    if ((status >= 0x80) && (status < 0xf0)) {
      const chid = status & 0x0f;
      if ((chid !== 0x0f) && (chid !== index)) {
        throw new Error(`Found event ${status} for channel ${chid} on track ${index}. I'm capriciously requiring channel and track assignments to match.`);
      }
    }
    
    // Data bytes.
    const chid = status & 0x0f;
    if ((status < 0xf0) && (chid === 14)) throw new Error(`Event ${status} on channel 14. Please keep this channel clear for the violin.`);
    switch (status & 0xf0) {
    
      case 0x80: { // Note Off
          srcp += 2;
        } break;
        
      case 0x90: { // Note On
          if (!src[srcp + 1]) {
            // Actually Note Off, ignore it.
          } else if (chid === 0x0f) { // Drums.
            ref(8, src[srcp]);
          } else {
            if (!(pidByChannel[chid] & 0x7f)) {
              throw new Error(`Event (${status},${src[srcp]},${src[srcp+1]}) on channel ${chid} in MTrk ${index} around ${srcp}/${src.length}, before Program set.`);
            }
          }
          srcp += 2;
        } break;
        
      case 0xa0: { // Note Adjust
          //TODO We could issue a warning here. I don't plan to use aftertouch, it's just dead weight.
          if (!(pidByChannel[chid] & 0x7f)) {
            throw new Error(`Event (${status},${src[srcp]},${src[srcp+1]}) on channel ${chid} in MTrk ${index} around ${srcp}/${src.length}, before Program set.`);
          }
          srcp += 2;
        } break;
        
      case 0xb0: { // Control Change. Bank Select are special, others are regular events.
          if (chid === 0x0f) {
            throw new Error(`Control Change on channel 15 in MTrk ${index} around ${srcp}/${src.length}. 15 is reserved for drums; all Controls are noop.`);
          }
          const k = src[srcp++];
          const v = src[srcp++];
          if (k === 0x00) pidByChannel[chid] = (pidByChannel[chid] & 0x3fff) | (v << 14);
          else if (k === 0x20) pidByChannel[chid] = (pidByChannel[chid] & 0x1fc07f) | (v << 7);
          else if (!(pidByChannel[chid] & 0x7f)) {
            throw new Error(`Control Change ${k}=${v} on channel ${chid} in MTrk ${index} around ${srcp}/${src.length}, before Program set.`);
          }
        } break;
      
      case 0xc0: { // Program Change
          if (chid === 0x0f) {
            throw new Error(`Program Change on channel 15 in MTrk ${index} around ${srcp}/${src.length}. 15 is reserved for drums.`);
          }
          const pid = src[srcp++];
          pidByChannel[chid] = (pidByChannel[chid] & 0xffffffff80) | pid;
          if (!pidByChannel[chid]) throw new Error(`MTrk ${index} around ${srcp}/${src.length}, setting program zero.`);
          ref(7, pidByChannel[chid]);
        } break;
      
      case 0xd0: { // Channel Pressure
          //TODO We could issue a warning here. I don't plan to use aftertouch, it's just dead weight.
          if (!(pidByChannel[chid] & 0x7f)) {
            throw new Error(`Event (${status},${src[srcp]}) on channel ${chid} in MTrk ${index} around ${srcp}/${src.length}, before Program set.`);
          }
          srcp += 1;
        } break;
        
      case 0xe0: { // Pitch Wheel
          if (!(pidByChannel[chid] & 0x7f)) {
            throw new Error(`Event (${status},${src[srcp]},${src[srcp+1]}) on channel ${chid} in MTrk ${index} around ${srcp}/${src.length}, before Program set.`);
          }
          srcp += 2;
        } break;
        
      default: { // Meta, invalid, or forbidden Sysex
          if ((status === 0xf0) || (status === 0xf7)) {
            throw new Error(`Sysex in MTrk ${index} around ${srcp}/${src.length}. Please remove.`);
          }
          if (status !== 0xff) throw new Error(`Unexpected status byte 0x${status.toString(16)} around ${srcp}/${src.length} in MTrk ${index}`);
          status = 0;
          const type = src[srcp++];
          let len = src[srcp++];
          if (len & 0x80) {
            len &= 0x7f;
            len <<= 7;
            len |= src[srcp++] & 0x7f;
            if (src[srcp-1] & 0x80) {
              len <<= 7;
              len |= src[srcp++] & 0x7f;
              if (src[srcp-1] & 0x80) {
                len <<= 7;
                len |= src[srcp++];
                if (src[srcp-1] & 0x80) throw new Error(`Malformed VLQ around ${srcp-3}/${src.length} in MTrk ${index}`);
              }
            }
          }
          if (srcp > src.length - len) throw new Error(`Meta payload overruns MTrk ${index} around ${srcp}/${src.length}`);
          srcp += len;
        } break;
    }
  }
}
 
module.exports = function(src, resources, ref/*(type,id)*/) {
  let MThdCount = 0;
  let MTrkCount = 0;
  for (let srcp=0; srcp<src.length; ) {
    const chunkId = src.toString("utf8", srcp, srcp + 4);
    const chunkLen = (src[srcp + 4] << 24) | (src[srcp + 5] << 16) | (src[srcp + 6] << 8) | src[srcp + 7];
    srcp += 8;
    if ((chunkLen < 0) || (srcp > src.length - chunkLen)) {
      throw new Error(`Chunk at ${srcp-8}/${src.length} overruns file`);
    }
    const chunk = src.slice(srcp, srcp + chunkLen);
    srcp += chunkLen;
    switch (chunkId) {
      case "MThd": {
          if (MThdCount++) throw new Error(`Multiple MThd`);
          verifyMThd(chunk);
        } break;
      case "MTrk": {
          verifyMTrk(chunk, ref, MTrkCount);
          MTrkCount++;
        } break;
      default: throw new Error(`Unexpected chunk ${JSON.stringify(chunkId)} in MIDI file.`);
    }
  }
  if (!MTrkCount) throw new Error(`No MTrk chunks`);
  return 0;
};
