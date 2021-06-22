function $(id) {
    let elements = document.querySelectorAll(id);
    if(elements.length == 0)
        return undefined;
    if(elements.length == 1)
        return elements[0];
    return elements;
}

function create(tag, content = '', attr = {}) {
    let elem = document.createElement(tag);
    elem.innerHTML = content;
    for(a in attr)
        elem.setAttribute(a, attr[a]);
    return elem;
}

function clear(elem) {
    elem.innerHTML = '';
}

function onClick(elem, f) {
    elem.addEventListener('click', f);
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
