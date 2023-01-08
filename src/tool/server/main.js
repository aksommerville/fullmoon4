#!/usr/bin/env node

const http = require("http");
const fs = require("fs");
const child_process = require("child_process");

/* First, validate parameters.
 */

let host = "localhost";
let port = 8080;
let htdocs = "";
let makeable = [];

for (const arg of process.argv.slice(2)) {
  if (arg.startsWith("--host=")) host = arg.substr(7);
  else if (arg.startsWith("--port=")) port = +arg.substr(7);
  else if (arg.startsWith("--htdocs=")) htdocs = arg.substr(9);
  else if (arg.startsWith("--makeable=")) makeable.push(arg.substr(11));
  else {
    console.log(`Unexpected argument '${arg}'`);
    console.log(`Usage: ${process.argv[1]} [--host=localhost] [--port=8080] --htdocs=PATH [--makeable=PATH...]`);
    process.exit(1);
  }
}

if (!htdocs) {
  console.log(`--htdocs required`);
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
    
    const localPath = makeable.find(m => m.endsWith(path));
    if (localPath) {
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
    //rsp.statusCode = 500;
    //rsp.statusMessage = "Internal server error";
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
