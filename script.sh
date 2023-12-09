#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Utilizare: $0 <caracter>"
    exit 1
fi

caracter="$1"

contor=0

while IFS= read -r linie; do
    if echo "$linie" | grep -qE "^[A-Z][a-zA-Z0-9 ,.!]*[.!?]$"; then
        # Verifica daca nu contine virgula (,) inainte de "si"
        if ! echo "$linie" | grep -qE ", si"; then
            contor=$((contor + 1))
        fi
    fi
done

# Afiseaza rezultatul final
echo "Numarul de propozitii corecte care contin '$caracter': $contor"
