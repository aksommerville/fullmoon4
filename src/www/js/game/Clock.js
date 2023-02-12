/* Clock.js
 * Keeps track of elapsed game time.
 */
 
export class Clock {
  static getDependencies() {
    return [Window];
  }
  constructor(window) {
    this.window = window;
    
    this.reset();
  }
  
  // Drop everything and stop running.
  reset() {
    this.runtimeActive = false;
    this.pausedHard = true;
    this.pausedSoft = false;
    this.sessionStartTime = 0; // Real time when the runtime loaded.
    this.sessionPriorTime = 0; // Time on the saved game's clock when we started.
    this.hardPauseTime = this.window.Date.now(); // Real time when we entered a hard pause state.
    this.softPauseTime = this.hardPauseTime; // '' for soft pause
    this.forgottenTime = 0; // Tallies the length of pauses.
    this.gameTime = 0; // Most recent timestamp returned.
  }
  
  // Notify that the runtime is ready and running.
  runtimeLoaded() {
    this.runtimeActive = true;
    this.pausedHard = false;
    this.pausedSoft = false;
    this.sessionStartTime = this.window.Date.now();
    this.sessionPriorTime = 0;
    this.forgottenTime = 0;
    this.gameTime = this.sessionPriorTime;
  }
  
  // Tell us the saved game's time already elapsed.
  setPriorTime(time) {
    if (!this.runtimeActive) return;
    this.sessionPriorTime = time;
    this.gameTime = time;
  }
  
  hardPause() {
    if (this.pausedHard) return;
    this.pausedHard = true;
    this.hardPauseTime = this.window.Date.now();
  }
  
  hardResume() {
    if (!this.pausedHard) return;
    this.pausedHard = false;
    if (this.pausedSoft) {
      if (this.softPauseTime > this.hardPauseTime) {
        this.forgottenTime += this.softPauseTime - this.hardPauseTime;
      }
    } else {
      const now = this.window.Date.now();
      this.forgottenTime += now - this.hardPauseTime;
    }
  }
  
  softPause() {
    if (this.pausedSoft) return;
    this.pausedSoft = true;
    this.softPauseTime = this.window.Date.now();
  }
  
  softResume() {
    if (!this.pausedSoft) return;
    this.pausedSoft = false;
    if (this.pausedHard) {
      if (this.hardPauseTime > this.softPauseTime) {
        this.forgottenTime += this.hardPauseTime - this.softPauseTime;
      }
    } else {
      const now = this.window.Date.now();
      this.forgottenTime += now - this.softPauseTime;
    }
  }
  
  /* Returns absolute game time in ms, ready to deliver to app.
   * This advances in real time as warranted.
   */
  update() {
    if (this.pausedHard || this.pausedSoft) return this.gameTime;
    if (this.debugging) {
      const debugInterval = 16;
      this.gameTime += debugInterval;
      this.debugStartTime += debugInterval;
      return this.gameTime;
    }
    const now = this.window.Date.now();
    let nextGameTime = now - this.sessionStartTime - this.forgottenTime + this.sessionPriorTime;
    if (nextGameTime < this.gameTime) { // oops
      return this.gameTime;
    }
    this.gameTime = nextGameTime;
    return this.gameTime;
  }
  
  /* In "debug" mode, we suspend time and each update advances it by a fixed interval regardless of real time.
   */
  debug() {
    if (this.debugging) return;
    this.debugging = true;
    this.debugStartTime = this.window.Date.now();
  }
  undebug() {
    if (!this.debugging) return;
    this.debugging = false;
    const now = this.window.Date.now();
    this.forgottenTime += now - this.debugStartTime;
    this.nextGameTime = this.gameTime;
  }
}

Clock.singleton = true;
