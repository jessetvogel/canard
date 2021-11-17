function $(id: string): HTMLElement {
    return document.getElementById(id);
}

function create(tag: string, content = '', attr: { [k: string]: string } = {}) {
    let elem = document.createElement(tag);
    elem.innerHTML = content;
    for (let a in attr)
        elem.setAttribute(a, attr[a]);
    return elem;
}

function clear(elem: HTMLElement): void {
    elem.innerHTML = '';
}

function onClick(elem: HTMLElement, f: (this: HTMLElement, ev: MouseEvent) => any) {
    elem.addEventListener('click', f);
}

function onContextMenu(elem: HTMLElement, f: (this: HTMLElement, ev: MouseEvent) => any): void {
    elem.addEventListener('contextmenu', f);
}

function onChange(elem: HTMLElement, f: (this: HTMLElement, ev: Event) => any): void {
    elem.addEventListener('change', f);
}

function onInput(elem: HTMLElement, f: (this: HTMLElement, ev: Event) => any): void {
    elem.addEventListener('input', f);
}

function onRightClick(elem: HTMLElement, f: (this: HTMLElement, ev: MouseEvent) => any): void {
    elem.addEventListener('contextmenu', f);
}

function onKeyPress(elem: HTMLElement, f: (this: HTMLElement, ev: KeyboardEvent) => any): void {
    elem.addEventListener('keypress', f);
}

function onKeyDown(elem: HTMLElement, f: (this: HTMLElement, ev: KeyboardEvent) => any): void {
    elem.addEventListener('keydown', f);
}

function onKeyUp(elem: HTMLElement, f: (this: HTMLElement, ev: KeyboardEvent) => any): void {
    elem.addEventListener('keyup', f);
}

function addClass(elem: HTMLElement, c: string): void {
    elem.classList.add(c);
}

function removeClass(elem: HTMLElement, c: string): void {
    elem.classList.remove(c);
}

function hasClass(elem: HTMLElement, c: string): boolean {
    return elem.classList.contains(c);
}

function setHTML(elem: HTMLElement, html: string): void {
    elem.innerHTML = html;
}

function setText(elem: HTMLElement, text: string): void {
    elem.innerText = text;
}

function requestGET(url: string) {
    return new Promise((resolve, reject) => {
        const xhr = new XMLHttpRequest();
        xhr.onload = function () { resolve(this.responseText); };
        xhr.onerror = reject;
        xhr.open('GET', url);
        xhr.send();
    });
}

function requestPOST(url: string, data: string) {
    return new Promise((resolve, reject) => {
        const xhr = new XMLHttpRequest();
        xhr.onload = function () { resolve(this.responseText); };
        xhr.onerror = reject;
        xhr.open('POST', url);
        xhr.setRequestHeader('Content-Type', 'application/json');
        xhr.send(data);
    });
}
