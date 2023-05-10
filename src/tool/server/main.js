#!/usr/bin/env node

const http = require("http");
const fs = require("fs");
const child_process = require("child_process");
const getSoundEffectIdByName = require("../common/getSoundEffectIdByName.js");

/* First, validate parameters.
 */

let host = "localhost";
let port = 8080;
let htdocs = "";
let makeable = [];
let midiBroadcast = false;
let htalias = []; // [requestPrefix, localPath]

for (const arg of process.argv.slice(2)) {
  if (arg.startsWith("--host=")) host = arg.substr(7);
  else if (arg.startsWith("--port=")) port = +arg.substr(7);
  else if (arg.startsWith("--htdocs=")) htdocs = arg.substr(9);
  else if (arg.startsWith("--makeable=")) makeable.push(arg.substr(11));
  else if (arg === "--midi-broadcast") midiBroadcast = true;
  else if (arg.startsWith("--htalias=")) htalias.push(arg.substr(10).split(':'));
  else {
    console.log(`Unexpected argument '${arg}'`);
    console.log(`Usage: ${process.argv[1]} [--host=localhost] [--port=8080] --htdocs=PATH [--makeable=PATH[:SERVE_AS]...] [--midi-broadcast] [--htalias=req:localpath...]`);
    process.exit(1);
  }
}

if (!htdocs) {
  console.log(`--htdocs required`);
  process.exit(1);
}

// You have to `npm i` to prep this.
const websocketLib = midiBroadcast ? require("websocket") : {};

/* Guess MIME type from path and content.
 */
 
function guessContentType(path, content) {
  const match = path.match(/\.([^.]*)$/);
  if (match) {
    const sfx = match[1].toLowerCase();
    switch (sfx) {
      case "png": return "image/png";
      case "css": return "text/css";
      case "html": return "text/html";
      case "js": return "application/javascript";
      case "wasm": return "application/wasm";
      case "json": return "application/json";
      case "mid": return "audio/midi";
      case "ico": return "image/icon";
    }
  }
  return "application/octet-stream";
}

/* Serve a regular file.
 */
 
function serveFile(rsp, path) {
  const content = fs.readFileSync(path);
  rsp.statusCode = 200;
  rsp.statusMessage = "OK";
  rsp.setHeader("Content-Type", guessContentType(path, content));
  rsp.end(content);
}

/* Invoke 'make' to freshen a file, resolve if it succeeds.
 */
 
function freshenMakeableFile(path) {
  const child = child_process.spawn("make", [path]);
  let log = "";
  return new Promise((resolve, reject) => {
    child.stdout.on("data", v => { log += v; });
    child.stderr.on("data", v => { log += v; });
    child.on("close", status => {
      if (status === 0) resolve();
      else reject({ log, status });
    });
  });
}

/* GET /api/soundNames
 */
 
function serveGetSoundNames(rsp) {
  rsp.setHeader("Content-Type", "application/json");
  rsp.end(JSON.stringify(getSoundEffectIdByName("$$toc$$")));
}

/* "/api/*" endpoints.
 */
 
function serveApi(req, rsp, path) {
  const name = req.method + path;
  switch (name) {
    case "GET/api/soundNames": return serveGetSoundNames(rsp);
  }
  throw new Error("Unknown API call");
}

/* Generate JSON from a freshMakeableFile error, to send to client.
 */
 
function makeErrorAsJson(error) {
  return JSON.stringify(error);
}

/* Set statusCode and statusMessage on a response, according to some exception.
 */
 
function setStatusForError(rsp, e) {
  try {
    if (e.code === "ENOENT") {
      rsp.statusCode = 404;
      rsp.statusMessage = "Not found";
      return;
    }
    throw null;
  } catch (ee) {
    rsp.statusCode = 500;
    rsp.statusMessage = "Internal server error";
  }
}

/* Create the server.
 */
 
const server = http.createServer((req, rsp) => {
  try {
    if (req.method !== "GET") {
      rsp.statusCode = 405;
      rsp.statusMessage = "Method not allowed";
      rsp.end();
      console.log(`405 ${req.method} ${req.url}`);
      return;
    }
    let path = req.url.split('?')[0];
    if (path === "/") path = "/index.html";
    if (!path.startsWith("/")) throw new Error("Invalid path");
    
    if (path.startsWith("/api/")) return serveApi(req, rsp, path);
    
    if (req.method === "GET") {
      for (const [requestPrefix, localPath] of htalias) {
        if (path.startsWith(requestPrefix)) {
          serveFile(rsp, `${localPath}${path.substring(requestPrefix.length)}`);
          console.log(`200 GET ${req.url}`);
          return;
        }
      }
    }
    
    // Things that get processed at build time have to be declared with "--makeable".
    // We invoke `make` every time somebody requests one.
    const mkbl = makeable.find(m => m.endsWith(path));
    if (mkbl) {
      const localPath = mkbl.split(':')[0];
      freshenMakeableFile(localPath).then(() => {
        serveFile(rsp, localPath);
        console.log(`200 GET ${req.url}`);
      }).catch(e => {
        console.log(`Failure from make for ${localPath}`);
        console.log(`555 GET ${req.url}`);
        rsp.statusCode = 555;
        rsp.statusMessage = "Make failure";
        rsp.setHeader("Content-Type", "application/json");
        rsp.end(makeErrorAsJson(e));
      });
      return;
    }
    
    serveFile(rsp, `${htdocs}${path}`);
    console.log(`200 GET ${req.url}`);
    
  } catch (e) {
    console.error(`Error serving ${req.method} ${req.url}`);
    console.error(e);
    setStatusForError(rsp, e);
    rsp.end();
    console.log(`${rsp.statusCode} ${req.method} ${req.url}`);
  }
});

server.listen(port, host, (error) => {
  if (error) {
    console.error(error);
  } else {
    console.log(`Listening on ${host}:${port}`);
  }
});

if (midiBroadcast) {
  const connections = [];
  const wsServer = new websocketLib.server({
    httpServer: server,
    autoAcceptConnections: true,
  });
  wsServer.on("connect", connection => {
    console.log(`WebSocket client connected.`);
    connections.push(connection);
    connection.on("message", (message) => {
      if (message.type === "binary") {
        for (const c of connections) {
          if (c === connection) continue;
          c.send(message.binaryData);
        }
      }
    });
  });
  wsServer.on("close", connection => {
    console.log(`WebSocket client disconnected.`);
    const p = connections.findIndex(c => c === connection);
    if (p >= 0) connections.splice(p, 1);
  });
  
  /* Another thing with midiBroadcast, we need to watch the underlying source file and notify clients when it changes.
   */
  fs.watch("src/data/instrument", (type, name) => {
    if (name === "WebAudio") {
      for (const connection of connections) {
        // A special SysEx event that our receivers will hopefully notice.
        connection.send(Buffer.from([0xf0, 0x12, 0x34, 0xf7]));
      }
    }
  });
}
