/* MapService.js
 * Your one-stop shop for all things FullmoonMap.
 */
 
import { FullmoonMap } from "./FullmoonMap.js";
 
export class MapService {
  static getDependencies() {
    return [];
  }
  constructor() {
  }
  
  /* Decode.
   ***********************************************************/
   
  decode(serial, id) {
    const map = new FullmoonMap(serial);
    map.id = id || 0;
    return map;
  }
  
  /* Encode.
   *******************************************************/
   
  encode(map) {
    return map.encode();
  }
}

MapService.singleton = true;
