#!/usr/bin/env python3
"""docs.kernel.org 전체 페이지를 .md 로 미러링 (재개 가능).

사용: python3 km_crawl.py [LIMIT]
  LIMIT 주면 앞에서 그만큼만 처리(테스트용). 없으면 전체.
출력: notes/kernel/docs-kernel-org/<uri>.md  (URL 경로 구조 = 챕터 구조 보존)
이미 존재하는 .md 는 건너뜀(resume). 실패는 km_failed.txt 에 기록.
"""
import os, sys, time, urllib.request, urllib.error
from bs4 import BeautifulSoup
from markdownify import markdownify as md

BASE = "https://docs.kernel.org/"
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "notes", "kernel", "docs-kernel-org"))
PAGES = os.path.join(HERE, "km_pages.txt")
FAILED = os.path.join(HERE, "km_failed.txt")
UA = "Mozilla/5.0 (kernel-docs-mirror; personal study)"
DELAY = 0.25  # 서버 예의상 지연(초)

def fetch(url):
    req = urllib.request.Request(url, headers={"User-Agent": UA})
    with urllib.request.urlopen(req, timeout=30) as r:
        return r.read().decode("utf-8", "replace")

def extract(html):
    soup = BeautifulSoup(html, "html.parser")
    # Sphinx 본문 컨테이너(kernel 테마: div.body[role=main])
    node = (soup.find("div", attrs={"role": "main"})
            or soup.find("div", attrs={"itemprop": "articleBody"})
            or soup.find("main")
            or soup.find("div", class_="body")
            or soup.find("article"))
    if node is None:
        node = soup.body or soup
    # 잡요소 제거: 언어선택기/사이드바/네비/푸터/편집링크/소스링크
    for t in node.find_all("div", class_="language-selection"):
        t.decompose()
    for sel in [("a", {"class": "headerlink"}), ("div", {"class": "sphinxsidebar"}),
                ("div", {"role": "navigation"}), ("footer", {}), ("nav", {})]:
        for t in node.find_all(sel[0], attrs=sel[1]):
            t.decompose()
    # 제목: 본문(node) 내부의 첫 h1 (사이트 배너 h1 아님)
    h1 = node.find("h1")
    title = h1.get_text(strip=True).rstrip("¶").strip() if h1 else ""
    return title, md(str(node), heading_style="ATX")

def main():
    limit = int(sys.argv[1]) if len(sys.argv) > 1 else None
    uris = [u.strip() for u in open(PAGES) if u.strip()]
    if limit:
        uris = uris[:limit]
    total = len(uris)
    done = skipped = failed = 0
    for n, uri in enumerate(uris, 1):
        out = os.path.join(ROOT, uri[:-5] + ".md")  # .html -> .md
        if os.path.exists(out):
            skipped += 1
            continue
        os.makedirs(os.path.dirname(out), exist_ok=True)
        url = BASE + uri
        try:
            title, body = extract(fetch(url))
            hdr = (f"# {title}\n\n" if title else "")
            hdr += (f"> 출처(원문): {url}\n"
                    f"> 자동 미러링: docs.kernel.org · 정본은 출처 URL (영문 원문 자동 변환본)\n\n---\n\n")
            with open(out, "w") as f:
                f.write(hdr + body.strip() + "\n")
            done += 1
        except Exception as e:
            failed += 1
            with open(FAILED, "a") as f:
                f.write(f"{uri}\t{type(e).__name__}: {e}\n")
        if n % 50 == 0 or n == total:
            print(f"[{n}/{total}] done={done} skip={skipped} fail={failed}", flush=True)
        time.sleep(DELAY)
    print(f"COMPLETE total={total} done={done} skip={skipped} fail={failed}", flush=True)

if __name__ == "__main__":
    main()
