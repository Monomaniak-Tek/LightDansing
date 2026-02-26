from pathlib import Path

Import("env")
root = Path(env.subst("$PROJECT_DIR"))
html_path = root / "web" / "webpage.html"
out_path = root / "include" / "webpage.generated.h"

delimiter = "__WEBPAGE__"
html = html_path.read_text(encoding="utf-8")

if f"){delimiter}\"" in html:
    raise RuntimeError("Delimiter collision in HTML content")

content = (
    "#pragma once\n"
    "// Auto-generated from web/webpage.html\n"
    "const char* webPage = R\"" + delimiter + "(\n"
    + html
    + "\n)" + delimiter + "\";\n"
)

out_path.write_text(content, encoding="utf-8")
print(f"Generated {out_path}")
