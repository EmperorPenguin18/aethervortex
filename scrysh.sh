#!/bin/sh

for I in {1..500}
do
	RESULTS="$(curl -sL -X "GET" "https://api.scryfall.com/cards/search?q=$(echo "$1" | sed 's/:/%3A/g')&format=csv&page=$I" | sed '1d' | cut -f -9 -d ',')"

	echo "$RESULTS" | grep validation_error >/dev/null && break
	echo "$RESULTS" | grep not_found >/dev/null && break

	echo "$RESULTS" | sed 's/""/"/g;s/,",/,{/g;s/","/",{/g' | cut -f 7- -d ',' | cut -f -1 -d '{' | rev | cut -c 2- | rev | sed 's/^"//g;s/"$//g'
done
