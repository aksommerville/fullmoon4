/* VumeterUi.js
 */
 
import { Dom } from "../util/Dom.js";

export class VumeterUi {
  static getDependencies() {
    return [HTMLCanvasElement, Dom];
  }
  constructor(element, dom) {
    this.element = element;
    this.dom = dom;
    
    this.samples = []; // {dur,lo,hi,rms} -- samples of the output window, these are not PCM samples
    this.viewWidthSeconds = 3.0;
    
    this.buildUi();
  }
  
  update(event) {
    //console.log(`VumeterUi.update`, event);
    if (event.dur > 0) {
      this.samples.push(event);
      const p = this.render();
      if (p > 0) this.samples.splice(0, p);
    }
  }
  
  end() {
    //console.log(`VumeterUi.end`);
  }
  
  buildUi() {
    this.element.innerHTML = "";
  }
  
  /* Renders the graph from scratch.
   * Returns the index of the first sample remaining in use.
   * Caller should drop everything before that, unless you're keeping them for sentimental reasons or something.
   */
  render() {
    const graph = this.generateGraph();
    const ctx = this.element.getContext("2d");
    ctx.fillStyle = "#000";
    ctx.fillRect(0, 0, this.element.width, this.element.height);
    
    const xscale = this.element.width / this.viewWidthSeconds;
    const yscale = this.element.height;
    
    ctx.beginPath();
    ctx.moveTo(-10, this.element.height);
    for (const [s, n] of graph.peak) ctx.lineTo(s * xscale, this.element.height - n * yscale);
    ctx.strokeStyle = "#f00";
    ctx.stroke();
    
    ctx.beginPath();
    ctx.moveTo(-10, this.element.height);
    for (const [s, n] of graph.rms) ctx.lineTo(s * xscale, this.element.height - n * yscale);
    ctx.strokeStyle = "#ff0";
    ctx.stroke();
    
    ctx.fillStyle = "#f00";
    ctx.fillText("peak", 0, 10);
    ctx.fillText(graph.peakSummary[0], 40, 10);
    ctx.fillText(graph.peakSummary[1], 80, 10);
    ctx.fillText(graph.peakSummary[2], 120, 10);
    
    ctx.fillStyle = "#ff0";
    ctx.fillText("rms", 0, 20);
    ctx.fillText(graph.rmsSummary[0], 40, 20);
    ctx.fillText(graph.rmsSummary[1], 80, 20);
    ctx.fillText(graph.rmsSummary[2], 120, 20);
    
    return graph.samplep;
  }
  
  /* Generate the view model from scratch.
   */
  generateGraph() {
    const graph = {
      samplep: 0, // index of first (this.samples) used, all before that can be dropped
      peak: [], // array of [x,y] in [seconds,-1..1]
      rms: [], // ''
      peakSummary: [], // [lo,hi,avg] for all visible points, normalized to 0..1000
      rmsSummary: [], // ''
    };
    const allPeaks = [];
    const allRms = [];
    let dur = 0;
    for (graph.samplep=this.samples.length-1; graph.samplep>=0; graph.samplep--) {
      const sample = this.samples[graph.samplep];
      dur += sample.dur;
      const peak = Math.max(-sample.lo, sample.hi) / 32678.0;
      graph.peak.splice(0, 0, [-dur, peak]);
      graph.rms.splice(0, 0, [-dur, sample.rms]);
      allPeaks.push(peak);
      allRms.push(sample.rms);
      if (dur >= this.viewWidthSeconds) break;
    }
    if (graph.peak.length > 0) {
      for (const pt of graph.peak) pt[0] += dur;
      for (const pt of graph.rms) pt[0] += dur;
      graph.peakSummary[0] = Math.round(Math.min(...allPeaks) * 1000);
      graph.peakSummary[1] = Math.round(Math.max(...allPeaks) * 1000);
      graph.peakSummary[2] = Math.round((allPeaks.reduce((a, v) => a + v, 0) * 1000) / allPeaks.length);
      graph.rmsSummary[0] = Math.round(Math.min(...allRms) * 1000);
      graph.rmsSummary[1] = Math.round(Math.max(...allRms) * 1000);
      graph.rmsSummary[2] = Math.round((allRms.reduce((a, v) => a + v, 0) * 1000) / allRms.length);
    }
    return graph;
  }
}
