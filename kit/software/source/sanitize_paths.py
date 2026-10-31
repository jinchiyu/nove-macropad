"""Map local paths out of compiler diagnostics without changing runtime logic."""
Import("env")
from pathlib import Path
roots = [env.subst("$PROJECT_DIR"), env.subst("$PROJECT_LIBDEPS_DIR"), env.subst("$PROJECT_CORE_DIR")]
for root in roots:
    if root and "$" not in root:
        value = str(Path(root).resolve())
        env.Append(CCFLAGS=["-ffile-prefix-map=" + value + "=/nove", "-fmacro-prefix-map=" + value + "=/nove"])
