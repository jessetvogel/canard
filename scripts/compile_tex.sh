#!/bin/bash

echo "Getting a list of the identifiers ..."
python3 list_identifiers.py > ../web/doc/identifiers

echo "Converting tex to html ..."
cd python && python3 convert.py
