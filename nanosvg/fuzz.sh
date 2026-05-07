# Check for crashes in the seeds
export AFL_CRASHING_SEEDS_AS_NEW_CRASH=1

# Require at least 6 input bytes
export AFL_INPUT_LEN_MIN=6

afl-fuzz \
    -i - \
    -o findings/findings4 \
    -x svg.dict \
    -- ./harness 