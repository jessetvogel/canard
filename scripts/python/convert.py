# To add a new cell, type '# %%'
# To add a new markdown cell, type '# %% [markdown]'
# %%
import os
from tex2html import Parser


# %%
# Specify paths
tex_path = '../../tex'
doc_path = '../../web/doc'

# Create parser
parser = Parser(doc_path)

# Set allowed entries
with open(doc_path + '/identifiers') as f:
    allowed = sorted([ line.strip() for line in f ])
    parser.set_allowed_identifiers(allowed)

# Search for .tex files, and parse
for f in os.listdir(tex_path):
    file_path = os.path.join(tex_path, f)
    file_extension = os.path.splitext(file_path)[1]

    if file_extension != '.tex': # only read .tex files
        continue
    if '--' in file_path: # ignore files containing '--'
        continue
    if not os.path.isfile(file_path): # skip directories / non-files
        continue
    
    try:
        parser.parse(file_path)
    except Exception as e:
        print('An error occured: ' + str(e))
        exit(0)


# %%
# Check for missing docs
docs = sorted(parser.get_identifiers())
missing = 0
for x in allowed: # TODO: be more efficient next time, use that lists are sorted
    if x not in docs:
        print('❓ Missing docs for [{}]'.format(x))
        missing += 1
if missing == 0:
    print('😁 No missing docs')


# %%
# Print stats
print('📝 Compiled {} docs'.format(len(docs)))


# %%



