#!/bin/bash
# Idempotent. Run from the source root. Puts beryld's domain in platform private
# policy and exempts it from the coredomain proc/sysfs neverallows.
set -e
SEP=system/sepolicy
MX=vendor/beryl-extras/sepolicy/plat_private
[ -d "$MX" ] || MX=vendor/malachite-extras/sepolicy/plat_private
{ [ -d "$SEP/private" ] && [ -d "$MX" ]; } || { echo "run from the source root"; exit 1; }
if [ -f "$MX/beryld.te" ]; then
  cp "$MX/beryld.te" "$SEP/private/beryld.te"
elif [ -f "$MX/malachited.te" ]; then
  cp "$MX/malachited.te" "$SEP/private/beryld.te"
fi
python3 - "$SEP" <<'PY'
import re, sys, os
cd = os.path.join(sys.argv[1], 'private/coredomain.te')
t = open(cd).read()
if '-beryld\n' not in t:
    pat = re.compile(r'(neverallow \{[^}]*?)(\n[ \t]*\} (?:proc|sysfs):file no_rw_file_perms;)')
    t, n = pat.subn(lambda m: m.group(1) + '\n    -beryld' + m.group(2), t)
    if n != 2:
        sys.exit('expected 2 neverallow blocks in coredomain.te, found %d; not patched' % n)
    open(cd, 'w').write(t)
print('sepolicy ready for beryld')
PY
