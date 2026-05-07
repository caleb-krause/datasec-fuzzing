We fuzzed inside of the AFL++ docker. See create\_docker.sh

You can build the harness with make

To get the inital seeds, use get\_seeds.sh. From there, use afl-cmin to trim down these seeds

Then fuzz with fuzz.sh
