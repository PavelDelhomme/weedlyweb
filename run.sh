#!/bin/bash

# Vérifiez que le programme est compilé
if [ ! -f "build/WeedlyWeb" ]; then
    echo "Le binaire n'existe pas. Compilation en cours..."
    mkdir -p build && cd build
    cmake .. && make
    cd ..
fi

# Lancer le programme
./build/WeedlyWeb
