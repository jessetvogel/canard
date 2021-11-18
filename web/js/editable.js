var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
function makeEditable(elem) {
    const [table, field, jsonCond] = elem.getAttribute('data-edit').split('|');
    const conditions = JSON.parse(jsonCond);
    const original = elem.innerHTML;
    onRightClick(elem, event => {
        if (!event.ctrlKey || !event.altKey)
            return;
        event.preventDefault();
        if (hasClass(elem, 'edit-mode'))
            return;
        addClass(elem, 'edit-mode');
        elem.setAttribute('contenteditable', 'true');
        setHTML(elem, original);
        const editButtons = create('div', '', { 'class': 'edit-buttons' });
        const saveButton = create('button', 'Save', { 'class': 'save-edit' });
        const cancelButton = create('button', 'Cancel', { 'class': 'save-edit' });
        onClick(saveButton, (event) => __awaiter(this, void 0, void 0, function* () {
            const password = prompt('Password:');
            if (password == null)
                return;
            var response = yield dbUpdate(table, { [field]: stripHTML(elem.innerHTML) }, conditions, password);
            if (response === true)
                window.location.reload();
            if (response === false) {
                response = yield dbInsert(table, Object.assign(Object.assign({}, conditions), { [field]: stripHTML(elem.innerHTML) }), password);
                if (response === true)
                    window.location.reload();
            }
            alert(`Failed to update: ${response.data}`);
            console.log(response);
        }));
        onClick(cancelButton, event => {
            setHTML(elem, original);
            removeClass(elem, 'edit-mode');
            elem.removeAttribute('contenteditable');
            editButtons.remove();
            MathJax.typeset([elem]);
        });
        editButtons.append(cancelButton);
        editButtons.append(saveButton);
        elem.after(editButtons);
    });
}
function stripHTML(html) {
    const doc = new DOMParser().parseFromString(html, 'text/html');
    return doc.body.textContent || '';
}
//# sourceMappingURL=editable.js.map