#!/usr/bin/env node

const fs = require("fs");

const dirPath = "src/data/song";

/* VLQ.
 */
 
function readVlq(src, srcp) {
  if (srcp >= src.length) throw new Error(`invalid vlq, eof`);
  if ((srcp + 0 < src.length) && !(src[srcp + 0] & 0x80)) return [1, src[srcp]];
  if ((srcp + 1 < src.length) && !(src[srcp + 1] & 0x80)) return [2, ((src[srcp] & 0x7f) << 7) | src[srcp+1]];
  if ((srcp + 2 < src.length) && !(src[srcp + 2] & 0x80)) return [3, ((src[srcp] & 0x7f) << 14) | ((src[srcp+1] & 0x7f) << 7) | src[srcp+2]];
  if ((srcp + 3 < src.length) && !(src[srcp + 3] & 0x80)) return [4, ((src[srcp] & 0x7f) << 21) | ((src[srcp+1] & 0x7f) << 14) | ((src[srcp+2] & 0x7f) << 7) | src[srcp+3]];
  throw new Error(`failed to decode vlq`);
}

/* Generate report for one MTrk chunk.
 */
 
function readTrack(src, path) {
  const report = {};
  let status = 0;
  let time = 0; // in ticks
  for (let srcp=0; srcp<src.length; ) {
  
    // Delay.
    const [seqlen, delay] = readVlq(src, srcp);
    srcp += seqlen;
    time += delay;
    
    // Do account for Running Status; there could be eg multiple Control Change at time zero.
    let lead = src[srcp];
    if (lead & 0x80) { status = lead; srcp++; }
    else if (!lead) {
      console.log(`${path}: Unexpected leading byte ${lead} at ${srcp}/${src.length} in MTrk`);
      return report;
    } else lead = status;
    
    switch (lead & 0xf0) {
    
      case 0x80: srcp += 2; break;
      
      case 0x90: {
          const note = src[srcp++];
          const velocity = src[srcp++];
          if ((lead & 0x0f) === 0x0f) { // channel 15, we use for drums: note is a sound id
            if (!report.drums) report.drums = [];
            if (report.drums.indexOf(note) < 0) report.drums.push(note);
          }
          if (!report.programs) report.notesWithoutProgram = true;
        } break;
      
      case 0xa0: srcp += 2; report.hasNoteAdjust = true; break;
      
      case 0xb0: {
          const k = src[srcp++];
          const v = src[srcp++];
        } break;
        
      case 0xc0: {
          if (!report.programs) report.programs = [];
          report.programs.push(src[srcp]);
          srcp++;
          if (time) report.programChangeAtNonzeroTime = true;
        } break;
        
      case 0xd0: srcp += 1; report.hasChannelPressure = true; break;
      
      case 0xe0: srcp += 2; report.hasPitchWheel = true; break;
      
      // Meta and Sysex, not examining.
      default: status = 0; switch (lead) {
          case 0xff: srcp += 1; // pass
          case 0xf0: case 0xf7: {
              const [seqlen, vlen] = readVlq(src, srcp);
              srcp += seqlen;
              srcp += vlen;
            } break;
          default: {
              console.log(`${path}: Unexpected leading byte ${lead} around ${srcp}/${src.length} in MTrk`);
              return report;
            }
        }
    }
  }
  return report;
}

/* Merge reports, (dst) is the file's aggregate and (src) is for one MTrk.
 */
 
function mergeReports(dst, src) {
  if (src.drums) {
    if (dst.drums) {
      for (const note of src.drums) {
        if (dst.drums.indexOf(note) < 0) dst.drums.push(note);
      }
    } else dst.drums = src.drums;
  }
  if (src.programs) {
    if (dst.programs) {
      for (const program of src.programs) {
        if (dst.programs.indexOf(program) < 0) dst.programs.push(program);
      }
    } else dst.programs = src.programs;
  }
  for (const k of [
    "notesWithoutProgram",
    "hasNoteAdjust",
    "hasChannelPressure",
    "hasPitchWheel",
    "programChangeAtNonzeroTime",
  ]) if (src[k]) dst[k] = true;
}

/* Dump the aggregated report for one file.
 */
 
function dumpFileReport(report) {
  console.log(JSON.stringify(report));
}

/* Main loop. Extract all MTrk from each file and gather reports.
 */

const allFiles = [];
for (const base of fs.readdirSync(dirPath)) {
  const path = dirPath + '/' + base;
  const src = fs.readFileSync(path);
  const fileReport = { path, length: src.length };
  for (let srcp=0; srcp<src.length; ) {
    const chunkId = src.toString("latin1", srcp, srcp + 4);
    const chunkLen = (src[srcp + 4] << 24) | (src[srcp + 5] << 16) | (src[srcp + 6] << 8) | src[srcp + 7];
    srcp += 8;
    if ((chunkLen < 0) || (srcp > src.length - chunkLen)) {
      console.log(`${path}: Framing error around ${srcp - 8}/${src.length}, chunkId=${JSON.stringify(chunkId)} chunkLen=${chunkLen}`);
      continue;
    }
    if (chunkId !== "MTrk") {
      srcp += chunkLen;
      continue;
    }
    const report = readTrack(src.slice(srcp, srcp + chunkLen), path);
    srcp += chunkLen;
    mergeReports(fileReport, report);
  }
  dumpFileReport(fileReport);
  allFiles.push(fileReport);
}

/* Check features across multiple files.
 */
 
for (let ai=1; ai<allFiles.length; ai++) {
  const a = allFiles[ai];
  for (let bi=0; bi<ai; bi++) {
    const b = allFiles[bi];
    
    if (a.programs && b.programs) {
      for (const program of a.programs) {
        if (b.programs.indexOf(program) >= 0) {
          console.log(`*** program ${program} present in both ${a.path} and ${b.path}`);
        }
      }
    }
  }
}
