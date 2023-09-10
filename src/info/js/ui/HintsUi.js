/* HintsUi.js
 * Panel content with advice on how to play.
 */
 
import { Dom } from "../util/Dom.js";

export class HintsUi {
  static getDependencies() {
    return [HTMLElement, Dom];
  }
  constructor(element, dom) {
    this.element = element;
    this.dom = dom;
    
    this.buildUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    
    this.dom.spawn(this.element, "H2", "General Advice");
    this.dom.spawn(this.element, "DIV", ["advice"], "Pencil and paper are recommended.");
    this.dom.spawn(this.element, "DIV", ["advice"], "Remember the Home Spell; you might need it if you get trapped.");
    this.dom.spawn(this.element, "DIV", ["advice"], "It's also always safe to End Game and then Continue -- you'll start at the nearest teleport point.");
    this.dom.spawn(this.element, "DIV", ["advice"], "Lost? Throw seeds on the ground and a bird will show you where to go.");
    this.dom.spawn(this.element, "DIV", ["advice"], "But take it with a grain of salt, I mean he's just a bird, what does he know?");
    
    this.dom.spawn(this.element, "H2", "For Non-Native English Speakers");
    this.dom.spawn(this.element, "DIV", ["advice"], "...hopefully you've managed to translate this page!");
    this.dom.spawn(this.element, "DIV", ["advice"], "You shouldn't need to understand English, except for these 12 words:");
    this.dom.spawn(this.dom.spawn(this.element, "DIV", ["spoiler"]), "IMG", { src: "./img/english-words.png" });
    this.dom.spawn(this.element, "DIV", ["advice"], "(actually you only need 6 of them, but I won't say which)");
    
    this.dom.spawn(this.element, "H2", "Do I actually need to do all the things?");
    this.dom.spawn(this.element, "DIV", ["advice"], "No.");
    this.dom.spawn(this.element, "DIV", ["advice"], "The demo can technically be completed with just two items.");
    this.dom.spawn(this.element, "DIV", ["advice"], "Full version, it's more like ten.");
    
    this.dom.spawn(this.element, "H2", "How many spells and songs are there?");
    this.dom.spawn(this.element, "DIV", ["advice"], "Three songs, seven teleport spells, and six other spells.");
    this.dom.spawn(this.element, "DIV", ["advice"], "As I'm writing this, there are actually six songs, but three of those aren't used.");
    this.dom.spawn(this.element, "DIV", ["advice"], "The music teacher can teach you all three songs.");
    this.dom.spawn(this.element, "DIV", ["advice"], "Spells are all over the place. Best place to start looking is the village.");
    
    this.dom.spawn(this.element, "H2", "How do I kill the werewolf???");
    this.dom.spawn(this.element, "DIV", ["advice"], "I'll never tell. But I promise it's possible. :)");
  }
}
