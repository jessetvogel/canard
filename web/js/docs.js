var definitions = null;
var documentation = null;

function initDocs() {
    // Load JSON data files
    Promise.all([
        requestGET('json/definitions.json'),
        requestGET('json/documentation.json'),
    ]).then(function (values) {
        definitions = JSON.parse(values[0]);
        documentation = JSON.parse(values[1]);
        checkURLHash();
    });

    // Loading icon while loading
    setHTML($('content'), '<div class="loading"></div>');
}

function gotoDoc(id) {
    const content = $('content');
    // Check if definition is valid
    if (!(id in definitions)) {
        setHTML(content, `No definition found for <span class="tt">${id}</span>`);
        return;
    }

    // If there is documentation, show it, otherwise try user-submitted documentation
    if (id in documentation) {
        showDoc(definitions[id], documentation[id]);
    }
    else {
        clear(content);
        content.append(createLoading());
        dbGet('documentation', ['documentation'], { 'identifier': id }).then(values => {
            const doc = (values.length == 0) ? 'No documentation found' : values.map(x => `<p>${x['documentation']}</p>`).join('<br/>');
            showDoc(definitions[id], doc);
        });
    }
}

function showDoc(definition, documentation) {
    // Add definition as header
    const content = $('content');
    clear(content);
    content.append(create('div', typeset(definition), { 'class': 'definition tt' }));

    // Add documentation as paragraph
    const p = create('div', documentation, { 'class': 'documentation' });
    content.append(p);
    MathJax.typeset([p]);
}

function checkURLHash() {
    const id = window.location.hash.substr(1);
    if (id != '') return gotoDoc(id);
    return showDocsOverview();
}

function showDocsOverview() {
    const content = $('content');
    clear(content);

    // Create search bar
    const search = create('input', '', { 'placeholder': 'Search for definition..', 'class': 'doc-search' });
    onInput(search, function () { searchDocOverview(search.value); });

    // Add to content
    content.append(search);

    // Sort all identifiers based on their namespace (more precisely, the initial namespace)
    const overview = create('div', '', { 'class': 'doc-overview' });
    const namespaces = {};
    for (let identifier in definitions) {
        const space = identifier.split('.')[0];
        if (!(space in namespaces)) namespaces[space] = [];
        namespaces[space].push(identifier);
    }

    const sections = Object.keys(namespaces);
    sections.sort();
    for (let space of sections) {
        // Namespace section title
        overview.append(create('span', space, { 'class': 'tt' }));
        // Create <ul> of definitions
        const ul = create('ul');
        namespaces[space].sort((a, b) => { return a.split('.').pop().localeCompare(b.split('.').pop()); });
        for (let id of namespaces[space])
            ul.append(create('li', `<span class="tt">${typeset(id)}</span>`, { 'data-id': id }));
        overview.append(ul);
    }
    content.append(overview);
}

function searchDocOverview(query) {
    for (let li of document.querySelectorAll('.doc-overview li')) {
        // Identifier with and without namespace
        const identifier = li.dataset.id;
        const final = identifier.split('.').pop();

        if (final.includes(query) || documentation[identifier]?.includes(query)) {
            li.style.display = '';
        } else {
            li.style.display = 'none';
        }
    }
}

window.onload = initDocs;
window.addEventListener('popstate', checkURLHash);
