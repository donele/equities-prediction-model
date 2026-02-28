#!/bin/bash

paste <(head -n 1 stat.txt | awk 'BEGIN{RS=" "};{print $1}' | sed '/^$/d') <(tail -n 1 stat.txt | awk 'BEGIN{RS=" "};($1!="mu"){print $1}' | sed '/^$/d') | sort -nrk 2
