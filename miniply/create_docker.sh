docker run \
    --name miniply \
    --security-opt seccomp=unconfined \
    -ti \
    --cpuset-cpus 0-11\
    -v ../:/miniply aflplusplus/aflplusplus
