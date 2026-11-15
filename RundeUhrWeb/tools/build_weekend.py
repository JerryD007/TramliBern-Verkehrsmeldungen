"""Convert approved four-pose strip to the existing 16-color firmware canvas."""
from pathlib import Path
from PIL import Image
import numpy as np
r=Path(__file__).resolve().parent.parent
src=Image.open(r/'assets/weekend_approved.png').convert('RGB')
colors=src.resize((540,180)).quantize(colors=13).getpalette()[:39]
palette=[0x0000,0xffff,0xfc10]+[((colors[i]>>3)<<11)|((colors[i+1]>>2)<<5)|(colors[i+2]>>3) for i in range(0,39,3)]
rgb=np.array([[(v>>11)*255//31,((v>>5)&63)*255//63,(v&31)*255//31] for v in palette],dtype=np.int32)
frames=[];preview=[]
for i in range(4):
 cell=src.crop((i*src.width//4,130,(i+1)*src.width//4,640)).resize((160,160),Image.Resampling.LANCZOS)
 a=np.array(cell).astype(np.int32);idx=((a[:,:,None,:]-rgb[None,None,:,:])**2).sum(3).argmin(2).astype('uint8')
 frames.append(idx);canvas=Image.new('RGB',(240,240));canvas.paste(Image.fromarray(rgb[idx].astype('uint8')),(40,55));preview.append(canvas)
header='#pragma once\nstatic const uint16_t weekendPalette[]={'+','.join(map(str,palette))+'};\n// Approved poses converted to existing palette; no runtime allocation.\nstatic const unsigned char weekendFrames[4][12800]={\n'
for a in frames:
 flat=a.flatten();packed=(flat[::2]<<4)|flat[1::2];header+='{'+','.join(map(str,packed))+'},\n'
header+='};\ninline void drawWeekendToast(Adafruit_GFX& g,uint32_t ms){const unsigned char order[]={0,1,2,2,3,0};int frame=order[(ms/550)%6];for(int y=0;y<160;y++)for(int x=0;x<160;x++){int n=y*160+x;unsigned char b=weekendFrames[frame][n/2];g.drawPixel(x+40,y+55,(n%2?b:b>>4)&15);}}\n'
(r/'RundeUhr/WeekendFrames.h').write_text(header)
preview[0].save(r/'preview/weekend_transition.gif',save_all=True,append_images=[preview[k] for k in [1,2,2,3,0]],duration=550,loop=0)
