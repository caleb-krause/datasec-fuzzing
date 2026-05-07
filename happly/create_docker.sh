# first, docker pull aflplusplus/aflplusplus

docker run \
    --name happly \
    --security-opt seccomp=unconfined \
    -ti \
    --cpuset-cpus 0-11\
    -v ../:/happly aflplusplus/aflplusplus
