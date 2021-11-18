declare var MathJax: any;

var definitions: { [id: string]: string } = null;
var documentation: { [id: string]: string } = null;

function docsInit(): void {
    // Load JSON data files
    Promise.all([
        requestGET('json/definitions.json'),
        requestGET('json/documentation.json'),
    ]).then(function (values: [string, string]) {
        definitions = JSON.parse(values[0]);
        documentation = JSON.parse(values[1]);
        checkURLHash();
    });

    // Loading icon while loading
    setHTML($('content'), '<div class="loading"></div>');
}

async function docsGoto(id: string) {
    const content = $('content');
    // Check if definition is valid
    if (!(id in definitions)) {
        setHTML(content, `No definition found for <span class="tt">${id}</span>`);
        return;
    }

    // If there is documentation, just show it
    if (id in documentation) {
        const doc = create('div', documentation[id]);
        MathJax.typeset([doc]);
        return docsShow(id, doc);
    }

    // If there is no documentation, try to get user-submitted documentation
    clear(content);
    content.append(createLoading());
    const response = await dbGet('documentation', ['id', 'documentation'], { 'identifier': id }) as { 'documentation': string }[];

    // Create editable div with text
    const doc = create('div', (response.length == 0) ? 'No documentation found' : response[0]['documentation'], { 'data-edit': `documentation|documentation|{"identifier":"${id}"}` });
    makeEditable(doc);
    MathJax.typeset([doc]);
    docsShow(id, doc);
}

function docsShow(id: string, doc: HTMLElement): void {
    // Add definition as header
    const content = $('content');
    clear(content);
    content.append(create('div', typeset(definitions[id]), { 'class': 'definition tt' }));
    content.append(doc);
    // MathJax.typeset([doc]);
}

function checkURLHash() {
    const id = window.location.hash.substr(1);
    if (id != '') return docsGoto(id);
    return docsShowOverview();
}

function docsShowOverview() {
    const content = $('content');
    clear(content);

    // Create search bar
    const search = create('input', '', { 'placeholder': 'Search for definition..', 'class': 'doc-search' }) as HTMLInputElement;
    onInput(search, function () { docsSearchOverview(search.value); });

    // Add to content
    content.append(search);

    // Sort all identifiers based on their namespace (more precisely, the initial namespace)
    const overview = create('div', '', { 'class': 'doc-overview' });
    const namespaces: { [space: string]: string[] } = {};
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

function docsSearchOverview(query: string): void {
    for (let li of document.querySelectorAll('.doc-overview li') as NodeListOf<HTMLElement>) {
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

window.onload = docsInit;
window.addEventListener('popstate', checkURLHash);
