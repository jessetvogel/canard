var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
const context = { objects: [], types: {}, typeArgs: {}, properties: {}, tex: {} };
const types = {
    "Ring": [],
    "Ring morphism": ["Ring", "Ring"],
    "Module": ["Ring"],
    "Module morphism": ["Module \\w+", "Module \\w+"],
    "Scheme": [],
    "Scheme morphism": ["Scheme", "Scheme"],
    "Sheaf": ["Scheme"],
    "Monoid": [],
    "Monoid morphism": ["Monoid", "Monoid"],
    "Cone": [],
    "Fan": []
};
const properties = {};
function getQueryHelper() {
    let query = '';
    let i = 0;
    for (let obj of context.objects) {
        const typeBase = (context.types[obj].endsWith('morphism') ? 'Morphism' : context.types[obj]);
        const type = [typeBase].concat(context.typeArgs[obj]).join(' ');
        query += `(${obj} : ${type}) `;
        for (let property in context.properties[obj])
            query += `(h${i++} : ${context.properties[obj][property] ? property + ' ' + obj : 'not (' + property + ' ' + obj + ')'}) `;
    }
    return query.slice(0, -1);
}
function getSearchQuery() {
    return `search ${getQueryHelper()}`;
}
function getProveQuery(conclusion) {
    return `search (P ${getQueryHelper()} : ${conclusion})` + `search (P ${getQueryHelper()} : not (${conclusion}))`;
}
function getContradictionQuery() {
    return `search (P (True : Prop) ${getQueryHelper()} : not True)`;
}
function contextAddObject(name, type, args) {
    context.objects.push(name);
    context.types[name] = type;
    context.typeArgs[name] = args;
    context.properties[name] = {};
    const tex = name + ' : ' + ((type.endsWith('morphism')) ? `${args[0]} \\to ${args[1]}` : [`\\text{${type}}`].concat(args).join(' '));
    context.tex[name] = tex;
    const list = $('object-list');
    const div = create('div', `<span class="math">$${tex}$</span><span class="adjectives"></span>`, { 'data-name': name });
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
function contextRemoveObject(name) {
    const i = context.objects.indexOf(name);
    context.objects.splice(i, 1);
    delete context.types[name];
    delete context.typeArgs[name];
    delete context.properties[name];
    delete context.tex[name];
    for (let obj of context.objects) {
        if (context.types[obj].match(`\\b${name}\\b`))
            contextRemoveObject(obj);
    }
    const list = $('object-list');
    for (let div of list.children) {
        if (div.dataset['name'] == name)
            div.remove();
    }
    clear($('object-properties'));
    const selected = document.querySelector('object-list .selected');
    if (selected != null)
        removeClass(selected, 'selected');
    updateTypeArguments();
}
function contextSelectObject(name) {
    if (!context.objects.includes(name))
        return;
    const list = $('object-list');
    for (let div of list.children) {
        removeClass(div, 'selected');
        if (div.dataset['name'] == name)
            addClass(div, 'selected');
    }
    const objectProperties = $('object-properties');
    clear(objectProperties);
    const type = context.types[name];
    for (let key in properties[type]) {
        const label = create('label', properties[type][key]);
        if (key in context.properties[name])
            addClass(label, context.properties[name][key] ? 'yes' : 'no');
        onClick(label, event => {
            if (hasClass(label, 'yes')) {
                removeClass(label, 'yes');
                addClass(label, 'no');
                context.properties[name][key] = false;
            }
            else if (hasClass(label, 'no')) {
                removeClass(label, 'no');
                delete context.properties[name][key];
            }
            else {
                addClass(label, 'yes');
                context.properties[name][key] = true;
            }
            updateAdjectives(name);
        });
        onContextMenu(label, event => {
            event.preventDefault();
            api(getProveQuery(`${key} ${name}`)).then(updateOutput).catch(error => alert(error));
            setHTML($('output'), '<div class="loading"></div>');
        });
        const div = create('div', '');
        div.append(label);
        objectProperties.append(div);
    }
}
function searchInit() {
    return __awaiter(this, void 0, void 0, function* () {
        Object.assign(properties, JSON.parse(yield requestGET('json/properties.json')));
        const select = $('object-type');
        for (let type in types)
            select.append(create('option', type, { 'value': type }));
        const typeArguments = $('object-type-arguments');
        onChange(select, updateTypeArguments);
        const button = $('object-add');
        onClick(button, event => {
            const name = $('object-name').value;
            const type = $('object-type').value;
            if (!name.match(/^\w+$/)) {
                alert('Invalid object name');
                return;
            }
            if (context.objects.includes(name)) {
                alert('Object name already used');
                return;
            }
            const typeArguments = $('object-type-arguments');
            const arguments = [];
            for (let select of typeArguments.children) {
                const arg = select.value;
                if (arg == '') {
                    alert('Missing argument');
                    return;
                }
                arguments.push(arg);
            }
            contextAddObject(name, type, arguments);
            contextSelectObject(name);
        });
        onClick($('button-search'), event => {
            if (context.objects.length == 0) {
                clear($('output'));
                alert('Please provide some data');
                return;
            }
            api(getSearchQuery()).then(updateOutput).catch(error => alert(error));
            setHTML($('output'), '<div class="loading"></div>');
        });
        onClick($('button-contradiction'), event => {
            api(getContradictionQuery()).then(updateOutput).catch(error => alert(error));
            setHTML($('output'), '<div class="loading"></div>');
        });
    });
}
function updateTypeArguments() {
    const arguments = types[$('object-type').value];
    const typeArguments = $('object-type-arguments');
    clear(typeArguments);
    if (arguments.length == 0) {
        typeArguments.style.display = 'none';
        return;
    }
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
function updateAdjectives(name) {
    for (let div of $('object-list').children) {
        if (div.dataset['name'] != name)
            continue;
        const span = div.querySelector('.adjectives');
        setText(span, adjectivesString(name));
    }
}
function adjectivesString(name) {
    const adjectives = [];
    const type = context.types[name];
    for (let key in context.properties[name])
        adjectives.push((context.properties[name][key] ? '' : 'not ') + properties[type][key]);
    return adjectives.join(', ');
}
function updateOutput(response) {
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
            for (const message of response.data) {
                if (message.status == 'success' && message.data.length == 0) {
                    const template = encodeURIComponent(`Hey, I know an example:\n\n${contextString()}\nNamely, consider `);
                    output.append(create('div', `No solutions found. Do you know an example? Please <a target="_blank" href="propose.php?m=${template}">let us know</a>!`, { 'class': 'result' }));
                    continue;
                }
                output.append(messageBox(message));
            }
            return;
    }
}
window.onload = searchInit;
function contextString() {
    let str = '';
    for (const name in context.types) {
        const adjs = adjectivesString(name);
        str += `${name}: ${context.types[name]}${adjs.length == 0 ? '' : ' (' + adjs + ')'}\n`;
    }
    return str;
}
//# sourceMappingURL=search.js.map