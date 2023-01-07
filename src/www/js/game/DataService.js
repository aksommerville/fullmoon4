/* DataService.js
 * Provides static assets and saved state.
 */
 
export class DataService {
  static getDependencies() {
    return [Window];
  }
  constructor(window) {
    this.window = window;
    
    this.images = []; // sparse, keyed by image id 0..255
  }
  
  loadImage(id) {
    let result = this.images[id];
    if (result) {
      if (result instanceof Promise) return result;
      return Promise.resolve(result);
    }
    result = new Promise((resolve, reject) => {
      const image = new this.window.Image();
      image.onload = () => {
        this.images[id] = image;
        resolve(image);
      };
      image.onerror = (error) => {
        console.error(`failed to load image ${id}`, error);
        this.images[id] = null;
        reject(error);
      };
      image.src = `/img/${id}.png`;
    });
    this.images[id] = result;
    return result;
  }
}

DataService.singleton = true;
