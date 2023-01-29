/* SongPlayer.js
 * Coordinates playback of one song.
 * When the song changes, we kill its player and make a new one.
 */
 
export class SongPlayer {
  constructor(synthesizer, song) {
    this.synthesizer = synthesizer;
    this.song = song;
  }
  
  update() {
  }
  
  releaseAll() {
    //TODO
  }
}
