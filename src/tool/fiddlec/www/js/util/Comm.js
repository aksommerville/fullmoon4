export class Comm {
  static getDependencies() {
    return [Window];
  }
  constructor(window) {
    this.window = window;
  }
  
  /* HTTP client, public interface.
   *********************************************************************/
  
  get(url, query) {
    return this.http("GET", url, query);
  }
  
  post(url, query, headers, body) {
    return this.http("POST", url, query, headers, body);
  }
  
  http(method, url, query, headers, body) {
    url = this._prepareUrl(url, query);
    const options = { method };
    if (headers) options.headers = headers;
    if (body) options.boody = body;
    return this.window.fetch(url, options).then(rsp => {
      if (!rsp.ok) throw rsp;
      return rsp;
    });
  }
  
  /* WebSocket.
   *****************************************************************/
   
  connectWebsocket(url) {
    if (!this.window.WebSocket) throw new Error(`WebSocket not supported`);
    if (url.startsWith("//") || url.startsWith("ws://") || url.startsWith("wss://")) ;
    else if (!url.startsWith("/")) throw new Error(`Invalid URL: ${url}`);
    else url = "ws://" + this.window.location.host + url;
    return new this.window.WebSocket(url);
  }
  
  /* HTTP client, private bits.
   *******************************************************************/
   
  _prepareUrl(url, query) {
    // Prepend host name if it wasn't provided.
    if (url.startsWith("//")) ;
    else if (url.startsWith("http://")) ;
    else if (url.startsWith("https://")) ;
    else if (!url.startsWith("/")) throw new Error(`Invalid URL: ${url}`);
    else url = this.window.location.origin + url;
    // Append query if provided.
    if (query) {
      let sep = '?';
      for (const k of Object.keys(query)) {
        url += `${sep}${encodeURIComponent(k)}=${encodeURIComponent(query[k])}`;
        sep = '&';
      }
    }
    return url;
  }
}

Comm.singleton = true;
