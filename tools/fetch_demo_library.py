#!/usr/bin/env python3
"""Build the demo library from real releases.

    python3 tools/fetch_demo_library.py library   # rewrite assets/demo/library.json
    python3 tools/fetch_demo_library.py covers    # download + resize cover art

`library` takes tracklists and durations from MusicBrainz (CC0 data) and
writes assets/demo/library.json. Display titles are lowercase. The script
proposes them, and DISPLAY below holds the hand-written ones that a plain
lowercase would get wrong. Review the JSON after a rerun: rule 3 of the
design system says lowercase titles are written by hand, never computed
by Keel at run time.

`covers` downloads each release group's front cover from the Cover Art
Archive (or, when it has none, from the iTunes Search API) and writes one PNG per size the UI draws. Cover art is copyrighted
by its owners, so assets/demo/covers/ is in .gitignore and never committed.
Needs Pillow (python3-pillow in the toolbox) for resizing.
"""
import json
import os
import sys
import time
import urllib.parse
import urllib.request

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LIB = os.path.join(ROOT, "assets", "demo", "library.json")
COVERS = os.path.join(ROOT, "assets", "demo", "covers")
UA = "keel-demo/0.1 (https://github.com/abbybigaouette/keel)"

# Pixel sizes the UI uses. Keep in sync with KEEL_ART_* in ui/theme.h.
SIZES = [480, 408, 198, 72]

# (MusicBrainz release-group id, album id used in the JSON)
RELEASE_GROUPS = [
    ("6ab2d1f5-1730-499d-a814-6fe6eb9ec1f0", "i-love-my-computer"),
    ("859667bc-767d-4924-9f6c-a54fb42a2c24", "girl-edm"),
    ("4c6d5a9d-70f9-4e45-9fd1-095d225579ea", "blumiere"),
    ("a807b4de-9773-4f69-921f-d220eddcf11b", "draining-love-story"),
    ("8c704c7e-51c5-4446-8291-62312629dc8d", "we-had-good-times-together"),
    ("505297b7-3fdc-4cde-9b61-96844152413a", "skitzofrenia-simulation"),
    ("dca45187-3d85-4998-a4fe-55a78dd636c9", "sewerslvt-presents-cynthoni-1"),
    ("2312e9f3-bd81-4900-9b5b-0168a47d21ad", "draining-love-story-cynthoni"),
    ("9bbc8220-9f72-4595-a0f7-55dfdbab569a", "1000-gecs"),
    ("d73daa13-b571-41dd-8dfd-740fde3fda7d", "10000-gecs"),
]

# Hand-written display titles where str.lower() is wrong or ugly.
# Keys are the title as released.
DISPLAY = {
    "girl EDM": "girl EDM",
}

PLAYLISTS = [
    ("late night", ["draining-love-story:1", "i-love-my-computer:3", "10000-gecs:2",
                    "we-had-good-times-together:2", "girl-edm:1", "draining-love-story-cynthoni:2"]),
    ("loud", ["1000-gecs:2", "10000-gecs:1", "i-love-my-computer:1",
              "skitzofrenia-simulation:1", "1000-gecs:4"]),
    ("recently added", ["draining-love-story-cynthoni:1", "draining-love-story-cynthoni:3", "i-love-my-computer:2",
                        "sewerslvt-presents-cynthoni-1:1", "girl-edm:2"]),
]


def get(url):
    req = urllib.request.Request(url, headers={"User-Agent": UA})
    for attempt in range(3):
        try:
            with urllib.request.urlopen(req, timeout=30) as r:
                return r.read()
        except urllib.error.HTTPError as e:
            if e.code < 500:
                raise
            time.sleep(1 + attempt)
    raise RuntimeError("rate limited: " + url)


def mb(path):
    time.sleep(1.1)  # MusicBrainz asks for at most one request per second
    return json.loads(get("https://musicbrainz.org/ws/2/" + path + "&fmt=json"))


def display(title):
    return DISPLAY.get(title, title.lower())


def pick_release(rgid):
    """Earliest official release named like its release group (not an
    instrumental or deluxe variant), most tracks first on a tie."""
    rg = mb(f"release-group/{rgid}?inc=releases")
    rels = mb(f"release?release-group={rgid}&status=official&inc=recordings+artist-credits+media&limit=100")["releases"]
    rels = [r for r in rels if r.get("media") and r["title"] == rg["title"]]
    rels.sort(key=lambda r: (r.get("date") or "9999", -sum(len(m.get("tracks", [])) for m in r["media"])))
    return rels[0]


def cmd_library():
    artists, albums = {}, []
    for rgid, album_id in RELEASE_GROUPS:
        rel = pick_release(rgid)
        credit = rel["artist-credit"][0]["artist"]
        artist_id = credit["name"].lower().replace(" ", "-")
        artists.setdefault(artist_id, {"id": artist_id, "name": display(credit["name"]),
                                       "source": credit["name"], "mbid": credit["id"]})
        tracks = []
        for medium in rel["media"]:
            for t in medium.get("tracks", []):
                tracks.append({"title": display(t["title"]), "source": t["title"],
                               "duration": round((t.get("length") or 0) / 1000)})
        albums.append({"id": album_id, "title": display(rel["title"]), "source": rel["title"],
                       "artist": artist_id, "year": int((rel.get("date") or "0")[:4] or 0),
                       "mbid_release_group": rgid, "tracks": tracks})
        print(f"{album_id}: {len(tracks)} tracks", file=sys.stderr)
    lib = {"version": 1,
           "note": "Tracklists from MusicBrainz (CC0). Cover art is not in git: run tools/fetch_demo_library.py covers.",
           "artists": list(artists.values()), "albums": albums,
           "playlists": [{"title": t, "tracks": tr} for t, tr in PLAYLISTS]}
    with open(LIB, "w", encoding="utf-8") as f:
        f.write(json.dumps(lib, indent=2, ensure_ascii=False) + "\n")


def fetch_cover(album, artist):
    try:
        return get(f"https://coverartarchive.org/release-group/{album['mbid_release_group']}/front-500")
    except (urllib.error.HTTPError, RuntimeError):
        pass
    # The release-group redirect sometimes fails while a release's own art works.
    for rel in mb(f"release?release-group={album['mbid_release_group']}&limit=25")["releases"]:
        try:
            return get(f"https://coverartarchive.org/release/{rel['id']}/front-500")
        except (urllib.error.HTTPError, RuntimeError):
            continue
    term = urllib.parse.quote(f"{artist} {album['source']}")
    found = json.loads(get(f"https://itunes.apple.com/search?term={term}&entity=album&limit=10"))["results"]
    for r in found:
        if artist.lower() in r.get("artistName", "").lower() and \
           r.get("collectionName", "").lower().startswith(album["source"].lower()[:12]):
            return get(r["artworkUrl100"].replace("100x100bb", "600x600bb"))
    raise RuntimeError(f"no cover art found for {album['id']}")


def cmd_covers():
    from io import BytesIO
    from PIL import Image

    with open(LIB, encoding="utf-8") as f:
        lib = json.load(f)
    albums = lib["albums"]
    artists = {a["id"]: a["source"] for a in lib["artists"]}
    os.makedirs(COVERS, exist_ok=True)
    for album in albums:
        if all(os.path.exists(os.path.join(COVERS, f"{album['id']}-{s}.png")) for s in SIZES):
            continue
        data = fetch_cover(album, artists[album["artist"]])
        img = Image.open(BytesIO(data)).convert("RGB")
        side = min(img.size)  # square crop from the center
        left, top = (img.width - side) // 2, (img.height - side) // 2
        img = img.crop((left, top, left + side, top + side))
        for s in SIZES:
            img.resize((s, s), Image.LANCZOS).save(os.path.join(COVERS, f"{album['id']}-{s}.png"), optimize=True)
        print(album["id"], file=sys.stderr)


if __name__ == "__main__":
    {"library": cmd_library, "covers": cmd_covers}[sys.argv[1] if len(sys.argv) > 1 else "library"]()
