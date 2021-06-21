function init() {
    const inputSearch = $('#input-search');
    const inputProve = $('#input-prove');

    let lines = 1;
    function updateLineNumbers() {
        const n = (inputSearch.value.match(/\n/g) || '').length + 1;
        if(n != lines) {
            // Update line numbers
            const lineNumbers = $('#context .line-numbers');
            while(lines < n)
                lineNumbers.append(create('span', ++lines));
            while(lines > n) {
                lineNumbers.removeChild(lineNumbers.lastChild);
                lines --;
            }

            // Update height of input
            const style = getComputedStyle(inputSearch);
            inputSearch.style.height = 'calc(' + style.paddingTop + ' + ' + style.paddingBottom + ' + ' + n + '*' + style.lineHeight + ')';
        }
    }
    updateLineNumbers();
    onInput(inputSearch, updateLineNumbers);

    onInput(inputProve, function (event) {
        // No newlines are allowed here!
        const n = (inputProve.value.match(/\n/g) || '').length + 1;
        if(n > 1)
            inputProve.value = inputProve.value.replaceAll('\n', '');
    });


    onClick($('#button-search'), function () {
        const query = 'search ' + getContextQuery();

        api(query).then(response => {
            switch(response.status) {
                case 'error':
                    $('#output').innerText = 'Error: ' + response.message;
                    break;
                case 'fail':
                    $('#output').innerText = 'Fail: ' + response.data;
                    break;
                case 'success':
                    const solutions = response.data[0].data;
                    if(solutions.length == 0) {
                        setHTML($('#output'), 'No solutions found..');
                        break;
                    }

                    let output = '';
                    for(let solution of solutions) {
                        for(let X in solution) {
                            output += '<span style="font-size: 1.25rem;" class="tt"><span style="color: rgba(0, 0, 0, 0.5);">' + X + '</span>: '+ typeset(solution[X]) + '</span><br/>';
                        }
                    }
                    setHTML($('#output'), output);
                    break;
            }
        }).catch(message => {
            console.error(message);
        });

        clear($('#output'));
        $('#output').append(create('div', '', {'class': 'loading'}));
    });

    onClick($('#button-prove'), function () {
        const query = 'search (__P ' + getContextQuery() + ' : ' + inputProve.value.trim() + ')';

        api(query).then(response => {
            switch(response.status) {
                case 'error':
                    $('#output').innerText = 'Error: ' + response.message;
                    break;
                case 'fail':
                    $('#output').innerText = 'Fail: ' + response.data;
                    break;
                case 'success':
                    const solutions = response.data[0].data;    
                    if(solutions.length == 0) {
                        setHTML($('#output'), 'No proof found..');
                        break;
                    }

                    let output = '';
                    for(let solution of solutions) {
                        for(let X in solution)
                            output += '<span class="tt">' + typeset(solution[X]) + '</span><br/>';
                    }
                    
                    setHTML($('#output'), output);
                    break;
            }
        }).catch(message => {
            console.error(message);
        });

        clear($('#output'));
        $('#output').append(create('div', '', {'class': 'loading'}));
    });

    onKeyDown(inputSearch, function (event) {
        // Shift + Enter means search!
        if(event.key == 'Enter' && event.shiftKey) {
            event.preventDefault();
            $('#button-search').click();
        }
    });

    onKeyDown(inputProve, function (event) {
        // Shift + Enter means prove!
        if(event.key == 'Enter' && event.shiftKey) {
            event.preventDefault();
            $('#button-prove').click();
        }
    });
}

function getContextQuery() {
    let query = '';
    for(let line of $('#context textarea').value.split('\n')) {
        // Skip empty lines
        line = line.trim();
        if(line.length == 0)
            continue;
            
        query += '(' + line+ ')';
    }
    return query;
}

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




// function expressionToHTML(expr) {
//     // console.log(`expressionToHTML(${expr})`);

//     // Case expr is string
//     if(typeof(expr) == 'string')
//         return `<a class="tt" href="doc.html#${expr}">${expr}</a>`;

//     // Case there are no dependencies
//     if(typeof(expr[0]) == 'string') {
//         const base = expressionToHTML(expr[0]);
//         const arguments = [];
//         for(let i = 1;i < expr.length; ++i)
//             arguments.push(expressionToHTML(expr[i]));

//         return base + '(' + arguments.join(' ') + ')';
//     }

//     // Case there *are* dependencies
//     const dependencies = expr[0].map(d => {
//         const explicit = (d[0] == 1);
//         let name;
//         if(typeof(d[1]) == 'string')
//             name = d[1];
//         else if(typeof(d[1][1]) == 'string')
//             name = d[1][1];
//         else console.assert(false, 'Oops!');

        
//         // = expressionToHTML(d[1]);


//         const type = expressionToHTML(d[2]);
//         return (explicit ? '(' : '{') + name + ' : ' + type + (explicit ? ')' : '}');
//     }).join(' ');
//     return 'λ ' + dependencies + ' := ' + expressionToHTML(expr[1]);
// }

// // Just try to rebuild .toFullString() in JS