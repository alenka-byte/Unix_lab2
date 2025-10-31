#!/bin/bash 

directory="/flock"
lock_file="$directory/.lock"

mkdir -p "$directory"
touch "$lock_file"

CONTAINER_ID="container-$(hostname)"
COUNTER=1

echo "Container $CONTAINER_ID started"

while true; do
          
          exec 200>"$lock_file"
          flock -x 200
          filename=""
          for i in {001..999}; do
              if [ ! -e "$directory/file$i" ]; then
                   filename="file$i"
                   break
              fi
          done
          if [ -n "$filename" ]; then 
                echo "Container: $CONTAINER_ID, File number: $COUNTER" > "$directory/$filename"
                echo "$(date): Created $filename"
          fi
          exec 200>&-
     if [ -n "$filename" ] && [ -f "$directory/$filename" ]; then
        sleep 1
        rm "$directory/$filename"
        echo "$(date): Deleted $filename"
        COUNTER=$((COUNTER+1))
     fi
     sleep 1
done
