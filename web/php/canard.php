<?php

include 'php_util.php';

// Get user data
$query = POST_data()['query'];
if ($query === null)
    response_bad_request('no query given');
if (strpos($query, 'import') !== false)
    response_bad_request('import not allowed');

// Setup process
$descriptorspec = array(
    0 => array('pipe', 'r'),  // stdin is a pipe that the child will read from
    1 => array('pipe', 'w'),  // stdout is a pipe that the child will write to
    2 => array('file', '/tmp/error-output.txt', 'a') // stderr is a file to write to
);

// Create process
$cwd = null;
$env = array();
$process = proc_open(dirname(__FILE__) . '/../bin/canard --json --namespaces --threads max ' . dirname(__FILE__) . '/../math/main.cnd', $descriptorspec, $pipes, $cwd, $env);

if (!is_resource($process))
    response_server_error('failed to start canard');

// Write commands to process
fwrite($pipes[0], 'open *;');
fwrite($pipes[0], $query);
fwrite($pipes[0], ";\n/--/;exit;");
fclose($pipes[0]);

header('Content-Type: application/json');

$timeout = time() + 10; // Set a timeout of 10 seconds
while (true) {
    $status = proc_get_status($process);
    if ($status == false) { // proc_get_status failed
        proc_terminate($process);
        echo '{"status":"error","data":"proc_get_status failed"}';
        exit;
    }

    if (!$status['running']) // Process terminated
        break;

    if (time() >= $timeout) {
        proc_terminate($process);
        echo '{"status":"fail","data":"timeout"}';
        exit;
    }
}

$output = stream_get_contents($pipes[1]);

// Print output
$data = '[' . implode(',', explode(PHP_EOL, trim($output))) . ']';
echo '{"status":"success","data":' . $data .  '}';

// Close write pipe
fclose($pipes[1]);

// It is important that you close any pipes before calling
// proc_close in order to avoid a deadlock
$return_value = proc_close($process);
