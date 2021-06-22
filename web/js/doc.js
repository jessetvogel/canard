function init() {
    checkURLHash();
}

function navigate(id) {
    load(id);
    window.history.pushState(id, id, '#' + id);
}

function load(id) {
    // Check identifier
    if(!id.match(/(\w+\.)*\w+/g)) {
        console.warn('Tried to load invalid identifier `' + id + '`!');
        return;
    }

    // Put loading icon
    const content = $('#content');
    setHTML(content, '<div class="loading"></div>');

    // Make request
    const xhttp = new XMLHttpRequest();
    xhttp.open('GET', '/doc/' + id.replaceAll('.', '/') + '.html', true);
    xhttp.onload = function () {
        if(xhttp.status == 200) {
            setHTML(content, this.responseText);
            if(MathJax && MathJax.typeset)
                MathJax.typeset([ content ]);
        }
        else if(xhttp.status == 404)
            setHTML(content, '<div class="error">Doc not found 🥺</div>');
    }
    xhttp.onerror = function () {
        setHTML(content, '<div class="error">Could not load doc 🥺</div>');
    }
    xhttp.send();

    // Set header
    setText($('#header'), id);
}

function checkURLHash() {
    const id = window.location.hash.substr(1);
    if(id == '') {
        clear($('#content'));
        return;
    }
    load(id);
}

window.addEventListener('popstate', checkURLHash);
