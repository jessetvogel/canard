#!/usr/bin/env python3

import subprocess
import json

namespaces  = [
    'commutative_algebra',
    'commutative_algebra.ring',
    'commutative_algebra.module',

    'algebraic_geometry',
    'algebraic_geometry.scheme',
    'algebraic_geometry.morphism',
    'algebraic_geometry.sheaf'
]

# Start process
process = subprocess.Popen(['java', '-jar', '../bin/canard-1.0.jar', '--json', '../math/main.cnd'], stdin = subprocess.PIPE, stdout = subprocess.PIPE, stderr = subprocess.PIPE)

# Send input and receive output
input = ''.join([ 'inspect ' + space + ';' for space in namespaces ]) + '; exit ;'
(stdoutdata, stderrdata) = process.communicate(input = input.encode())
output = stdoutdata.decode('UTF-8')
error = stderrdata.decode('UTF-8')

# Interpret output
identifiers = []
for space, message in zip(namespaces, output.strip().split('\n')):
    for x in json.loads(message)['data']:
        identifiers.append(space + '.' + x)

# Simply print the identifiers
for x in identifiers:
    print(x)
