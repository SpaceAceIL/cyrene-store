#!/usr/bin/env python3
import argparse
from pathlib import Path

TEMPLATE = '''<?xml version="1.0" encoding="utf-8" standalone="yes"?>
<psproject fmt="gp4" version="1000">
  <volume>
    <volume_type>pkg_ps4_app</volume_type>
    <volume_id>PS4VOLUME</volume_id>
    <volume_ts>2024-01-01 00:00:00</volume_ts>
    <package content_id="{content_id}" passcode="{passcode}" storage_type="bd50" app_type="full"><sc>dummy</sc></package>
    <chunk_info chunk_count="1" scenario_count="1">
      <chunks><chunk id="0" layer="0"/></chunks>
      <scenarios default_id="0"><scenario id="0" type="pkg_ps4_app"/></scenarios>
    </chunk_info>
  </volume>
  <files img_no="0">
    <file targ_path="eboot.bin" orig_path="eboot.bin"/>
    <file targ_path="sce_sys/param.sfo" orig_path="sce_sys/param.sfo"/>
    <file targ_path="sce_sys/icon0.png" orig_path="sce_sys/icon0.png"/>
  </files>
  <rootdir><dir targ_name="sce_sys"><file targ_path="param.sfo" orig_path="sce_sys/param.sfo"/><file targ_path="icon0.png" orig_path="sce_sys/icon0.png"/></dir></rootdir>
</psproject>
'''

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', required=True)
    ap.add_argument('--content-id', required=True)
    ap.add_argument('--passcode', default='0' * 32)
    a = ap.parse_args()
    Path(a.out).write_text(TEMPLATE.format(
        content_id=a.content_id, passcode=a.passcode))
    print(f'Wrote {a.out}')

if __name__ == '__main__':
    main()
