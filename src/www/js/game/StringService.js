/* StringService.js
 */
 
export class StringService {
  static getDependencies() {
    return [];
  }
  constructor() {
  }
  
  getStringById(stringId) {
    return `(STRING#${stringId})`;
  }
}

StringService.singleton = true;
