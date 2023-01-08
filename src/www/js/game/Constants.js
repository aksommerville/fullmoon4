/* Constants.js
 * You might expect it to be static, but I'm making it an injectable singleton in case some "constants" prove to be dynamic.
 */
 
export class Constants {
  static getDependencies() {
    return [];
  }
  constructor() {
  
    this.COLC = 20;
    this.ROWC = 12;
    
    // Arguably could live in Renderer instead, but i dunno...
    this.TILESIZE = 16;
    
  }
}

Constants.singleton = true;
