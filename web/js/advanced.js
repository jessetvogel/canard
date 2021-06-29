function init() {
    const input = $('#input-search');
    
    let lines = 1;
    function updateLineNumbers() {
        const n = (input.value.match(/\n/g) || '').length + 1;
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
            const style = getComputedStyle(input);
            input.style.height = 'calc(' + style.paddingTop + ' + ' + style.paddingBottom + ' + ' + n + '*' + style.lineHeight + ')';
        }
    }
    updateLineNumbers();
    onInput(input, updateLineNumbers);

    onClick($('#button-search'), function () {
        // Query is input.value
        const query = input.value;

        api(query).then(response => {
            switch(response.status) {
                case 'error':
                    setHTML($('#output'), '<div class="result error">' + response.data + '</div>');
                    break;
                case 'fail':
                    setHTML($('#output'), '<div class="result fail">' + response.data + '</div>');
                    break;
                case 'success':
                    let output = '';
                    for(let message of response.data) {
                        switch(message.status) {
                            case 'error':
                                output += '<div class="result error">' + message.data + '</div>';
                                break;

                            case 'fail':
                                output += '<div class="result fail">' + message.data + '</div>';
                                break;

                            case 'success':
                                if(message.data.length == 0) {
                                    output += '<div class="result">No solutions found..</div>';
                                    break;
                                }

                                for(let solution of message.data) {
                                    output += '<div class="result">';
                                    for(let X in solution)
                                        output += '<span style="font-size: 1.25rem;" class="tt"><span style="color: rgba(0, 0, 0, 0.5);">' + X + '</span>: '+ typeset(solution[X]) + '</span><br/>';
                                    output += '</div>';
                                }
                                break;
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

    
    onKeyDown(input, function (event) {
        // Shift + Enter means search!
        if(event.key == 'Enter' && event.shiftKey) {
            event.preventDefault();
            $('#button-search').click();
        }
    });
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
