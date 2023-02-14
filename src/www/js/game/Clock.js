/* Clock.js
 * Keeps track of elapsed game time.
 */
 
export class Clock {
  static getDependencies() {
    return [Window];
  }
  constructor(window) {
    this.window = window;
    
    /* The amount of time that an update advances the game clock are clamped to this range.
     * No matter how much real time elapsed, we report 1..50 inclusive.
     * If you update faster than 1000 Hz, the game will run fast. Slower than 20 Hz, it will run slow.
     * That skew is preferable to the unpredictable behavior we would get by trying to apply extremely long or short updates.
     * TODO Consider tightening this range, find some realistic test cases.
     */
    this.SHORTEST_UPDATE_MS = 1;
    this.LONGEST_UPDATE_MS = 50;
    
    /* When debugging, we advance game time by a uniform amount regardless of real time.
     * 16 ms is close to 60 Hz.
     */
    this.DEBUG_INTERVAL_MS = 16;
    
    this.reset(0);
  }
  
  /* Clear all volatile state and start from scratch.
   * (initialTime) is the elapsed time from a saved game, or zero if starting fresh.
   */
  reset(initialTime) {
    this.lastGameTime = ~~initialTime;
    this.lastRealTime = this.window.Date.now();
    this.paused = false;
    this.debugging = false;
  }
  
  /* Call when the game is running, each time you're about to update the core game.
   * We will return the absolute game time in ms, clamping to a a sane distance from the last update.
   * (except every 1200 hours or so when it overflows).
   */
  update() {
    if (this.paused) {
      this.paused = false;
      this.lastGameTime = this.window.Date.now();
    }
    if (this.debugging) {
      this.lastGameTime += this.DEBUG_INTERVAL_MS;
    } else {
      const now = this.window.Date.now();
      const elapsedReal = now - this.lastRealTime;
      const elapsedGame = Math.max(this.SHORTEST_UPDATE_MS, Math.min(this.LONGEST_UPDATE_MS, elapsedReal));
      this.lastRealTime = now;
      this.lastGameTime += elapsedGame;
    }
    return this.lastGameTime;
  }
  
  /* Call in place of update(), when the core game will not be updated.
   * We will discard whatever real time has elapsed since the last update.
   */
  skip() {
    if (!this.debugging) {
      this.lastRealTime = this.window.Date.now();
    }
  }
  
  /* pause/resume to notify when you expect a long delay before the next update.
   * eg Hard Pause from the user.
   * If you forget to pause, it's not the end of the world, but there will be one saturated frame when you update next.
   * If you forget to resume, it doesn't matter, we implicitly resume at updates.
   */
  pause() {
    if (this.paused) return;
    this.paused = true;
  }
  resume() {
    if (!this.paused) return;
    this.paused = false;
    this.lastRealTime = this.window.Date.now();
  }
  
  /* Enter a pause/step debug mode, where the meaning of time is somewhat unclear.
   * While debugging, each update advances the game clock by a sensible interval, the same amount every time.
   */
  debug() {
    if (this.debugging) return;
    this.debugging = true;
  }
  undebug() {
    if (!this.debugging) return;
    this.debugging = false;
    this.lastRealTime = this.window.Date.now();
  }
}

Clock.singleton = true;
