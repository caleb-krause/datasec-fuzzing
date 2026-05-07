docker run \
    --name nanosvg \
    --security-opt seccomp=unconfined \
    -ti \
    --cpuset-cpus 0-15\
    -v ../:/nanosvg aflplusplus/aflplusplus
