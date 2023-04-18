const linewise = require("./linewise.js");

/* We could read them out of src/app/fmn_platform.h, similar to other evaluators.
 * But this set is not likely to change.
 */
 
const ITEM_NAMES = [
  "NONE",
  "HAT",
  "PITCHER",
  "SEED",
  "COIN",
  "MATCH",
  "BROOM",
  "WAND",
  "UMBRELLA",
  "FEATHER",
  "SHOVEL",
  "COMPASS",
  "VIOLIN",
  "CHALK",
  "BELL",
  "CHEESE",
];

function resolveItemName(src) {
  const p = ITEM_NAMES.indexOf(src);
  if (p < 0) throw new Error(`Unknown item name ${JSON.stringify(src)}`);
  return p;
}

module.exports = resolveItemName;
