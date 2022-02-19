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

    // If there is already documentation, use that
    let doc;
    if (id in documentation) {
        doc = create('div', documentation[id]);
    } else {
        // If there is no documentation, try to get user-submitted documentation
        clear(content);
        content.append(createLoading());
        const response = await dbGet('documentation', ['id', 'documentation'], { 'identifier': id }) as { 'documentation': string }[];

        // Create editable div with text
        doc = create('div', (response.length == 0) ? 'No documentation found' : response[0]['documentation'], { 'data-edit': `documentation|documentation|{"identifier":"${id}"}` });
        makeEditable(doc);
    }
    MathJax.typeset([doc]);
    docsShow(id, doc);

    // Button to search for properties in case we are on a page about schemes
    if (definitions[id].endsWith(': algebraic_geometry.scheme.Scheme')) {
        const button = create('button', 'Search properties', { 'style': 'margin: 16px auto; display: block;' });
        onClick(button, () => { button.remove(); autoSearchProperties(id); });
        content.append(button);
    }
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
        const space = identifier.substring(0, identifier.lastIndexOf('.')); // identifier.split('.')[0];
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

function autoSearchProperties(id: string): void {
    // Determine dependencies and parameters from the definition
    const dependencies = definitions[id].replace(/[\w\. ]+/, '').replace(/: algebraic_geometry\.scheme\.Scheme$/, '');
    console.log(dependencies);
    const match = dependencies.match(/\(\w+/g);
    const parameters = (match == null) ? [] : match.map(s => s.substr(1));

    // Array of (property, query) pairs, to execute
    const queries = [];
    const properties: { [key: string]: string } = {
        "scheme.affine": "affine",
        "scheme.quasi_compact": "quasi-compact",
        "scheme.regular": "regular",
        "scheme.noetherian": "noetherian",
        "scheme.locally_noetherian": "locally noetherian",
        "scheme.reduced": "reduced",
        "scheme.irreducible": "irreducible",
        "scheme.cohen_macaulay": "cohen-macaulay",
        "scheme.excellent": "excellent",
        "scheme.separated": "separated",
        "scheme.quasi_separated": "quasi-separated",
        "scheme.jacobson": "jacobson",
        "scheme.normal": "normal",
        "scheme.integral": "integral",
        "scheme.finite_dimensional": "finite dimensional",
        "scheme.connected": "connected"
    };
    for (let P in properties) {
        queries.push([properties[P], `search (H ${dependencies} : ${P} (${id} ${parameters.join(' ')}))`, `search (H ${dependencies} : not (${P} (${id} ${parameters.join(' ')})))`]);
    }

    // Create a table of 'properties and proofs', whose proofs are dynamically loaded
    const table = create('table', '', { 'style': 'margin-top: 32px;' });
    const tr = create('tr');
    tr.append(create('th', 'Property', { 'style': 'width: 256px;' }), create('th', 'Proof'));
    table.append(tr);
    for (let query of queries) {
        const tr = create('tr');
        const tdProof = create('td', '', { 'class': 'tt' });
        tdProof.append('(searching)');
        const tdProperty = create('td', query[0], { 'style': 'text-align: center;' });
        tr.append(tdProperty, tdProof);
        table.append(tr);

        api(query[1]).then((response: Message) => {
            let success = false;
            if (response.status != 'success')
                setText(tdProof, `(${response.data})`);
            else if (response.data[0].status != 'success')
                setText(tdProof, `(${response.data[0].data})`);
            else if (response.data[0].data.length == 0)
                setText(tdProof, '(could not prove)');
            else success = true;

            if (success)
                return setHTML(tdProof, typeset(response.data[0].data[0]['H']));

            // If not succeeded, try the second query; try to disprove the statement
            api(query[2]).then((response: Message) => {
                let success = false;
                if (response.status != 'success')
                    setText(tdProof, `(${response.data})`);
                else if (response.data[0].status != 'success')
                    setText(tdProof, `(${response.data[0].data})`);
                else if (response.data[0].data.length == 0)
                    setText(tdProof, '(could not prove)');
                else success = true;

                if (success) {
                    setText(tdProperty, `not ${query[0]}`);
                    return setHTML(tdProof, typeset(response.data[0].data[0]['H']));
                }
            });
        });
    }
    $('content').append(table);
}

window.onload = docsInit;
window.addEventListener('popstate', checkURLHash);
