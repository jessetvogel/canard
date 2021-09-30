const context = {
    'objects': [],
    'types': {},
    'properties': {}
};

const types = {
    'Ring': [],
    'RingMorphism': [ 'Ring', 'Ring' ],
    'Module': [ 'Ring' ],
    'ModuleMorphism': [ 'Module \\w+', 'Module \\w+' ],
    'Scheme': [],
    'SchemeMorphism': [ 'Scheme', 'Scheme' ],
    'Sheaf': [ 'Scheme' ],
};

const properties = {
    'Ring': {
        'ring.field': 'field',
        'ring.domain': 'domain',
        'ring.noetherian': 'noetherian',
        'ring.reduced': 'reduced',
        'ring.artin': 'artin',
        'ring.gorenstein': 'Gorenstein',
        'ring.local': 'local',
        'ring.euclidean': 'Euclidean',
        'ring.valuation': 'valuation ring',
        'ring.finite': 'finite',
        'ring.dedekind': 'Dedekind',
        'ring.pid': 'principal ideal domain',
        'ring.ufd': 'unique factorization domain',
        'ring.dvr': 'discrete valuation ring',
        'ring.integrally_closed': 'integrally closed',
    },
    'Ring Morphism': {
        
    },
    'Module': {
        'module.faithful': 'faithful',
        'module.simple': 'simple',
        'module.cyclic': 'cyclic',
        'module.finitely_generated': 'finitely generated',
        'module.flat': 'flat',
        'module.free': 'free',
        'module.projective': 'projective',
        'module.injective': 'injective',
    },
    'Scheme': {
        'scheme.affine': 'affine',
        'scheme.quasi_compact': 'quasi-compact',
        'scheme.regular': 'regular',
        'scheme.noetherian': 'noetherian',
        'scheme.locally_noetherian': 'locally noetherian',
        'scheme.reduced': 'reduced',
        'scheme.irreducible': 'irreducible',
        'scheme.cohen_macaulay': 'cohen-macaulay',
        'scheme.excellent': 'excellent',
        'scheme.separated': 'separated',
        'scheme.quasi_separated': 'quasi-separated',
        'scheme.jacobson': 'jacobson',
        'scheme.normal': 'normal',
        'scheme.integral': 'integral',
        'scheme.finite_dimensional': 'finite dimensional',
        'scheme.connected' : 'connected'
    },
    'SchemeMorphism': {
        'morphism.formally_etale': 'formally étale',
        'morphism.formally_unramified': 'formally unramified',
        'morphism.formally_smooth': 'formally smooth',
        'morphism.proper': 'proper',
        'morphism.finite': 'finite',
        'morphism.quasi_finite': 'quasi-finite',
        'morphism.flat': 'flat',
        'morphism.affine': 'affine',
        'morphism.quasi_compact': 'quasi-compact',
        'morphism.separated': 'separated',
        'morphism.quasi_separated': 'quasi-separated',
        'morphism.regular': 'regular',
        'morphism.finitely_presented': 'finitely-presented',
        'morphism.locally_finite_type': 'locally finite type',
        'morphism.locally_finitely_presented': 'locally finitely presented',
        'morphism.etale': 'étale',
        'morphism.smooth': 'smooth',
        'morphism.unramified': 'unramified',
        'morphism.finite_type': 'finite type',
        'morphism.open_morphism': 'open morphism',
        'morphism.universally_closed': 'universally closed',
        'morphism.immersion': 'immersion',
        'morphism.open_immersion': 'open immersion',
        'morphism.closed_immersion': 'closed immersion',
        'morphism.finite_fibers': 'finite fibers',
        'morphism.surjective': 'surjective',
        'morphism.zariski_cover': 'Zariski cover',
        'morphism.etale_cover': 'etale cover',
        'morphism.smooth_cover': 'smooth cover',
        'morphism.syntomic_cover': 'syntomic cover',
        'morphism.fppf_cover': 'fppf cover',
        'morphism.fpqc_cover': 'fpqc cover',
        'morphism.faithfully_flat': 'faithfully flat'
    },
    'Sheaf': {
        'sheaf.quasi_coherent': 'quasi-coherent',
        'sheaf.coherent': 'coherent',
        'sheaf.locally_free': 'locally free',
        'sheaf.free': 'free',
    }
};

// ------------------------------------------------------------

function toQuery(context) {
    let query = '';
    let i = 0;
    for(let obj of context.objects) {
        // Object definition
        const type = context.types[obj].replace(/\b\w+Morphism\b/g, 'Morphism');; // All morphisms are Morphisms
        query += `(${obj} : ${type}) `;
        // Object properties
        for(let property in context.properties[obj])
            query += `(h${i++} : ${context.properties[obj][property] ? property + ' ' + obj : 'not (' + property + ' ' + obj + ')'}) `;
    }
    return query.slice(0, -1);
}

function toSearchQuery(context) {
    return `search ${toQuery(context)}`;
}

function toProveQuery(context, conclusion) {
    return `search (P ${toQuery(context)} : ${conclusion})` + `search (P ${toQuery(context)} : not (${conclusion}))`;
}

function toContradictionQuery(context) {
    return `search (P (True : Prop) ${toQuery(context)} : not True)`;
}

// ------------------------------------------------------------

function addObject(name, type) {
    // Update context
    context.objects.push(name);
    context.types[name] = type;
    context.properties[name] = {};

    // Update overview
    const list = $('#object-list');
    const div = create('div', `<span class="name math">${name}</span><span class="type"> : ${type}</span><span class="adjectives"></span>`);
    MathJax.typeset([ div ]);
    onClick(div, event => selectObject(name));
    const deleteIcon = create('div', '', { 'class': 'delete' });
    onClick(deleteIcon, event => {
        deleteObject(name);
        updateTypeArguments();
    });
    div.append(deleteIcon);
    list.append(div);
}

function deleteObject(name) {
    // Update context
    const i = context.objects.indexOf(name);
    context.objects.splice(i, 1);
    delete context.types[name];
    delete context.properties[name];

    for(let obj of context.objects) { // Remove objects depending (through type) on `name`
        if(context.types[obj].match(`\\b${name}\\b`))
            deleteObject(obj);
    }

    // Update overview
    const list = $('#object-list');
    for(let div of list.children) {
        if(div.innerText.startsWith(`${name} :`))
            div.remove();
    }

    // Clear properties, deselect all
    clear($('#object-properties'));
    if($('#object-list .selected') != null)
        removeClass($('#object-list .selected'), 'selected');

    // Update type arguments, just in case
    updateTypeArguments();
}

// ------------------------------------------------------------

function selectObject(name) {
    // If object does not exist, just stop already
    if(!context.objects.includes(name))
        return;

    // Update overview
    const list = $('#object-list');
    for(let div of list.children) {
        removeClass(div, 'selected');
        if(div.innerText.startsWith(`${name} :`))
            addClass(div, 'selected');
    }

    // Update properties
    const objectProperties = $('#object-properties');
    clear(objectProperties);
    const typeBase = context.types[name].split(' ')[0];
    for(let key in properties[typeBase]) {
        const label = create('label', properties[typeBase][key]);
        if(key in context.properties[name])
            addClass(label, context.properties[name][key] ? 'yes' : 'no');
        onClick(label, event => {
            // Yes --> No
            if(hasClass(label, 'yes')) {
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
            const query = toProveQuery(context, `${key} ${name}`);
            api(query).then(response => {
                switch(response.status) {
                    case 'error':
                        setText($('#output'), 'Error: ' + response.message);
                        return;
                    case 'fail':
                        setText($('#output'), 'Fail: ' + response.data);
                        return;
                    case 'success':
                        let output = '';
                        for(let message of response.data) {
                            const solutions = message.data;
                            if(solutions.length == 0) {
                                output += 'No solutions found..</br>';
                                continue;
                            }
                            for(let solution of solutions) {
                                for(let X in solution)
                                    output += '<span style="font-size: 1.25rem;" class="tt"><span style="color: rgba(0, 0, 0, 0.5);">' + X + '</span>: '+ typeset(solution[X]) + '</span><br/>';
                            }
                        }
                        setHTML($('#output'), output);
                        return;
                }
            }).catch(message => {
                alert('Fail: ' + message);
            });

            // Loading icon
            setHTML($('#output'), '<div class="loading"></div>');
        });

        const div = create('div', '');
        div.append(label);
        objectProperties.append(div);
    }
}

// ------------------------------------------------------------

function init() {
    // Options for object type <select>
    const select = $('#object-type');
    for(let type in types)
        select.append(create('option', type, { 'value': type }));

    // On change for object type, update type arguments
    const typeArguments = $('#object-type-arguments');
    onChange(select, updateTypeArguments);

    // Click add object <button>
    const button = $('#object-add');
    onClick(button, event => {
        const name = $('#object-name').value;
        const typeBase = $('#object-type').value;

        // Validate name
        if(!name.match(/^\w+$/)) {
            alert('Invalid object name');
            return;
        }

        // Check if name has already been used
        if(context.objects.includes(name)) {
            alert('Object name already used');
            return;
        }

        // Check type arguments
        const typeArguments = $('#object-type-arguments');
        const arguments = [];
        for(let select of typeArguments.children)
            arguments.push(select.value);
        
        // Construct type
        arguments.unshift(typeBase);
        const type = arguments.join(' ');

        // Add and select object
        addObject(name, type);
        selectObject(name);
    });

    // Click search <button>
    onClick($('#button-search'), event => {
        // If there are no objects, just clear output
        if(context.objects.length == 0) {
            clear($('#output'));
            alert('Please provide some data');
            return;
        }

        // Construct query and make API call
        const query = toSearchQuery(context);
        api(query).then(response => {
            switch(response.status) {
                case 'error':
                    setText($('#output'), 'Error: ' + response.message);
                    return;
                case 'fail':
                    setText($('#output'), 'Fail: ' + response.data);
                    return;
                case 'success':
                    let output = '';
                    for(const message of response.data) {
                        if(message.status == 'error')
                            output += `<div class="result error">Error: ${message.data}</div>`;
                        if(message.status == 'fail')
                            output += `<div class="result error">Fail: ${message.data}</div>`;
                        if(message.status == 'success') {
                            const solutions = message.data;
                            if(solutions.length == 0) {
                                output = 'No solutions found..';
                                break;
                            }
                            output += '<div class="result">';
                            for(let solution of solutions) {
                                for(let X in solution)
                                    output += '<span style="font-size: 1.125rem;" class="tt"><span style="color: rgba(0, 0, 0, 0.5);">' + X + '</span>: '+ typeset(solution[X]) + '</span><br/>';
                            }
                            output += '</div>';
                        }
                    }
                    setHTML($('#output'), output);
                    return;
            }
        }).catch(message => {
            alert(message);
        });

        // Set loading icon
        setHTML($('#output'), '<div class="loading"></div>');
    });

    // Click contradiction <button>
    onClick($('#button-contradiction'), event => {
        const query = toContradictionQuery(context);
        console.log(query);

        api(query).then(response => {
            switch(response.status) {
                case 'error':
                    setText($('#output'), 'Error: ' + response.message);
                    return;
                case 'fail':
                    setText($('#output'), 'Fail: ' + response.data);
                    return;
                case 'success':
                    const solutions = response.data[0].data;
                    if(solutions.length == 0) {
                        setText($('#output'), 'No solutions found..');
                        return;
                    }
                    
                    let output = '';
                    for(let solution of solutions) {
                        for(let X in solution)
                            output += '<span style="font-size: 1.125rem;" class="tt"><span style="color: rgba(0, 0, 0, 0.5);">' + X + '</span>: '+ typeset(solution[X]) + '</span><br/>';
                    }
                    setHTML($('#output'), output);
                    return;
            }
        }).catch(message => {
            alert(message);
        });

        // Set loading icon
        setHTML($('#output'), '<div class="loading"></div>');
    });
}

// ------------------------------------------------------------

function typeset(expression) {
    return expression.replace(
        new RegExp(/(\w+(?:\.\w+)+)/, 'g'),
        function($1) {
            const i = $1.lastIndexOf('.');
            const namespace = $1.substr(0, i);
            const identifier = $1.substr(i + 1);
            return `<a href="doc.html#${$1}" target="_blank">${identifier}</a>`;
    });
}

// ------------------------------------------------------------

function updateTypeArguments() {
    const arguments = types[$('#object-type').value];
    const typeArguments = $('#object-type-arguments');
    clear(typeArguments);

    // Case the type does not require arguments
    if(arguments.length == 0) {
        typeArguments.style.display = 'none';
        return;
    }

    // Case the type does require arguments:
    // Create <select>'s with objects whose type matches the provided regexes
    typeArguments.style.display = null;
    for(let rgx of arguments) {
        const selectArg = create('select', '');
        for(let obj of context.objects) {
            if(context.types[obj].match(`^${rgx}$`))
                selectArg.append(create('option', obj))
        }
        typeArguments.append(selectArg);
    }
}

function updateAdjectives(name) {
    for(let div of $('#object-list').children) {
        if(div.querySelector('.name').innerText != name)
            continue;

        const span = div.querySelector('.adjectives');
        const adjectives = [];
        const typeBase = context.types[name].split(' ')[0];
        for(let key in context.properties[name])
            adjectives.push((context.properties[name][key] ? '' : 'not ') + properties[typeBase][key]);
        setText(span, adjectives.join(', '));
    }
}
