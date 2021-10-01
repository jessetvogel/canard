function $(id) {
    return document.getElementById(id);
}

function create(tag, content = '', attr = {}) {
    let elem = document.createElement(tag);
    elem.innerHTML = content;
    for (a in attr)
        elem.setAttribute(a, attr[a]);
    return elem;
}

function clear(elem) {
    elem.innerHTML = '';
}

function onClick(elem, f) {
    elem.addEventListener('click', f);
}

function onContextMenu(elem, f) {
    elem.addEventListener('contextmenu', f);
}

function onChange(elem, f) {
    elem.addEventListener('change', f);
}

function onInput(elem, f) {
    elem.addEventListener('input', f);
}

function onRightClick(elem, f) {
    elem.addEventListener('contextmenu', f);
}

function onKeyPress(elem, f) {
    elem.addEventListener('keypress', f);
}

function onKeyDown(elem, f) {
    elem.addEventListener('keydown', f);
}

function onKeyUp(elem, f) {
    elem.addEventListener('keyup', f);
}

function addClass(elem, c) {
    elem.classList.add(c);
}

function removeClass(elem, c) {
    elem.classList.remove(c);
}

function hasClass(elem, c) {
    return elem.classList.contains(c);
}

function setHTML(elem, html) {
    elem.innerHTML = html;
}

function setText(elem, text) {
    elem.innerText = text;
}

function requestGET(url) {
    return new Promise((resolve, reject) => {
        const xhr = new XMLHttpRequest();
        xhr.onload = function () { resolve(this.responseText); };
        xhr.onerror = reject;
        xhr.open('GET', url);
        xhr.send();
    });
}

function requestPOST(url, data) {
    return new Promise((resolve, reject) => {
        const xhr = new XMLHttpRequest();
        xhr.onload = function () { resolve(JSON.parse(this.responseText)); };
        xhr.onerror = reject;
        xhr.open('POST', url);
        xhr.setRequestHeader('Content-Type', 'application/json');
        xhr.send(data);
    });
}
