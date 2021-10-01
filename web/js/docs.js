var definitions = null;
var documentation = null;

function initDocs() {
    // Load JSON data files
    Promise.all([
        requestGET('doc/definitions.json'),
        requestGET('doc/documentation.json'),
    ]).then(function (values) {
        definitions = JSON.parse(values[0]);
        documentation = JSON.parse(values[1]);
        checkURLHash();
    });

    // Loading icon while loading
    setHTML($('content'), '<div class="loading"></div>');
}

window.onload = initDocs;

function gotoDoc(id) {
    const content = $('content');
    // Check if definition is valid
    if (!(id in definitions)) {
        setHTML(content, `No definition found for <span class="tt">${id}</span>`);
        return;
    }

    // Add definition as header
    clear(content);
    content.append(create('div', typeset(definitions[id]), { 'class': 'definition tt' }));

    // Add documentation as paragraph
    const doc = (id in documentation) ? documentation[id] : 'No documentation found';
    const p = create('div', doc, {'class': 'documentation' });
    content.append(p);
    MathJax.typeset([p]);
}

function checkURLHash() {
    const id = window.location.hash.substr(1);
    if (id != '') return gotoDoc(id);
    return showDocsOverview();
}

function showDocsOverview() {
    const ul = create('ul');
    for (let id in definitions)
        ul.append(create('li', `<span class="tt">${typeset(id)}</span>`));
    const content = $('content');
    clear(content);
    content.append(ul);
}

window.addEventListener('popstate', checkURLHash);
