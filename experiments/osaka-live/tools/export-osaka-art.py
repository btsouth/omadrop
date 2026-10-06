#!/usr/bin/env python3
"""One-time migration from bfc79ff. art.svg is the editable source afterward.
Requires git and a C++17 compiler; never overwrites art.svg implicitly.
"""
import argparse, json, re, subprocess, tempfile
from pathlib import Path
ROOT = Path(__file__).resolve().parents[3]
BASE = "bfc79ff"
def source(name):
    return subprocess.check_output(["git", "show", BASE + ":experiments/osaka-live/src/" + name], cwd=ROOT, text=True)
def clean(text):
    return re.sub(r'^(?:#include|#pragma once).*$', '', text, flags=re.M)
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    town = source("kit/town.cpp")
    # Partition at the existing drawing boundaries, without changing arithmetic.
    for key, call in [("near-house", "roof(cv, x0, x1"), ("right-house-2", "roof(cv, x0, x0 + 250"), ("right-house-3", "roof(cv, x0, x0 + 470")]:
        at = town.index(call); end = town.index(';', at) + 1
        town = town[:at] + 'cv.begin("' + key + '-roof"); ' + town[at:end] + ' cv.begin("' + key + '-eaves");' + town[end:]
    town = town.replace('"near-house"', '"near-house-shell"').replace('"right-house-2"', '"right-house-2-shell"').replace('"right-house-3"', '"right-house-3-shell"')
    primitives = source("kit/primitives.cpp")
    primitives = primitives[primitives.index('void OsakaLatticeV1'):]
    scene = json.loads(subprocess.check_output(["git", "show", BASE + ":worlds/osaka-jade/scene.json"], cwd=ROOT, text=True))
    panes = scene["profiles"]["osaka-pane-v1"]["near"]
    upper = scene["profiles"]["osaka-pane-v1"]["upper"]
    layout = ','.join('{' + ','.join(str(q[k]) for k in ["x","y","w","h","cols","rows","on","band"]) + '}' for q in panes)
    cpp = source("art.h") + source("kit/layout.h") + clean(source("kit/palette.h"))
    cpp += (Path(__file__).with_name("export-osaka-art-recorder.h")).read_text()
    cpp += clean(source("kit/primitives.h")) + clean(source("kit/town.h"))
    cpp += '\nnamespace Journey::Kit {\n' + primitives
    cpp += clean(town)
    cpp += '\nnamespace Journey {\n' + source("caption-assets/sign-outlines.inc") + '\n}\n'
    cpp += r"""
int main() {
    using namespace Journey; using namespace Journey::Kit;
    std::cout << "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 1920 1080\">\n"
              << "<!-- Migrated from bfc79ff. Edit this artwork, not the exporter. -->\n"
              << "<!-- Sign outlines: Noto Sans CJK JP Bold, SIL OFL 1.1; see noto-sans-cjk-OFL.txt. Font SHA-256: faa5f3656a78b2e2d450d27fe8382c778bc2b6bb5ea29c986664a6a435056ceb. One em, baseline y down. -->\n";
    Canvas cv; Ctx c; OsakaState s;
    OsakaNearHouseV1::draw(c,s,cv,-80,540,Col(.009f,.028f,.023f),Col(.013f,.050f,.039f),Col(.040f,.170f,.127f));
    OsakaRightHouse2V1::draw(c,s,cv,1262,Col(.014f,.046f,.037f),Col(.018f,.070f,.054f),Col(.050f,.215f,.160f));
    OsakaRightHouse3V1::draw(c,s,cv,1512,Col(.014f,.046f,.037f),Col(.018f,.070f,.054f),Col(.050f,.215f,.160f));
    const NearPane panes[4] = {LAYOUT};
    cv.begin("near-house-lattice");
    for (const auto& q:panes) lattice(cv,q.x,q.y,q.w,q.h,q.cols,q.rows,INK,1.6);
    cv.begin("izakaya-lattice");
UPPER
    cv.begin("izakaya-counter"); cv.line(1512+58,904,1512+252,904,5,INK);
    cv.begin("laundry-line"); cv.line(1262-6,788,1262+150,788,1.2,INK);
    const Col roomCol=mix(WARM_T,INK,.62);
    cv.begin("near-house-lamp-hanger"); cv.fillRect(214,648,20,4,roomCol); cv.line(270,640,270,694,1.5,roomCol);
    OsakaDeckV1::draw(c,s,cv,0,roomCol,Col(.009f,.028f,.023f));
    OsakaRailingV1::draw(c,s,cv,2330,0);
    OsakaCartFrameV1::draw(c,s,cv,770);
    OsakaNearMaskV1::draw(c,s,cv,0,panes);
    OsakaShamisenMaskV1::draw(c,s,cv,1512);
    for (int i=0;i<int(sizeof(signGlyphs)/sizeof(signGlyphs[0]));++i) {
        cv.begin("sign-glyph-"+std::to_string(i)); cv.color(Col(1,1,1)); cv.path=signGlyphs[i]; cv.fill();
    }
    cv.end(); std::cout << "</svg>\n";
}
""".replace("LAYOUT", layout).replace("UPPER", '\n'.join('    lattice(cv,1512+'+str(q['wx'])+',640,'+str(q['ww'])+',96,'+str(3 if q['ww']>70 else 2)+',3,INK,1.3);' for q in upper))
    with tempfile.TemporaryDirectory(prefix="osaka-export-") as tmp:
        code=Path(tmp)/"export.cpp"; binary=Path(tmp)/"export"; code.write_text(cpp)
        subprocess.run(["c++","-std=c++17","-O0",str(code),"-o",str(binary)], check=True)
        args.output.write_bytes(subprocess.check_output([str(binary)]))
if __name__ == "__main__": main()
