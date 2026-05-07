We fuzzed inside of the AFL++ docker. See create_docker.sh
You can build the harness with build.sh
To get the inital seeds, use get_seeds.sh. From there, use afl-cmin to trim down these seeds
Then fuzz with fuzz.sh