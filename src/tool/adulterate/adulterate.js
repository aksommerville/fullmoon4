#!/usr/bin/env node
// adulterate.js: Interactive command-line tool for poking Full Moon save files.

const fs = require("fs");
const readline = require("readline");
const SavedGame = require("./SavedGame.js");

if (process.argv.length !== 3) {
  console.log(`Usage: ${process.argv[1]} SAVEFILE`);
  process.exit(1);
}

const SAVEFILE = process.argv[2];
const src = fs.readFileSync(SAVEFILE);
const savedGame = new SavedGame(src, SAVEFILE);

savedGame.validateSignature();
savedGame.decode();

const rl = readline.createInterface({ input: process.stdin, output: process.stdout });
rl.on("close", () => console.log(""));
promptForever(); 

//-----------------------------------------------------------
    
function printSummary() {
  console.log(`${SAVEFILE}: Signature OK, total ${savedGame.src.length} bytes`);
  console.log(`           spellid: ${savedGame.spellid} (${reprSpell(savedGame.spellid)})`);
  console.log(`      game_time_ms: ${savedGame.game_time_ms} (${reprGameTime(savedGame.game_time_ms)})`);
  console.log(`      damage_count: ${savedGame.damage_count}`);
  console.log(`transmogrification: ${savedGame.transmogrification} (${reprTransmogrification(savedGame.transmogrification)})`)
  console.log(`     selected_item: ${savedGame.selected_item} (${reprItem(savedGame.selected_item)})`);
  console.log(`             itemv: ${savedGame.itemv} (${reprItemBits(savedGame.itemv)})`);
  console.log(`            itemqv: ${JSON.stringify(Array.from(savedGame.itemqv))}`);
  console.log(`                  : pitcher=${reprPitcher(savedGame.itemqv[2])}`);
  console.log(`                  :   seeds=${savedGame.itemqv[3]}`);
  console.log(`                  :   coins=${savedGame.itemqv[4]}`);
  console.log(`                  : matches=${savedGame.itemqv[5]}`);
  console.log(`                  :  cheese=${savedGame.itemqv[15]}`);
  console.log(`                gs: ${savedGame.gs.length} bytes`); //TODO how to summarize?
  console.log(`            plants: ${savedGame.plants.length}`);
  console.log(`          sketches: ${savedGame.sketches.length}`);
  console.log(`              tail: ${savedGame.tail.length || 'none'}`);
  console.log(`    unknown chunks: ${savedGame.unknown.length}`);
}

function promptForever() {
  printSummary();
  const dirtyOption = savedGame.dirty ? ", (S)ave" : "";
  rl.question(`(Q)uit${dirtyOption}, 'key=value': `, v => processInstruction(v));
}

function processInstruction(v) {
  console.log("");
  const kv = v.match(/^\s*([^\s=]+)\s*=\s*([0-9a-zA-Z_]*)\s*$/);
  if (kv) {
    setKeyValue(kv[1], kv[2]);
    promptForever();
    return;
  }
  switch (v.trim()) {
    case "": promptForever(); break;
    case "Q": case "q": case "Quit": case "quit": process.exit(0); break;
    case "S": case "s": case "Save": case "save": saveFile(); promptForever(); break;
    //TODO gsbit details, and ought to read labels from src/data/gsbit
    //TODO plant details
    //TODO sketch details
    //TODO show tail?
    //TODO show unknown?
    default: {
        console.log(`!!! Invalid instruction: ${JSON.stringify(v)}`);
        promptForever();
      } break;
  }
}

function setKeyValue(k, v) {
  try {
    savedGame.setField(k, v);
    savedGame.dirty = true;
  } catch (e) {
    console.log(`Failed to set ${JSON.stringify(k)}: ${e.message}`);
  }
}

function saveFile() {
  savedGame.encode();
  fs.writeFileSync(SAVEFILE, savedGame.src);
  savedGame.dirty = false;
  console.log(`${SAVEFILE}: Saved.`);
}

//-----------------------------------------------------------

function reprSpell(spellid) {
  // Would be more correct to read FMN_SPELLID_* from src/app/fmn_platform.h, but meh.
  // Only the teleport spells matter.
  switch (spellid) {
    case 0: return "unset, equivalent to HOME";
    case 8: return "HOME:forest";
    case 9: return "TELE1:beach";
    case 10: return "TELE2:swamp";
    case 11: return "TELE3:mountains";
    case 12: return "TELE4:castle";
    case 22: return "TELE5:desert";
    case 23: return "TELE6:steppe";
  }
  return "INVALID!";
}

function reprGameTime(ms) {
  let s = Math.floor(ms / 1000); ms %= 1000;
  let m = Math.floor(s / 60); s %= 60;
  let h = Math.floor(m / 60); m %= 60;
  if (m < 10) m = "0" + m;
  if (s < 10) s = "0" + s;
  if (ms < 10) ms = "00" + ms;
  else if (ms < 100) ms = "0" + ms;
  return `${h}:${m}:${s}.${ms}`;
}

function reprTransmogrification(v) {
  switch (v) {
    case 0: return "normal";
    case 1: return "pumpkin";
  }
  return "UNKNOWN";
}

function reprItem(itemid) {
  // Would be more correct to read FMN_ITEM_* from src/app/fmn_platform.h, but meh.
  return [
    "snowglobe",
    "hat",
    "pitcher",
    "seed",
    "coin",
    "match",
    "broom",
    "wand",
    "umbrella",
    "feather",
    "shovel",
    "compass",
    "violin",
    "chalk",
    "bell",
    "cheese",
  ][itemid] || "INVALID!";
}

function reprItemBits(itemv) {
  const strings = [];
  for (let mask=1, itemid=0; itemid<16; mask<<=1, itemid++) {
    if (itemv & mask) strings.push(reprItem(itemid));
  }
  return strings.join(',');
}

function reprPitcher(content) {
  switch (content) {
    case 0: return "empty";
    case 1: return "water";
    case 2: return "milk";
    case 3: return "honey";
    case 4: return "sap";
  }
  return "UNKNOWN";
}
