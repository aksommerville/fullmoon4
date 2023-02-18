/* Synthesizer.js
 */
 
import { Globals } from "../game/Globals.js";
import { Constants } from "../game/Constants.js";
import { DataService } from "../game/DataService.js";
import { AudioChannel } from "./AudioChannel.js";
import { AudioVoice } from "./AudioVoice.js";
import { SongPlayer } from "./SongPlayer.js";
 
export class Synthesizer {
  static getDependencies() {
    return [Window, Globals, Constants, DataService];
  }
  constructor(window, globals, constants, dataService) {
    this.window = window;
    this.globals = globals;
    this.constants = constants;
    this.dataService = dataService;
    
    this.context = null;
    this.channels = []; // Channels don't exist until configured.
    this.fqpids = []; // Parallel to (channels).
    this.voices = [];
    this.songPlayer = null;
    this.overrideInstrumentPid = null; // only fiddle should use
  }
  
  /* Entry points from Runtime.
   ********************************************************************/
  
  reset() {
    if (!this.context) {
      // This has to be done after the first user interaction, not during construction.
      if (this.window.AudioContext) {
        this.context = new this.window.AudioContext({
          sampleRate: this.constants.AUDIO_FRAME_RATE,
          latencyHint: "interactive",
        });
      } else {
        console.warn(`AudioContext not found. Will not produce audio.`);
        return;
      }
    }
    if (this.context.state === "suspended") {
      this.context.resume();
    }
    for (const voice of this.voices) voice.abort();
    for (const channel of this.channels) if (channel) channel.abort();
    this.voices = [];
    this.channels = [];
    this.fqpids = [];
    if (this.songPlayer) this.songPlayer.reset();
  }
  
  pause() {
    if (!this.context) return;
    if (this.songPlayer) this.songPlayer.releaseAll();
    this.context.suspend();
  }
  
  resume() {
    if (!this.context) return;
    if (this.songPlayer) this.songPlayer.resetClock();
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
  
  /* As a general rule, we are a MIDI receiver.
   * Some exceptions:
   *  - Channel 0x0f is reserved for sound effects. a=MSB, b=LSB. Note Off not necessary.
   *  - Songs and the game contend for the same 16 channels. No independent sub-busses.
   */
  event(chid, opcode, a, b) {
    //console.log(`Synthesizer event chid=${chid} opcode=${opcode} a=${a} b=${b}`);
    
    // Channel 15 is special, it's for sound effects.
    if (chid === 0x0f) {
      if (opcode !== 0x90) return;
      if (!this.context) return;
      this.startSoundEffect(a, b);
      return;
    }
  
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
    this.fqpids[chid] = fqpid;
  }
  
  requireChannel(chid) {
    if (chid < 0) return null;
    if (chid >= this.constants.AUDIO_CHANNEL_COUNT) return null;
    if (this.channels[chid]) return this.channels[chid];
    let fqpid = this.fqpids[chid];
    if (!fqpid) fqpid = 0;
    if (this.overrideInstrumentPid) fqpid = this.overrideInstrumentPid;
    const channel = this.instantiateChannel(chid, fqpid);
    this.channels[chid] = channel;
    return channel;
  }
  
  instantiateChannel(chid, pid) {
    let res = this.dataService.getInstrument(pid);
    if (!res) {
      res = this.dataService.getInstrument(1);
      if (!res) {
        console.log(`instrument:${pid} not found, and no fallback either. Using unconfigured default channel.`);
        return new AudioChannel(this, chid, pid, null);
      } else {
        console.log(`instrument:${pid} not found, substituting 1`);
      }
    } else {
      console.log(`using instrument:${pid} ${this.overrideInstrumentPid ? 'OVERRIDE' : ''}`);
    }
    const channel = new AudioChannel(this, chid, pid, res);
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
    for (const channel of this.channels) if (channel) channel.silence();
    for (const voice of this.voices) voice.abort();
    this.voices = [];
  }
  
  dropChannels() {
    for (const channel of this.channels) if (channel) channel.silence();
    this.channels = [];
  }
  
  startSoundEffect(note, velocity) {
    if (!this.context) return;
    const sfx = this.dataService.getSound(note);
    if (!sfx) return;
    const node = new AudioBufferSourceNode(this.context, {
      buffer: sfx.buffer,
      channelCount: 1,
    });
    node.connect(this.context.destination);
    node.start();
  }
  
  // For fiddle. If (pid) nonzero, every channel will use it and Program Change will be ignored.
  overrideAllInstruments(pid) {
    if (pid === this.overrideInstrumentPid) return;
    this.overrideInstrumentPid = pid;
    this.silenceAll();
    this.dropChannels();
  }
}

Synthesizer.singleton = true;
