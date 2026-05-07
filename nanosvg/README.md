We fuzzed inside of the AFL++ docker. See create\_docker.sh

Build the harness with build.sh

You can get the intial svg seeds from https://github.com/salmonx/seeds, then trim them using afl-cmin

The svg dictionary comes from here: https://github.com/AFLplusplus/AFLplusplus/blob/stable/dictionaries/svg.dict

Fuzz with fuzz.sh