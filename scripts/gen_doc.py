import subprocess
import json

namespaces  = [
    'commutative_algebra',
    'commutative_algebra.ring',
    'commutative_algebra.ring_map',
    'commutative_algebra.module',
    'commutative_algebra.module_map',
    'commutative_algebra.monoid',
    'commutative_algebra.monoid_map',

    'algebraic_geometry',
    'algebraic_geometry.scheme',
    'algebraic_geometry.scheme_map',
    'algebraic_geometry.sheaf'
]

# Feeds input to process, reads one line as output
def feed(process, input):
    process.stdin.write((input + ';\n').encode())
    process.stdin.flush()
    output = process.stdout.readline().decode()
    jsonoutput = json.loads(output)
    if jsonoutput['status'] == 'error':
        print(f'[⚠️ ERROR] input {input} gives {jsonoutput["data"]}')
    return jsonoutput

# Start process
canard = subprocess.Popen(['../bin/canard', '--json', '--explicit', '--doc', '../web/math/main.cnd'], stdin = subprocess.PIPE, stdout = subprocess.PIPE, stderr = subprocess.PIPE)

# List all identifiers
identifiers = []
for space in namespaces:
    result = feed(canard, 'inspect ' + space)
    for identifier in result['data']:
        identifiers.append(identifier)

# Generate doc for each identifier
documentation = {}
definitions = {}
for identifier in identifiers:
    result = feed(canard, 'doc ' + identifier)
    if result['data']:
        documentation[identifier] = result['data'].strip()

    result = feed(canard, 'check ' + identifier)
    if result['data']:
        definitions[identifier] = result['data']


# Write to files
file = open('../web/json/documentation.json', 'w')
file.write(json.dumps(documentation))
file.close()

file = open('../web/json/definitions.json', 'w')
file.write(json.dumps(definitions))
file.close()

print('Done!')
