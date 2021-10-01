function typeset(expression) {
    return expression.replace(
        new RegExp(/(\w+(?:\.\w+)+)/, 'g'),
        function ($1) {
            const i = $1.lastIndexOf('.');
            const namespace = $1.substr(0, i);
            const identifier = $1.substr(i + 1);

            if (window.location.pathname.endsWith('docs.html'))
                return `<a href="#${$1}">${identifier}</a>`;
            else
                return `<a href="docs.html#${$1}" target="_blank">${identifier}</a>`;
        });
}

function messageBox(message) {
    switch (message.status) {
        case 'error': return create('div', message.data, { 'class': 'result error' });
        case 'fail': return create('div', message.data, { 'class': 'result fail' });
        case 'success':
            if (typeof message.data === 'string')
                return create('div', typeset(message.data), { 'class': 'result tt', 'style': 'font-size: 1.25rem' });

            if (message.data.length == 0)
                return create('div', 'no solutions found', { 'class': 'result' });

            for (let solution of message.data) {
                const result = create('div', '', { 'class': 'result' });
                for (let X in solution) {
                    result.append(create('span', '<span style="color: rgba(0, 0, 0, 0.5);">' + X + '</span>: ' + typeset(solution[X]), { 'class': 'tt', 'style': 'fonts-size: 1.25rem' }));
                    result.append(create('br'));
                }
                return result;
            }
    }
}

function createLoading() {
    return create('div', '', { 'class': 'loading' });
}

function api(query) {
    console.log(query);
    return requestPOST('php/query.php', JSON.stringify({ 'query': query }));
}
