/* Synthesizer.js
 */
 
import { Globals } from "../game/Globals.js";
import { Constants } from "../game/Constants.js";
import { AudioChannel } from "./AudioChannel.js";
import { AudioVoice } from "./AudioVoice.js";
import { SongPlayer } from "./SongPlayer.js";
 
export class Synthesizer {
  static getDependencies() {
    return [Window, Globals, Constants];
  }
  constructor(window, globals, constants) {
    this.window = window;
    this.globals = globals;
    this.constants = constants;
    
    this.context = null;
    this.channels = []; // Channels don't exist until configured.
    this.fqpids = []; // Parallel to (channels).
    this.voices = [];
    this.songPlayer = null;
    if (this.window.AudioContext) {
      this.context = new this.window.AudioContext({
        sampleRate: this.constants.AUDIO_FRAME_RATE,
        latencyHint: "interactive",
      });
    } else {
      console.warn(`AudioContext not found. Will not produce audio.`);
    }
  }
  
  /* Entry points from Runtime.
   ********************************************************************/
  
  reset() {
    if (!this.context) return;
    if (this.context.state === "suspended") {
      this.context.resume();
    }
    for (const voice of this.voices) voice.abort();
    for (const channel of this.channels) channel.abort();
    this.voices = [];
    this.channels = []
  }
  
  pause() {
    if (!this.context) return;
    this.context.suspend();
  }
  
  resume() {
    if (!this.context) return;
    this.context.resume();
  }
  
  update() {
    if (!this.context) return;
    for (let i=this.voices.length; i-->0; ) {
      const voice = this.voices[i];
      if (voice.isFinished()) {
        voice.abort();
        this.voices.splice(i, 1);
      }
    }
    if (this.songPlayer) {
      try {
        this.songPlayer.update();
      } catch (e) {
        console.error(`error updating song`, e);
        this.songPlayer.releaseAll();
        this.songPlayer = null;
      }
    }
  }
  
  /* MIDI and MIDI-ish events.
   *************************************************************/
   
  playSong(song, force) {
    if (this.songPlayer) {
      if ((this.songPlayer.song === song) && !force) return;
      this.songPlayer.releaseAll();
      this.songPlayer = null;
    }
    if (!song) return;
    this.songPlayer = new SongPlayer(this, song);
  }
   
  event(chid, opcode, a, b) {
  
    // Some events, eg All Sound Off, do not target a channel.
    if ((chid < 0) || (chid >= this.constants.AUDIO_CHANNEL_COUNT)) {
      this.channellessEvent(opcode, a, b);
    
    // Program Change and Bank Select are managed at this level.
    } else if (opcode === 0xc0) {
      this.setPartialPid(chid, a, 0);
    } else if ((opcode === 0xb0) && (a === 0x00)) {
      this.setPartialPid(chid, b, 14);
    } else if ((opcode === 0xb0) && (a === 0x20)) {
      this.setPartialPid(chid, b, 7);
    
    // Note Off is managed at this level; channels not involved.
    } else if (opcode === 0x80) {
      this.releaseNote(chid, a, b);
    
    // Most commands, eg Note On, get processed by the channel.
    } else {
      const channel = this.requireChannel(chid);
      channel.event(opcode, a, b);
    }
  }
  
  channellessEvent(opcode, a, b) {
    switch (opcode) {
      case 0xff: this.silenceAll(); break;
    }
  }
  
  /* Program Change and Bank Select all contribute to the Fully Qualified Program ID.
   * When we get either of those, we update the fqpid and drop the live channel if present.
   * Channels are not instantiated until required.
   */
  setPartialPid(chid, v, shift) {
    if (this.channels[chid]) {
      const channel = this.channels[chid];
      this.channels[chid] = null;
      for (const voice of this.voices) {
        if (voice.channel === channel) {
          voice.release(0x40);
        }
      }
    }
    let fqpid = this.fqpids[chid] || 0;
    fqpid &= ~(0x7f << shift);
    fqpid |= v << shift;
  }
  
  requireChannel(chid) {
    if (chid < 0) return null;
    if (chid >= this.constants.AUDIO_CHANNEL_COUNT) return null;
    if (this.channels[chid]) return this.channels[chid];
    const channel = this.instantiateChannel(chid, this.fqpids[chid] || 0);
    this.channels[chid] = channel;
    return channel;
  }
  
  instantiateChannel(chid, pid) {
    const channel = new AudioChannel(this, chid, pid);
    return channel;
  }
  
  releaseNote(chid, noteId, velocity) {
    for (const voice of this.voices) {
      if (voice.chid !== chid) continue;
      if (voice.noteId !== noteId) continue;
      voice.release(velocity);
    }
  }
  
  silenceAll() {
    for (const channel of this.channels) channel.silence();
    for (const voice of this.voices) voice.abort();
    this.voices = [];
  }
}

Synthesizer.singleton = true;
