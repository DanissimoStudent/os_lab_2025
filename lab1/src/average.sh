#!/bin/bash

AVERAGE=0

for number in "$@"; do
    AVERAGE=$((AVERAGE + number))
done

if [ $# -eq 0 ]; then
    echo "no data"
else 
    AVERAGE=$(echo "scale=4; $AVERAGE / $#" | bc -l)

    echo -e "Всего аргументов: $#\nСреднее арифметическое: $AVERAGE"
fi
