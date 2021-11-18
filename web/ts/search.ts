const context: {
    objects: string[],
    types: { [obj: string]: string },
    properties: { [obj: string]: { [prop: string]: boolean } }
} = { objects: [], types: {}, properties: {} };

const types: { [type: string]: string[] } = {
    "Ring": [],
    "RingMorphism": ["Ring", "Ring"],
    "Module": ["Ring"],
    "ModuleMorphism": ["Module \\w+", "Module \\w+"],
    "Scheme": [],
    "SchemeMorphism": ["Scheme", "Scheme"],
    "Sheaf": ["Scheme"]
};

const properties: { [type: string]: { [prop: string]: string } } = {};

function getQueryHelper(): string {
    let query = '';
    let i = 0;
    for (let obj of context.objects) {
        // Object definition
        const type = context.types[obj].replace(/\b\w+Morphism\b/g, 'Morphism');; // All morphisms are Morphisms
        query += `(${obj} : ${type}) `;
        // Object properties
        for (let property in context.properties[obj])
            query += `(h${i++} : ${context.properties[obj][property] ? property + ' ' + obj : 'not (' + property + ' ' + obj + ')'}) `;
    }
    return query.slice(0, -1);
}

function getSearchQuery(): string {
    return `search ${getQueryHelper()}`;
}

function getProveQuery(conclusion: string): string {
    return `search (P ${getQueryHelper()} : ${conclusion})` + `search (P ${getQueryHelper()} : not (${conclusion}))`;
}

function getContradictionQuery(): string {
    return `search (P (True : Prop) ${getQueryHelper()} : not True)`;
}

// ------------------------------------------------------------

function contextAddObject(name: string, type: string): void {
    // Update context
    context.objects.push(name);
    context.types[name] = type;
    context.properties[name] = {};

    // Update overview
    const list = $('object-list');
    const div = create('div', `<span class="name math">${name}</span><span class="type"> : ${type}</span><span class="adjectives"></span>`);
    MathJax.typeset([div]);
    onClick(div, event => contextSelectObject(name));
    const deleteIcon = create('div', '', { 'class': 'delete' });
    onClick(deleteIcon, event => {
        contextRemoveObject(name);
        updateTypeArguments();
    });
    div.append(deleteIcon);
    list.append(div);
}

function contextRemoveObject(name: string): void {
    // Update context
    const i = context.objects.indexOf(name);
    context.objects.splice(i, 1);
    delete context.types[name];
    delete context.properties[name];

    for (let obj of context.objects) { // Remove objects depending (through type) on `name`
        if (context.types[obj].match(`\\b${name}\\b`))
            contextRemoveObject(obj);
    }

    // Update overview
    const list = $('object-list');
    for (let div of list.children) {
        if ((div as HTMLElement).innerText.startsWith(`${name} :`))
            div.remove();
    }

    // Clear properties, deselect all
    clear($('object-properties'));
    const selected = document.querySelector('object-list .selected') as HTMLElement;
    if (selected != null)
        removeClass(selected, 'selected');

    // Update type arguments, just in case
    updateTypeArguments();
}

// ------------------------------------------------------------

function contextSelectObject(name: string): void {
    // If object does not exist, just stop already
    if (!context.objects.includes(name))
        return;

    // Update overview
    const list = $('object-list') as HTMLElement;
    for (let div of list.children) {
        removeClass(div as HTMLElement, 'selected');
        if ((div as HTMLElement).innerText.startsWith(`${name} :`))
            addClass(div as HTMLElement, 'selected');
    }

    // Update properties
    const objectProperties = $('object-properties');
    clear(objectProperties);
    const typeBase = context.types[name].split(' ')[0];
    for (let key in properties[typeBase]) {
        const label = create('label', properties[typeBase][key]);
        if (key in context.properties[name])
            addClass(label, context.properties[name][key] ? 'yes' : 'no');
        onClick(label, event => {
            // Yes --> No
            if (hasClass(label, 'yes')) {
                removeClass(label, 'yes');
                addClass(label, 'no');
                context.properties[name][key] = false;
                // No --> None
            } else if (hasClass(label, 'no')) {
                removeClass(label, 'no');
                delete context.properties[name][key];
                // None --> Yes
            } else {
                addClass(label, 'yes');
                context.properties[name][key] = true;
            }
            updateAdjectives(name);
        });

        onContextMenu(label, event => {
            event.preventDefault();
            // Call API and show loading icon
            api(getProveQuery(`${key} ${name}`)).then(updateOutput).catch(error => alert(error));
            setHTML($('output'), '<div class="loading"></div>');
        });

        const div = create('div', '');
        div.append(label);
        objectProperties.append(div);
    }
}

// ------------------------------------------------------------
async function searchInit() {
    // Load JSON data files
    Object.assign(properties, JSON.parse(await requestGET('json/properties.json') as string));

    // Options for object type <select>
    const select = $('object-type');
    for (let type in types)
        select.append(create('option', type, { 'value': type }));

    // On change for object type, update type arguments
    const typeArguments = $('object-type-arguments');
    onChange(select, updateTypeArguments);

    // Click add object <button>
    const button = $('object-add');
    onClick(button, event => {
        const name = ($('object-name') as HTMLInputElement).value;
        const typeBase = ($('object-type') as HTMLInputElement).value;

        // Validate name
        if (!name.match(/^\w+$/)) {
            alert('Invalid object name');
            return;
        }

        // Check if name has already been used
        if (context.objects.includes(name)) {
            alert('Object name already used');
            return;
        }

        // Check type arguments
        const typeArguments = $('object-type-arguments');
        const arguments = [];
        for (let select of typeArguments.children)
            arguments.push((select as HTMLInputElement).value);

        // Construct type
        arguments.unshift(typeBase);
        const type = arguments.join(' ');

        // Add and select object
        contextAddObject(name, type);
        contextSelectObject(name);
    });

    // Click search <button>
    onClick($('button-search'), event => {
        // If there are no objects, just clear output
        if (context.objects.length == 0) {
            clear($('output'));
            alert('Please provide some data');
            return;
        }

        // Call API and show loading icon
        api(getSearchQuery()).then(updateOutput).catch(error => alert(error));
        setHTML($('output'), '<div class="loading"></div>');
    });

    // Click contradiction <button>
    onClick($('button-contradiction'), event => {
        // Call API and show loading icon
        api(getContradictionQuery()).then(updateOutput).catch(error => alert(error));
        setHTML($('output'), '<div class="loading"></div>');
    });
}

// ------------------------------------------------------------

function updateTypeArguments(): void {
    const arguments = types[($('object-type') as HTMLInputElement).value];
    const typeArguments = $('object-type-arguments');
    clear(typeArguments);

    // Case the type does not require arguments
    if (arguments.length == 0) {
        typeArguments.style.display = 'none';
        return;
    }

    // Case the type does require arguments:
    // Create <select>'s with objects whose type matches the provided regexes
    typeArguments.style.display = null;
    for (let rgx of arguments) {
        const selectArg = create('select', '');
        for (let obj of context.objects) {
            if (context.types[obj].match(`^${rgx}$`))
                selectArg.append(create('option', obj));
        }
        typeArguments.append(selectArg);
    }
}

function updateAdjectives(name: string): void {
    for (let div of $('object-list').children) {
        if ((div.querySelector('.name') as HTMLElement).innerText != name)
            continue;

        const span = div.querySelector('.adjectives') as HTMLElement;
        const adjectives = [];
        const typeBase = context.types[name].split(' ')[0];
        for (let key in context.properties[name])
            adjectives.push((context.properties[name][key] ? '' : 'not ') + properties[typeBase][key]);
        setText(span, adjectives.join(', '));
    }
}

function updateOutput(response: Message) {
    const output = $('output');
    clear(output);
    switch (response.status) {
        case 'error':
            setText($('output'), 'Error: ' + response.data);
            return;
        case 'fail':
            setText($('output'), 'Fail: ' + response.data);
            return;
        case 'success':
            for (const message of response.data)
                output.append(messageBox(message));
            return;
    }
}

window.onload = searchInit;
