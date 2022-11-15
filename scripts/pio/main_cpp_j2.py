"""
Build src/main.cpp from template docker/project/main.cpp.j2 using variables
from the build.env file.
"""

import os

Import("env")

project_dir = env.subst("$PROJECT_DIR")
src_dir = os.path.join(project_dir, "src")
build_env = os.path.join(project_dir, "build.env")
main_cpp_j2 = os.path.join(project_dir, "docker", "project", "src", "main.cpp.j2")
main_cpp = os.path.join(src_dir, "main.cpp")

try:
    if os.path.exists(main_cpp):
        os.unlink(main_cpp)
except Exception as e:
    print(f"Could not delete {main_cpp}: {e}")

cmd = f"j2 {main_cpp_j2} {build_env} > {main_cpp}"

env.Execute(cmd)