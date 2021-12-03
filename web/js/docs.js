var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
var definitions = null;
var documentation = null;
function docsInit() {
    Promise.all([
        requestGET('json/definitions.json'),
        requestGET('json/documentation.json'),
    ]).then(function (values) {
        definitions = JSON.parse(values[0]);
        documentation = JSON.parse(values[1]);
        checkURLHash();
    });
    setHTML($('content'), '<div class="loading"></div>');
}
function docsGoto(id) {
    return __awaiter(this, void 0, void 0, function* () {
        const content = $('content');
        if (!(id in definitions)) {
            setHTML(content, `No definition found for <span class="tt">${id}</span>`);
            return;
        }
        let doc;
        if (id in documentation) {
            doc = create('div', documentation[id]);
        }
        else {
            clear(content);
            content.append(createLoading());
            const response = yield dbGet('documentation', ['id', 'documentation'], { 'identifier': id });
            doc = create('div', (response.length == 0) ? 'No documentation found' : response[0]['documentation'], { 'data-edit': `documentation|documentation|{"identifier":"${id}"}` });
            makeEditable(doc);
        }
        MathJax.typeset([doc]);
        docsShow(id, doc);
        if (definitions[id].endsWith(': algebraic_geometry.scheme.Scheme')) {
            const button = create('button', 'Search properties', { 'style': 'margin: 16px auto; display: block;' });
            onClick(button, () => { button.remove(); autoSearchProperties(id); });
            content.append(button);
        }
    });
}
function docsShow(id, doc) {
    const content = $('content');
    clear(content);
    content.append(create('div', typeset(definitions[id]), { 'class': 'definition tt' }));
    content.append(doc);
}
function checkURLHash() {
    const id = window.location.hash.substr(1);
    if (id != '')
        return docsGoto(id);
    return docsShowOverview();
}
function docsShowOverview() {
    const content = $('content');
    clear(content);
    const search = create('input', '', { 'placeholder': 'Search for definition..', 'class': 'doc-search' });
    onInput(search, function () { docsSearchOverview(search.value); });
    content.append(search);
    const overview = create('div', '', { 'class': 'doc-overview' });
    const namespaces = {};
    for (let identifier in definitions) {
        const space = identifier.split('.')[0];
        if (!(space in namespaces))
            namespaces[space] = [];
        namespaces[space].push(identifier);
    }
    const sections = Object.keys(namespaces);
    sections.sort();
    for (let space of sections) {
        overview.append(create('span', space, { 'class': 'tt' }));
        const ul = create('ul');
        namespaces[space].sort((a, b) => { return a.split('.').pop().localeCompare(b.split('.').pop()); });
        for (let id of namespaces[space])
            ul.append(create('li', `<span class="tt">${typeset(id)}</span>`, { 'data-id': id }));
        overview.append(ul);
    }
    content.append(overview);
}
function docsSearchOverview(query) {
    var _a;
    for (let li of document.querySelectorAll('.doc-overview li')) {
        const identifier = li.dataset.id;
        const final = identifier.split('.').pop();
        if (final.includes(query) || ((_a = documentation[identifier]) === null || _a === void 0 ? void 0 : _a.includes(query))) {
            li.style.display = '';
        }
        else {
            li.style.display = 'none';
        }
    }
}
function autoSearchProperties(id) {
    const dependencies = definitions[id].replace(/[\w\. ]+/, '').replace(/: algebraic_geometry\.scheme\.Scheme$/, '');
    console.log(dependencies);
    const match = dependencies.match(/\(\w+/g);
    const parameters = (match == null) ? [] : match.map(s => s.substr(1));
    const queries = [];
    const properties = {
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
        api(query[1]).then((response) => {
            let success = false;
            if (response.status != 'success')
                setText(tdProof, `(${response.data})`);
            else if (response.data[0].status != 'success')
                setText(tdProof, `(${response.data[0].data})`);
            else if (response.data[0].data.length == 0)
                setText(tdProof, '(could not prove)');
            else
                success = true;
            if (success)
                return setHTML(tdProof, typeset(response.data[0].data[0]['H']));
            api(query[2]).then((response) => {
                let success = false;
                if (response.status != 'success')
                    setText(tdProof, `(${response.data})`);
                else if (response.data[0].status != 'success')
                    setText(tdProof, `(${response.data[0].data})`);
                else if (response.data[0].data.length == 0)
                    setText(tdProof, '(could not prove)');
                else
                    success = true;
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
//# sourceMappingURL=docs.js.map