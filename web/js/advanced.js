function initAdvanced() {
    const input = $('input-search');

    let lines = 1;
    function updateLineNumbers() {
        const n = (input.value.match(/\n/g) || '').length + 1;
        if (n != lines) {
            // Update line numbers
            const lineNumbers = document.querySelector('#context .line-numbers');
            while (lines < n)
                lineNumbers.append(create('span', ++lines));
            while (lines > n) {
                lineNumbers.removeChild(lineNumbers.lastChild);
                lines--;
            }

            // Update height of input
            const style = getComputedStyle(input);
            input.style.height = 'calc(' + style.paddingTop + ' + ' + style.paddingBottom + ' + ' + n + '*' + style.lineHeight + ')';
        }
    }
    updateLineNumbers();
    onInput(input, updateLineNumbers);

    onClick($('button-search'), function () {
        const output = $('output');
        const query = input.value;
        api(query).then(response => {
            clear(output);
            switch (response.status) {
                case 'error':
                    setHTML(output, '<div class="result error">' + response.data + '</div>');
                    break;
                case 'fail':
                    setHTML(output, '<div class="result fail">' + response.data + '</div>');
                    break;
                case 'success':
                    for (let message of response.data)
                        output.append(messageBox(message));
            }
        }).catch(message => {
            console.error(message);
        });

        clear(output);
        output.append(createLoading());
    });

    onKeyDown(input, function (event) {
        // Shift + Enter means search!
        if (event.key == 'Enter' && event.shiftKey) {
            event.preventDefault();
            $('button-search').click();
        }
    });
}

window.onload = initAdvanced;