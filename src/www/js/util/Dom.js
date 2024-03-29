/* Dom.js
 */
 
import { Injector } from "./Injector.js";

export class Dom {
  static getDependencies() {
    return [Window, Document, Injector];
  }
  constructor(window, document, injector) {
    this.window = window;
    this.document = document;
    this.injector = injector;
    
    this.mutationObserver = new this.window.MutationObserver((events) => this.onMutations(events));
    this.mutationObserver.observe(this.document.body, { childList: true, subtree: true });
    
    this.window.addEventListener("keydown", (event) => {
      if (event.code === "Escape") {
        this.dismissModals();
        event.preventDefault();
        event.stopPropagation();
      }
    });
  }
  
  onMutations(records) {
    for (const record of records) {
      for (const parent of record.removedNodes || []) {
        this.forControllersInElement(parent, (controller, element) => {
          controller?.onRemoveFromDom?.();
          delete element._fmn_controller;
        });
      }
    }
  }
  
  forControllersInElement(element, cb) {
    if (element._fmn_controller) {
      cb(element._fmn_controller, element);
    }
    for (const child of element.children || []) {
      this.forControllersInElement(child, cb);
    }
  }
  
  /* (args) may contain:
   *  - string|number => innerText
   *  - array => CSS class names
   *  - {
   *      "on-*" => event listener
   *      "*" => attribute
   *    }
   * Anything else is an error.
   */
  spawn(parent, tagName, ...args) {
    const element = this.document.createElement(tagName);
    for (const arg of args) {
      switch (typeof(arg)) {
        case "string": case "number": element.innerText = arg; break;
        case "object": {
            if (arg instanceof Array) {
              for (const cls of arg) {
                element.classList.add(cls);
              }
            } else for (const k of Object.keys(arg)) {
              if (k.startsWith("on-")) {
                element.addEventListener(k.substr(3), arg[k]);
              } else {
                element.setAttribute(k, arg[k]);
              }
            }
          } break;
        default: throw new Error(`Unexpected argument ${arg}`);
      }
    }
    parent.appendChild(element);
    return element;
  }
  
  /* Same as spawn, but doesn't attach to a parent.
   * Please avoid document.createElement() on your own; we want a single point of contact if possible.
   */
  createElement(tagName, ...args) {
    return this.spawn({ appendChild: () => {} }, tagName, ...args);
  }
  
  spawnController(parent, clazz, overrides) {
    const element = this.spawn(parent, this.tagNameForControllerClass(clazz), [clazz.name]);
    const controller = this.injector.getInstance(clazz, [...(overrides || []), element]);
    element._fmn_controller = controller;
    return controller;
  }
  
  queryControllerClass(parent, clazz) {
    const child = parent.querySelector(`.${clazz.name}`);
    if (child._fmn_controller instanceof clazz) return child._fmn_controller;
    return null;
  }
  
  spawnModal(clazz, overrides) {
    const frame = this._spawnModalFrame();
    const controller = this.spawnController(frame, clazz, overrides);
    return controller;
  }
  
  getTopModalController() {
    const frames = Array.from(this.document.body.querySelectorAll(".modalFrame"));
    if (!frames.length) return null;
    const frame = frames[frames.length - 1];
    const element = frame.children[0];
    return element._fmn_controller;
  }
  
  tagNameForControllerClass(clazz) {
    for (const dcls of clazz.getDependencies?.() || []) {
      const match = dcls.name?.match(/^HTML(.*)Element$/);
      if (match) switch (match[1]) {
        // Unfortunately, the names of HTMLElement subclasses are not all verbatim tag names.
        case "": return "DIV";
        case "Div": return "DIV";
        case "Canvas": return "CANVAS";
        default: {
            console.log(`TODO: Unexpected HTMLElement subclass name '${match[1]}', returning 'DIV'`);
            return "DIV";
          }
      }
    }
    return "DIV";
  }
  
  dismissModals() {
    this.document.body.querySelector(".modalStack")?.remove();
  }
  
  popTopModal() {
    const frame = this.document.body.querySelector(".modalFrame:last-child");
    if (!frame) return;
    frame.remove();
    if (!this.document.body.querySelector(".modalFrame")) {
      this.dismissModals();
    } else {
      this._replaceModalBlotter();
    }
  }
  
  popModal(controller) {
    for (const frame of this.document.body.querySelectorAll(".modalFrame")) {
      if (Array.from(frame.children || []).find(e => e._fmn_controller === controller)) {
        frame.remove();
        if (!this.document.body.querySelector(".modalFrame")) {
          this.dismissModals();
        } else {
          this._replaceModalBlotter();
        }
        return;
      }
    }
    console.log(`failed to pop modal`, controller);
  }
  
  _spawnModalFrame() {
    const stack = this._requireModalStack();
    const frame = this.spawn(stack, "DIV", ["modalFrame"]);
    this._replaceModalBlotter();
    return frame;
  }
  
  _requireModalStack() {
    let stack = this.document.body.querySelector(".modalStack");
    if (!stack) {
      stack = this.spawn(this.document.body, "DIV", ["modalStack"]);
    }
    return stack;
  }
  
  _replaceModalBlotter() {
    const stack = this.document.body.querySelector(".modalStack");
    if (!stack) return;
    const frames = Array.from(stack.querySelectorAll(".modalFrame"));
    if (frames.length < 1) return;
    const topFrame = frames[frames.length - 1];
    let blotter = stack.querySelector(".modalBlotter");
    if (!blotter) blotter = this.createElement("DIV", ["modalBlotter"], {
      "on-mousedown": (event) => {
        this.popTopModal();
      },
    });
    stack.insertBefore(blotter, topFrame);
  }
}

Dom.singleton = true;
