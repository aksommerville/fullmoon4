/* SoundEffects.js
 * Responsible for receiving requests by sound effect ID from the game, and delivering MIDI events to the Synthesizer accordingly.
 */
 
import { Synthesizer } from "./Synthesizer.js";

export class SoundEffects {
  static getDependencies() {
    return [Synthesizer, Window];
  }
  constructor(synthesizer, window) {
    this.synthesizer = synthesizer;
    this.window = window;
    
    /* Acoustically speaking, the minimum should be no lower than 50 ms.
     * That's about the point where a chain of impulses starts to sound like a hum.
     * The fastest I can tap a button is something like 100..150 ms.
     * With light testing, I find 100 ms is pretty agreeable. Very tweakable.
     */
    this.MINIMUM_SPACING_MS = 100;

    this.lastPlayTimeBySfxid = [];
  }
  
  play(sfxid, level) {
    if (this._tooFrequent(sfxid)) return;
    this.synthesizer.event(0x0f, 0x90, sfxid, level || 0xff);
  }
  
  /* Returns false and updates lastPlayTimeBySfxid if it's OK to play this sound effect.
   * If they're coming in too fast, we return true, meaning don't play it.
   */
  _tooFrequent(sfxid) {
    const lastTime = this.lastPlayTimeBySfxid[sfxid];
    if (!lastTime) { // first play, always kosher
      this.lastPlayTimeBySfxid[sfxid] = this.window.Date.now();
      return false;
    }
    const now = this.window.Date.now();
    const elapsedMs = now - lastTime;
    if (elapsedMs < 0) { // clock is broken. reset and allow play.
      this.lastPlayTimeBySfxid[sfxid] = now;
      return false;
    }
    if (elapsedMs < this.MINIMUM_SPACING_MS) {
      // Too frequent. Leave last time as is and reject.
      return true;
    }
    // Normal case, last play was far enough in the past.
    this.lastPlayTimeBySfxid[sfxid] = now;
    return false;
  }
}

SoundEffects.singleton = true;
