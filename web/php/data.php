<?php

include 'php_util.php';
$DATABASE_PATH = '../data/database.db';
$PASSWORD_FILE = '../data/password.pwd';
$method = $_SERVER['REQUEST_METHOD'];

function is_subset($subset, $set)
{
    return count(array_intersect($subset, $set)) == count($subset);
}

function is_equal_set($a, $b)
{
    return count($a) == count($b) && count(array_intersect($a, $b)) == count($a);
}

function check_password($password)
{
    global $PASSWORD_FILE;
    return (file_get_contents($PASSWORD_FILE) === sha1($password));
}

// Use GET method to retreive data
if ($method == 'GET') {
    // Get user data
    if (!isset($_GET['query'])) return response_bad_request('missing query');
    $query = json_decode($_GET['query'], true);
    if (!is_array($query)) return response_bad_request('invalid query');
    if (!isset($query['table'])) return response_bad_request('missing table');
    if (!isset($query['fields'])) return response_bad_request('missing fields');

    $table = $query['table'];
    $fields = $query['fields'];
    $conditions = isset($query['conditions']) ? $query['conditions'] : array(); // note that conditions are not required

    // Validate the user data
    $allowed = [
        'documentation' => ['id', 'identifier', 'documentation'],
        'proposals' => ['id', 'user', 'proposal']
    ];

    if (!is_string($table) || !is_array($fields) || !is_array($conditions)) return response_bad_request('invalid query');
    if (!array_key_exists($table, $allowed)) return response_bad_request('invalid table');
    if (!is_subset($fields, $allowed[$table])) return response_bad_request('invalid field(s)');
    if (!is_subset(array_keys($conditions), $allowed[$table])) return response_bad_request('invalid condition(s)');
    if (count($fields) == 0) return response_bad_request('no fields provided');

    // Convert conditions to sql_conditions
    $sql_conditions = [];
    foreach ($conditions as $key => $value)
        array_push($sql_conditions, $key . '=:' . $key);

    // Build query to get data from database
    $db = new SQLite3($DATABASE_PATH);
    $statement = $db->prepare('SELECT ' . implode(',', $fields) . ' FROM ' . $table . ' ' . (count($sql_conditions) == 0 ? '' : 'WHERE ' . implode(' AND ', $sql_conditions)));
    foreach ($conditions as $key => $value)
        $statement->bindValue(':' . $key, $value, SQLITE3_TEXT);
    $result = $statement->execute();
    $results = array();
    while (($entry = $result->fetchArray(SQLITE3_ASSOC)) != false)
        array_push($results, $entry);
    $db->close();

    // Print the results as a JSON array
    header('Content-Type: application/json');
    print(json_encode($results));
    return;
}

// Use POST method to update data
if ($method == 'POST') {
    // Get user data
    $query = POST_data();
    if (!isset($query['table'])) return response_bad_request('missing table');
    if (!isset($query['values'])) return response_bad_request('missing value(s)');
    if (!isset($query['password'])) return response_bad_request('missing password');
    if (!check_password($query['password'])) return response_bad_request('invalid password');

    $table = $query['table'];
    $values = $query['values'];
    $should_update = isset($query['conditions']);
    if ($should_update)
        $conditions = $query['conditions'];

    // Validate user data
    $allowed = [
        'proposals' => ['user', 'proposal'],
        'documentation' => ['identifier', 'documentation']
    ];

    if (!is_string($table) || !is_array($values)) return response_bad_request('invalid query');
    if (!array_key_exists($table, $allowed)) response_bad_request('invalid table');
    if (!$should_update && !is_equal_set(array_keys($values), $allowed[$table])) response_bad_request('invalid field(s)'); # when inserting a new row, all fields have to be set!
    if ($should_update && !is_subset(array_keys($values), $allowed[$table])) response_bad_request('invalid field(s)'); # when updating, only a subset of the fields have to be set
    if ($should_update && !is_array($conditions)) return response_bad_request('invalid condition(s)');
    if ($should_update && !is_subset(array_keys($conditions), array_merge($allowed[$table], ['id']))) return response_bad_request('invalid condition(s)'); # when updating, the id is also an allowed field to put a condition on
    if (count($values) == 0) return response_bad_request('missing value(s)');

    // Update database
    $db = new SQLite3($DATABASE_PATH);
    if ($should_update) {
        // Update row(s) in database

        // Convert conditions to sql
        $sql_conditions = [];
        foreach ($conditions as $key => $value)
            array_push($sql_conditions, $key . '=:condition_' . $key);
        $sql_values = [];
        foreach ($values as $key => $value)
            array_push($sql_values, $key . '=:value_' . $key);

        // Build and execute statement
        $sql_query = 'UPDATE ' . $table . ' SET ' . implode(', ', $sql_values) . (count($sql_conditions) == 0 ? '' : ' WHERE ' . implode(' AND ', $sql_conditions));
        $statement = $db->prepare($sql_query);
        $statement->bindValue(':value', $value, SQLITE3_TEXT);
        foreach ($conditions as $key => $value)
            $statement->bindValue(':condition_' . $key, $value, SQLITE3_TEXT);
        foreach ($values as $key => $value)
            $statement->bindValue(':value_' . $key, $value, SQLITE3_TEXT);
        $result = $statement->execute();
        $changes = $db->changes();
    } else {
        // Insert new row into database
        $statement = $db->prepare('INSERT INTO ' . $table . ' (' . implode(',', array_keys($values)) . ') VALUES (:' . implode(',:', array_keys($values)) . ')');
        $statement->bindValue(':value', $value, SQLITE3_TEXT);
        foreach ($values as $key => $value)
            $statement->bindValue(':' . $key, $value, SQLITE3_TEXT);
        $result = $statement->execute();
        $changes = $db->changes();
    }
    $db->close();

    // Print status (whether changes were made)
    header('Content-Type: application/json');
    print($changes > 0 ? 'true' : 'false');
    return;
}

// If a different method was used, respond with a 'method not allowed'
response_method_not_allowed();
