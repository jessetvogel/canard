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
file = open('../web/doc/documentation.json', 'w')
file.write(json.dumps(documentation))
file.close()

file = open('../web/doc/definitions.json', 'w')
file.write(json.dumps(definitions))
file.close()

print('Done!')

# # Send input and receive output
# input = ''.join([ 'inspect ' + space + ';' for space in namespaces ]) + '; exit ;'
# (stdoutdata, stderrdata) = process.communicate(input = input.encode())
# output = stdoutdata.decode('UTF-8')
# error = stderrdata.decode('UTF-8')

# # Interpret output
# identifiers = []
# for space, message in zip(namespaces, output.strip().split('\n')):
#     for x in json.loads(message)['data']:
#         identifiers.append(space + '.' + x)

# # Simply print the identifiers
# for x in identifiers:
#     print(x)
