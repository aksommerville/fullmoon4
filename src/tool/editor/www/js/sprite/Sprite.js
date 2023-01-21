/* Sprite.js
 */
 
export class Sprite {
  constructor(src) {
  
    // Each is an array of string, verbatim from the file. Comments and whitespace are discarded.
    this.commands = [];
    
    if (!src) ;
    else if (typeof(src) === "string") this.decode(src);
    else if (src instanceof Sprite) this.copy(src);
    else throw new Error(`Inappropriate input for Sprite`);
  }
  
  /* Copy, decode, encode. All trivial.
   *******************************************************************/
  
  copy(src) {
    this.commands = src.commands.map(cmd => [...cmd]);
  }
  
  decode(src) {
    for (let srcp=0, lineno=1; srcp<src.length; lineno++) {
      let nlp = src.indexOf("\n", srcp);
      if (nlp < 0) nlp = src.length;
      const line = src.substring(srcp, nlp).replace(/#.*$/, "").trim();
      srcp = nlp + 1;
      if (!line) continue;
      this.commands.push(line.split(/\s+/).filter(v => v));
    }
  }
  
  encode() {
    const serial = this.commands.reduce((a, v) => a + v.join(" ") + "\n", "");
    if (!serial) return "\n"; // don't let it be false
    return serial;
  }
  
  /* Access to fields.
   * You can read and write commands as a string. We will split or join with spaces.
   ********************************************************/
  
  // Returns the live command object (array of string), or null.
  findCommand(key, index=0) {
    for (const command of this.commands) {
      if (command[0] !== key) continue;
      if (index--) continue;
      return command;
    }
    return null;
  }
   
  getCommand(key) {
    const command = this.findCommand(key);
    if (!command) return this.defaultValueForCommand(key);
    return command.slice(1).join(" ");
  }
  
  setCommand(key, value) {
    const ncommand = [key, ...value.split(/\s+/).filter(v => v)];
    const command = this.commands.find(c => c[0] === key);
    if (command) {
      command.splice(0, command.length, ...ncommand);
    } else {
      this.commands.push(ncommand);
    }
  }
  
  defaultValueForCommand(key) {
    switch (key) {
      case "style": return "1"; // TILE
    }
    return "";
  }
}
