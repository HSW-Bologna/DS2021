#!/usr/bin/env python
import sys
import os
import csv
import argparse


def main(indir, outdir):
    print(f"Generazioni da {indir} a {outdir}...")
    files = [x for x in [os.path.join(indir, y) for y in os.listdir(
        indir)] if os.path.isfile(x) and x.endswith('.csv')]
    translations = {}

    for csvfile in files:
        with open(csvfile, 'r') as f:
            csvreader = csv.reader(f, delimiter=',', skipinitialspace=True)
            arrayname = os.path.basename(csvfile).replace(".csv", "")
            tmp = {}
            csvreader.__next__() # Drop the first line
            for line in csvreader:
                if len(line) < 2:
                    print("Devono esserci almeno due colonne (la prima e' per l'enum)")
                    exit(1)
                tmp[line[0]] = [x.lstrip() for x in line[1:]]

            translations[arrayname] = tmp

    try:
        name = os.path.basename(indir)

        with open(os.path.join(outdir, f"AUTOGEN_FILE_{name}.c"), 'w') as c, open(os.path.join(outdir, f"AUTOGEN_FILE_{name}.h"), "w") as h:
            h.write(f"#ifndef AUTOGEN_FILE_{name.upper()}_H_INCLUDED\n")
            h.write(f"#define AUTOGEN_FILE_{name.upper()}_H_INCLUDED\n\n")
            for key, value in translations.items():
                if key == name:
                    prefix = name
                else:
                    prefix = f"{name}_{key}"

                items = len(value.keys())
                lingue = len(list(value.values())[0])

                c.write(f"const char *{prefix}[{items}][{lingue}] = {{\n")
                count = 1

                h.write(f"typedef enum {{\n")
                for enum in value.keys():
                    if len(value[enum]) != lingue:
                        print(
                            f"Numero di lingue diverso nel file {key}.csv, {count}: {lingue} vs {len(value[enum])}")
                        exit(1)

                    if count == 1:
                        h.write(
                            f"    {prefix.upper()}_{enum.upper()} = 0,\n")
                    else:
                        h.write(f"    {prefix.upper()}_{enum.upper()},\n")
                    count += 1

                    c.write("    {")
                    for string in value[enum]:
                        c.write('"' + string.replace('"', '\\"') + '", ')
                    c.write("},\n")

                c.write("};\n\n")
                h.write(f"}} {prefix}_t;\n\n")
                h.write(
                    f"extern const char *{prefix}[{items}][{lingue}];\n")

            h.write("\n#endif\n")
    except EnvironmentError as e:
        print(e)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description="Generazione automatica di array C di traduzioni")
    parser.add_argument('cartella', type=str,
                        help='Cartella dove trovare i file .csv')
    parser.add_argument('-o', '--output', type=str, nargs='?', default='.',
                        help='Cartella dove vengono salvati i sorgenti generati')
    args = parser.parse_args()

    main(args.cartella, args.output)
