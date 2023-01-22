#!/usr/bin/env node

const http = require("http");
const fs = require("fs");
const child_process = require("child_process");

/* First, validate parameters.
 */

let host = "localhost";
let port = 8080;
let htdocs = "";
let dataPath = "";
let makeable = [];

for (const arg of process.argv.slice(2)) {
  if (arg.startsWith("--host=")) host = arg.substr(7);
  else if (arg.startsWith("--port=")) port = +arg.substr(7);
  else if (arg.startsWith("--htdocs=")) htdocs = arg.substr(9);
  else if (arg.startsWith("--data=")) dataPath = arg.substr(7);
  else if (arg.startsWith("--makeable=")) makeable.push(arg.substr(11));
  else {
    console.log(`Unexpected argument '${arg}'`);
    console.log(`Usage: ${process.argv[1]} [--host=localhost] [--port=8080] --htdocs=PATH --data=PATH [--makeable=PATH...]`);
    process.exit(1);
  }
}

if (!htdocs || !dataPath) {
  console.log(`--htdocs and --data required`);
  process.exit(1);
}

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
 
function serveFile(rsp, method, path, body) {
  switch (method) {
    case "GET": {
        let content = "";
        try {
          content = fs.readFileSync(path);
          rsp.setHeader("Content-Type", guessContentType(path, content));
        } catch (e) {
          if (e.code === "EISDIR") {
            content = JSON.stringify(fs.readdirSync(path));
            rsp.setHeader("Content-Type", "application/json");
          } else throw e;
        }
        rsp.statusCode = 200;
        rsp.statusMessage = "OK";
        rsp.end(content);
      } break;
    case "PUT": {
        fs.writeFileSync(path, body);
        rsp.statusCode = 200;
        rsp.statusMessage = "OK";
        rsp.end();
      } break;
    case "DELETE": {
        fs.unlinkSync(path);
        rsp.statusCode = 200;
        rsp.statusMessage = "OK";
        rsp.end();
      } break;
    default: {
        rsp.statusCode = 405;
        rsp.statusMessage = `Method ${method} not supported`;
        rsp.end();
      }
  }
}

/* GET /api/spriteControllers
 */
 
function serveSpriteControllers(rsp) {
  const controllers = [];
  const src = fs.readFileSync("src/app/sprite/fmn_sprite.h");
  for (let srcp=0, lineno=1; srcp<src.length; lineno++) {
    let nlp = src.indexOf(0x0a, srcp);
    if (nlp < 0) nlp = src.length;
    const line = src.toString("utf-8", srcp, nlp).trim();
    srcp = nlp + 1;
    
    const match = line.match(/^#define FMN_SPRCTL_([^\s]*)\s+([^\s]*)$/);
    if (!match) continue;
    const name = match[1];
    const id = +match[2];
    if (!name || isNaN(id) || (id < 0) || (id > 0xffff)) continue;
    controllers.push({ name, id });
  }
  rsp.statusCode = 200;
  rsp.setHeader("Content-Type", "application/json");
  rsp.end(JSON.stringify(controllers));
  return Promise.resolve();
}

/* Dispatch for "/api/" requests.
 * Return a Promise.
 */
 
function serveApi(rsp, req, path) {
  switch (req.method + path) {
  
    case "GET/api/spriteControllers": return serveSpriteControllers(rsp);
  
  }
  rsp.statusCode = 404;
  rsp.statusmessage = "Not found";
  rsp.end();
  return Promise.resolve();
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

/* Serve request.
 */
 
function serve(req, rsp) {
  try {
    let path = req.url.split('?')[0];
    if (path === "/") path = "/index.html";
    if (!path.startsWith("/")) throw new Error("Invalid path");
    
    if (path.startsWith("/data/")) {
      serveFile(rsp, req.method, `${dataPath}${path.substr(5)}`, req.body);
      console.log(`200 ${req.method} ${req.url}`);
      return;
    }
    
    if (path.startsWith("/api/")) {
      return serveApi(rsp, req, path)
        .catch(e => {
          console.error(`Error serving ${req.method} ${req.url}`);
          console.error(e);
          setStatusForError(rsp, e);
          rsp.end();
        })
        .then(() => console.log(`${rsp.statusCode} ${req.method} ${req.url}`))
    }
    
    if (req.method !== "GET") {
      rsp.statusCode = 405;
      rsp.statusMessage = "Method not allowed";
      rsp.end();
      console.log(`405 ${req.method} ${req.url}`);
      return;
    }
    serveFile(rsp, "GET", `${htdocs}${path}`, null);
    console.log(`200 GET ${req.url}`);
    
  } catch (e) {
    console.error(`Error serving ${req.method} ${req.url}`);
    console.error(e);
    setStatusForError(rsp, e);
    rsp.end();
    console.log(`${rsp.statusCode} ${req.method} ${req.url}`);
  }
}

/* Create the server.
 */
 
const server = http.createServer((req, rsp) => {
  req.body = "";
  req.on("data", d => req.body += d);
  req.on("end", () => serve(req, rsp));
});

server.listen(port, host, (error) => {
  if (error) {
    console.error(error);
  } else {
    console.log(`Listening on ${host}:${port}`);
  }
});
