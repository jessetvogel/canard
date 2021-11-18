function makeEditable(elem: HTMLElement): void {
    // data-edit contains table|field|conditions
    const [table, field, jsonCond] = elem.getAttribute('data-edit').split('|');
    const conditions = JSON.parse(jsonCond);
    const original = elem.innerHTML;

    // Control + alt + right-click activates edit mode!
    onRightClick(elem, event => {
        if (!event.ctrlKey || !event.altKey) return; // check for control + alt
        event.preventDefault();
        if (hasClass(elem, 'edit-mode')) return; // if already in edit-mode, nothing to do

        addClass(elem, 'edit-mode');
        elem.setAttribute('contenteditable', 'true');
        setHTML(elem, original);

        const editButtons = create('div', '', { 'class': 'edit-buttons' });
        const saveButton = create('button', 'Save', { 'class': 'save-edit' });
        const cancelButton = create('button', 'Cancel', { 'class': 'save-edit' });

        onClick(saveButton, async event => {
            const password = prompt('Password:');
            if (password == null) return;
            var response = await dbUpdate(table, { [field]: stripHTML(elem.innerHTML) }, conditions, password) as any;
            if (response === true) window.location.reload();
            if (response === false) {
                // Could not update, try inserting instead!
                response = await dbInsert(table, Object.assign(Object.assign({}, conditions), { [field]: stripHTML(elem.innerHTML) }), password);
                if (response === true) window.location.reload();
            }

            alert(`Failed to update: ${response.data}`);
            console.log(response);
        });

        onClick(cancelButton, event => {
            setHTML(elem, original); // restore to original text
            removeClass(elem, 'edit-mode'); // get rid of edit-mode
            elem.removeAttribute('contenteditable'); // not editable anymore
            editButtons.remove(); // remove the edit buttons
            MathJax.typeset([elem]); // typeset content again
        });

        editButtons.append(cancelButton);
        editButtons.append(saveButton);
        elem.after(editButtons);
    });
}

function stripHTML(html: string) {
    const doc = new DOMParser().parseFromString(html, 'text/html');
    return doc.body.textContent || '';
}
