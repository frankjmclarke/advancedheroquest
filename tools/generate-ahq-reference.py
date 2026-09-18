"""Generate/check the checked-in C reference table; Python is not needed to build or run HQ-Map."""
import argparse
from ahq_monsters import ROOT, c_header


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='Fail if the generated header is stale')
    args = parser.parse_args()
    dest = ROOT / 'ahq-reference-data.h'
    text = c_header()
    if args.check:
        if not dest.exists() or dest.read_text(encoding='ascii') != text:
            parser.exit(1, 'AHQ reference data is stale. Run python tools/generate-ahq-reference.py\n')
        print('AHQ reference data is up to date.')
    else:
        dest.write_text(text, encoding='ascii', newline='\n')
        print(dest)


if __name__ == '__main__':
    main()
