#!/bin/bash

# $1 es el prefijo (primer argumento). Si no hay, estará vacío.
PREFIX=$1

# Explicación de la tubería:
# 1. ls -1p: Lista 1 archivo por línea, y le pone un "/" al final a los directorios.
# 2. grep -v /: Filtra y elimina todo lo que tenga "/", dejando solo ficheros.
# 3. grep "^$PREFIX": Se queda solo con los que empiecen por el prefijo.
# 4. wc -l: Cuenta cuántas líneas (ficheros) han quedado.
COUNT=$(ls -1pA 2>/dev/null | grep -v / | grep "^$PREFIX" | wc -l)

echo "Número de ficheros encontrados: $COUNT"

# El retardo de monitorización que pide el enunciado
sleep 2

# Valores de retorno exigidos
if [ "$COUNT" -gt 0 ]; then
    exit 0
else
    exit 1
fi
