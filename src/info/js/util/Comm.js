/* Comm.js
 * Fetch wrapper.
 */
 
export class Comm {
  static getDependencies() {
    return [Window];
  }
  constructor(window) {
    this.window = window;
  }
  
  getText(url) {
    return this.window.fetch(url).then(rsp => {
      if (!rsp.ok) throw rsp;
      return rsp.text();
    });
  }
  
  getBinary(url) {
    return this.window.fetch(url).then(rsp => {
      if (!rsp.ok) throw rsp;
      return rsp.arraybuffer();
    });
  }
}

Comm.singleton = true;
