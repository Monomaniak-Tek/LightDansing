from pathlib import Path

Import("env")
root = Path(env.subst("$PROJECT_DIR"))
out_path = root / "include" / "webpages.generated.h"

pages = [
    ("page_effets", root / "web" / "page_effets.html"),
    ("page_modele", root / "web" / "page_modele.html"),
    ("page_config", root / "web" / "page_config.html"),
]

assets = [
    ("asset_modele_generator_js", root / "web" / "js" / "modele_generator.js"),
]

parts = ["#pragma once\n", "// Auto-generated from web/page_*.html and web/js/*.js\n"]

all_items = pages + assets
for idx, (name, path) in enumerate(all_items):
    content = path.read_text(encoding="utf-8")
    delimiter = f"W{idx}"
    if f"){delimiter}\"" in content:
        raise RuntimeError(f"Delimiter collision in {path}")

    const_name = "webPage" + "".join(token.capitalize() for token in name.split("_"))
    parts.append(f"const char* {const_name} = R\"{delimiter}(\n")
    parts.append(content)
    parts.append(f"\n){delimiter}\";\n")

out_path.write_text("".join(parts), encoding="utf-8")
print(f"Generated {out_path}")
