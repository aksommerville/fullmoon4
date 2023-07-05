/* verifyTileprops.js
 */
 
module.exports = function(serial, resources, ref/*(type,id)*/) {
  if (serial.length !== 256) throw new Error(`Tileprops length must be exactly 256, found ${serial.length}`);
  if (serial[0x0f] !== 3) throw new Error(`Tile 0x0f must be UNSHOVELLABLE(3), have cellphysics ${serial[0x0f]}`);
  return 0;
};
