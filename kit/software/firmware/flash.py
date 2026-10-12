"""Flash only explicitly. Default is a command preview, with no device access."""
import argparse,sys,subprocess,shlex,hashlib,json
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--port',required=True,help='Serial port of your XIAO ESP32-S3')
p.add_argument('--mode',choices=['first','update'],default='update')
p.add_argument('--write',action='store_true',help='Actually execute esptool; omitted means preview only')
a=p.parse_args();root=Path(__file__).resolve().parent
images=[('0x0','bootloader.bin'),('0x8000','partitions.bin'),('0xe000','boot_app0.bin'),('0x10000','firmware.bin')] if a.mode=='first' else [('0x10000','firmware.bin')]
cmd=[sys.executable,'-m','esptool','--chip','esp32s3','--port',a.port,'--baud','115200','--no-stub','--before','default_reset','--after','hard_reset','write_flash','--flash_mode','dio','--flash_freq','80m','--flash_size','8MB']
for offset,name in images:
 f=root/name
 if not f.is_file():p.error(f'Missing {name}')
 cmd.extend([offset,str(f)])
print(shlex.join(cmd))
if a.write:subprocess.run(cmd,check=True)
else:print('PREVIEW ONLY. Install esptool==4.5.1, then add --write to program your board. First mode is for a new board; update preserves the existing partition layout.')
