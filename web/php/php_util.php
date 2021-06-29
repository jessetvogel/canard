<?php

function POST_data() {
    # Method must be POST
    $method = $_SERVER['REQUEST_METHOD'];
    if($method != 'POST')
        response_method_not_allowed();

    # Try reading data (maximum of 64KB)
    $body = file_get_contents('php://input');
    // response_payload_too_large()
    
    $data = json_decode($body, true);
    if($data == null)
        response_bad_request('invalid JSON');
    
    # Return JSON data
    return $data;
}

# ---- Root directory might differ per server ----
// def root_dir():
//     if os.getcwd().endswith('cgi-bin'):
//         return '../'
//     else:
//         return ''

# ---- Some response methods ----

function response_bad_request($message = '') {
    header('Status: 400 Bad Request');
    header('Content-Type: application/json');
    print('{"status":"error","data":"Bad request' . ($message == '' ? '' : ' (' . $message . ')') . '"}');
    exit(0);
}

function response_forbidden() {
    header('Status: 403 Forbidden');
    header('Content-Type: application/json');
    print('{"status":"error","data":"Forbidden"}');
    exit(0);
}

function response_method_not_allowed() {
    header('Status: 405 Method Not Allowed');
    header('Content-Type: application/json');
    print('{{"status":"error","data":"Method not allowed"}');
    exit(0);
}

function response_payload_too_large() {
    header('Status: 413 Payload Too Large');
    header('Content-Type: application/json');
    print('{{"status":"error","data":"Payload too large"}');
    exit(0);
}

function response_server_error($message = '') {
    header('Status: 500 Internal Server Error');
    header('Content-Type: application/json');
    print('{{"status":"error","data":"500 Internal Server Error' . ($message == '' ? '' : ' (' . $message . ')') . '"}');
    exit(0);
}

?>