/* Preferences.js
 * Singleton containing a database of user preferences.
 */
 
export class Preferences {
  static getDependencies() {
    return [Window];
  }
  constructor(window) {
    this.window = window;
    
    this.prefs = {
      musicEnable: true,
      scaling: "nearest", // "nearest" | "linear"
    };
    
    this.listeners = [];
    this.nextListenerId = 1;
  }
  
  // Same as listen, but also trigger the callback synchronously once.
  fetchAndListen(cb) {
    const id = this.listen(cb);
    cb(this.prefs);
    return id;
  }
  
  listen(cb) {
    const id = this.nextListenerId++;
    this.listeners.push({id, cb});
    return id;
  }
  
  unlisten(id) {
    const p = this.listeners.findIndex(l => l.id === id);
    if (p >= 0) {
      this.listeners.splice(p, 1);
    }
  }
  
  broadcast() {
    for (const {cb} of this.listeners) cb(this.prefs);
  }
  
  update(changes) {
    const newPrefs = this.mergePrefsIfChanged(this.prefs, changes);
    if (!newPrefs) return;
    this.prefs = newPrefs;
    this.broadcast();
  }
  
  // null if nothing changed, otherwise a new instance with (incoming) overriding (existing).
  mergePrefsIfChanged(existing, incoming) {
    if (!incoming) return null;
    let outgoing = null;
    for (const k of Object.keys(incoming)) {
      if (existing[k] === incoming[k]) continue;
      if (!outgoing) outgoing = {...existing};
      outgoing[k] = incoming[k];
    }
    return outgoing;
  }
  
  /* Caller supplies (prefs) because there are fields we don't manage.
   * (What jackass designed this thing...)
   * Roll (this.prefs) into a new object also containing the extra fields.
   * Runtime calls this.
   */
  save(prefs) {
    this.window.localStorage.setItem("settings", JSON.stringify(prefs));
  }
  
  /* Read prefs from localStorage but don't do anything with them.
   * If you want to keep them, call my 'update()' after.
   */
  load() {
    try {
      return JSON.parse(this.window.localStorage.getItem("settings")) || {};
    } catch (e) {
      return {};
    }
  }
}

Preferences.singleton = true;
