function request(endpoint, data) {
    return new Promise((resolve, reject) => {
        var xhr = new XMLHttpRequest();
        xhr.open('POST', endpoint, true);
        xhr.setRequestHeader('Content-Type', 'application/json');
        xhr.send(JSON.stringify(data));
        xhr.onload = function() {
            console.log(this.responseText);
            const data = JSON.parse(this.responseText);
            resolve(data);
        }
        xhr.onerror = function () {
            reject(this.responseText);
        }
    });
}

function api(query) {
    console.log(query);
    return request('/php/query.php', { 'query': query });
}
