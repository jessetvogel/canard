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

# Feeds input to process, reads one line as output
def feed(process, input):
    process.stdin.write((input + ';\n').encode())
    process.stdin.flush()
    return json.loads(process.stdout.readline().decode())

# Start process
canard = subprocess.Popen(['../bin/canard', '--json', '--explicit', '--doc', '../web/math/main.cnd'], stdin = subprocess.PIPE, stdout = subprocess.PIPE, stderr = subprocess.PIPE)

# List all identifiers
identifiers = []
for space in namespaces:
    result = feed(canard, 'inspect ' + space)
    for identifier in result['data']:
        identifiers.append(space + '.' + identifier)

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
