"""Offline asset preparation. Requires Pillow/NumPy; no runtime dependency.
NASA TIFF inputs belong in Saved/LunarSources; see Docs/lunar-orbit-prototype.md.
"""
from pathlib import Path
import sys, math, hashlib, json
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'Saved/python'))
import numpy as np
from PIL import Image

OUT = ROOT / 'Saved/LunarSources'
color = Image.open(OUT / 'lroc_color_poles_8k.tif').convert('RGB')
color.save(OUT / 'T_MoonColor.png')
height = np.asarray(Image.open(OUT / 'ldem_16_uint.tif'), dtype=np.float64) * 0.5 - 10000.0
h, w = height.shape
# Tangent-space slopes from LOLA height in meters. No artistic height multiplier.
dx = 1737400 * 2 * math.pi / w * np.maximum(np.sin((np.arange(h)+0.5)*math.pi/h), .02)[:,None]
dy = 1737400 * math.pi / h
sx = (np.roll(height,-1,axis=1)-np.roll(height,1,axis=1))/(2*dx)
sy = np.gradient(height, axis=0)/dy
normal = np.stack((-sx, sy, np.ones_like(sx)), axis=2)
normal /= np.linalg.norm(normal, axis=2, keepdims=True)
Image.fromarray(np.uint8(np.clip(normal*.5+.5,0,1)*255)).save(OUT / 'T_MoonNormal.png')

# Unit sphere of radius 100 cm, later scaled to the lunar reference radius.
# Geometry follows sampled LOLA heights without exaggeration; UV seam duplicated.
nx, ny = 512, 256
with (OUT / 'SM_Moon.obj').open('w') as f:
    f.write('o Moon\n')
    for j in range(ny+1):
        v = j/ny
        lat = math.pi*(.5-v)
        for i in range(nx+1):
            u=i/nx
            lon=2*math.pi*(u-.5)
            elev=height[min(int(v*h),h-1),int(u*w)%w]
            r=100*(1+elev/1737400)
            f.write(f'v {r*math.cos(lat)*math.cos(lon):.7f} {r*math.cos(lat)*math.sin(lon):.7f} {r*math.sin(lat):.7f}\n')
            f.write(f'vt {u:.7f} {1-v:.7f}\n')
            f.write(f'vn {math.cos(lat)*math.cos(lon):.7f} {math.cos(lat)*math.sin(lon):.7f} {math.sin(lat):.7f}\n')
    for j in range(ny):
        for i in range(nx):
            a=j*(nx+1)+i+1; b=a+1; c=a+nx+1; d=c+1
            triangles = ((b,c,d),) if j == 0 else ((a,c,b),) if j == ny-1 else ((a,c,b),(b,c,d))
            for tri in triangles:
                f.write('f '+' '.join(f'{k}/{k}/{k}' for k in tri)+'\n')

# Original parabolic dish, centimeters. Temporary proportions only.
with (OUT / 'SM_Dish.obj').open('w') as f:
    f.write('o Dish\n')
    for j in range(17):
        r=max(.01,j/16*60)
        for i in range(65):
            t=i/64*2*math.pi
            f.write(f'v {r*math.cos(t)} {r*math.sin(t)} {r*r/180}\n')
            f.write(f'vt {i/64} {j/16}\n')
    for j in range(16):
        for i in range(64):
            a=j*65+i+1; b=a+1; c=a+65; d=c+1
            f.write(f'f {a}/{a} {b}/{b} {c}/{c}\nf {b}/{b} {d}/{d} {c}/{c}\n')

# Original solar cell grid and foil micro-normal; not scientific material data.
y,x=np.mgrid[:1024,:1024]
grid=(x%128<4)|(y%128<4)
busbar=(x%32<1)
cells=np.zeros((1024,1024,3),dtype=np.uint8)
cells[:]=[12,25,48]; cells[grid]=[92,98,105]; cells[busbar & ~grid]=[48,63,83]
Image.fromarray(cells).save(OUT/'T_SolarCells.png')
rng=np.random.default_rng(19)
small=Image.fromarray(np.uint8(rng.random((64,64))*255)).resize((512,512),Image.Resampling.BICUBIC)
foil=np.asarray(small,dtype=float)/255
fy,fx=np.gradient(foil)
n=np.stack((-fx*5,fy*5,np.ones_like(fx)),axis=2)
n/=np.linalg.norm(n,axis=2,keepdims=True)
Image.fromarray(np.uint8((n*.5+.5)*255)).save(OUT/'T_FoilNormal.png')
manifest={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in OUT.glob('*.tif')}
(OUT/'source_hashes.json').write_text(json.dumps(manifest,indent=2))
print('Prepared NASA derivatives and original prototype meshes/textures:', OUT)
