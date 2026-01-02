
import re
from pathlib import Path

root = Path("docs_src/wiki")
md_files = list(root.rglob("*.md"))

# wiki sayfaları arasında en sık görülen alias düzeltmeleri
ALIASES = {
    "faq": "Frequently-Asked-Questions-(FAQ).md",
    "FAQ": "Frequently-Asked-Questions-(FAQ).md",
    "troubleshooting.md": "Troubleshooting.md",
    "troubleshooting": "Troubleshooting.md",
    "Troubleshooting.md": "Troubleshooting.md",
}

# mevcut dosyaları isim->gerçek dosya eşleştir (case-insensitive)
existing = {p.name.lower(): p.name for p in md_files}

def fix_one_link(url: str) -> str:
    url = url.strip()

    # dış link / anchor / mail
    if re.match(r"^(https?:|mailto:|#)", url, re.I):
        return url

    base, anchor = (url.split("#", 1) + [""])[:2]
    anchor = ("#" + anchor) if anchor else ""

    # ./ kırp
    if base.startswith("./"):
        base = base[2:]

    if not base:
        return url

    # alias fix
    if base in ALIASES:
        return ALIASES[base] + anchor

    # uzantı varsa: case düzeltmesi yap
    if re.search(r"\.[a-z0-9]{1,6}$", base, re.I):
        key = base.lower()
        if key in existing:
            return existing[key] + anchor
        return base + anchor

    # uzantısızsa .md ekle ve var olan dosyaya map et
    candidate = (base + ".md").lower()
    if candidate in existing:
        return existing[candidate] + anchor

    # yoksa dokunma
    return url

link_re = re.compile(r"\]\(([^)]+)\)")

changed = 0
for p in md_files:
    src = p.read_text(encoding="utf-8")
    def repl(m):
        return "](" + fix_one_link(m.group(1)) + ")"
    out = link_re.sub(repl, src)

    # küçük ama kritik typo düzeltmesi: Frequently-Asked-Questions-(FAQ.md) -> ...FAQ).md
    out = out.replace("Frequently-Asked-Questions-(FAQ.md)", "Frequently-Asked-Questions-(FAQ).md")

    if out != src:
        p.write_text(out, encoding="utf-8")
        changed += 1

print("Done. Changed files:", changed)
