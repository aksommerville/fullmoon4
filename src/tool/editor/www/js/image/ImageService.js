/* ImageService.js
 * Things related to Image and Tileprops objects.
 * Mostly Tileprops, since Image is just the DOM Image class; browser does all the work.
 */
 
import { Tileprops } from "./Tileprops.js";
 
export class ImageService {
  static getDependencies() {
    return [];
  }
  constructor() {
  }
  
  decodeTileprops(src, id) {
    const tileprops = new Tileprops(src);
    tileprops.id = id || 0;
    return tileprops;
  }
}

ImageService.singleton = true;

ImageService.PRESETS = [{
  name: "fat5x3",
  desc: "Fills any shape if at least two tiles wide everywhere.",
  w: 5,
  h: 3,
  neighbors: [
    0x0b, 0x1f, 0x16, 0xfe, 0xfb,
    0x6b, 0xff, 0xd6, 0xdf, 0x7f,
    0x68, 0xf8, 0xd0, 0x00, 0x00,
  ],
  weight: [
    0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff,
  ],
}, {
  name: "skinny4x4",
  desc: "Fills any shape at all, but awkward en masse.",
  w: 4,
  h: 4,
  neighbors: [
    0xa0, 0x1a, 0x12, 0x02,
    0x4a, 0x5a, 0x52, 0x42,
    0x48, 0x58, 0x50, 0x40,
    0x08, 0x18, 0x10, 0x00,
  ],
  weight: [
    0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff,
  ],
}, {
  name: "square3x3",
  desc: "No concave corners.",
  w: 3,
  h: 3,
  neighbors: [
    0x0b, 0x1f, 0x16,
    0x6b, 0xff, 0xd6,
    0x68, 0xf8, 0xd0,
  ],
  weight: [
    0xff, 0xff, 0xff,
    0xff, 0xff, 0xff,
    0xff, 0xff, 0xff,
  ],
}, {
  name: "equal4",
  desc: "Four options, equal odds.",
  w: 4,
  h: 1,
  weight: [ 0xff, 0xff, 0xff, 0xff ],
}, {
  name: "exp4",
  desc: "Four options, first much likelier.",
  w: 4,
  h: 1,
  weight: [ 0x80, 0x40, 0x20, 0x10 ],
}, {
  name: "exp8",
  desc: "Eight options, first much likelier.",
  w: 8,
  h: 1,
  weight: [ 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 ],
}];
