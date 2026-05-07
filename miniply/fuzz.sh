export AFL_CRASHING_SEEDS_AS_NEW_CRASH=1

INPUT=seeds
OUTPUT=findings

afl-fuzz \
    -i $INPUT \
    -o $OUTPUT \
    -a text \
    -M master \
    -- ./harness @@ &

for i in $(seq 1 11); do
    afl-fuzz \
        -i $INPUT \
        -o $OUTPUT \
        -a text \
        -S $i \
        -- ./harness @@ > /dev/null 2>&1 &
done

wait