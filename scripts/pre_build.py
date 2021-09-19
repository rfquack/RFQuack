Import("env")

env.Execute("j2 docker/project/src/main.cpp.j2 build.env > src/main.cpp")