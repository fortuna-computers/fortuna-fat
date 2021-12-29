#!/usr/bin/env python

import sys
import pprint

pp = pprint.PrettyPrinter(indent=1)

show_backups = False

#
# HTML stuff
#

THEAD = '''<thead>
  <tr>
    <th>ADDR</th><th>Cluster</th><th>Sector</th><th></th>
    <th>_0</th><th>_1</th><th>_2</th><th>_3</th><th>_4</th><th>_5</th><th>_6</th><th>_7</th><th></th>
    <th>_8</th><th>_9</th><th>_A</th><th>_B</th><th>_C</th><th>_D</th><th>_E</th><th>_F</th><th></th>
    <th></th>
  </tr>
</thead>'''


def open_html():
    print('''<!DOCTYPE HTML>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>fat2html</title>
    <style>
        table { border-collapse: collapse; }
        th, td { 
            font-family: monospace; 
            text-align: left; 
            font-size: x-large; 
            border: 1px solid lightgray;
            padding: 4px;
        }
        .spacing { width: 16px; }
        .zero { color: slategray; }
        .right { text-align: right; }
    </style>
</head>
<body>''')


def close_html():
    print('</body></html>')


def to_char(c):
    if c == 32:
        return '&#9141;'
    elif 32 < c < 127:
        return chr(c)
    else:
        return '&centerdot;'


def html_section(name, array, sector, part_start=None, bs=None):
    t = ['<h2>', name, '</h2>', '<table>', THEAD, '<tbody>']
    previous_was_empty = False
    for i in range(len(array) // 16):
        this_is_empty = all(v == 0 for v in array[(i * 16) : (i * 16 + 16)])
        if this_is_empty and previous_was_empty:
            continue
        t.append('<tr>')
        if this_is_empty:
            t.append('<td>*</td>')
            for j in range(22):
                t.append('<td></td>')
            previous_was_empty = True
        else:
            t.append('<td>%08X</td>' % (((sector + (part_start or 0)) * SECTOR_SZ) + (i * 16)))
            if part_start == None:
                t.append('<td></td><td></td>')
            else:
                t.append('<td class="right">%X</td>' % ((sector + (i * 16 // 512)) // bs.sectors_per_cluster))
                t.append('<td class="right">%X</td>' % (sector + (i * 16 // 512)))
            t.append('<td class="spacing"></td>')
            for j in range(16):
                data = array[(i * 16) + j]
                t.append('<td%s>%02X</td>' % ((' class="zero"' if data == 0 else ''), data))
                if j == 7:
                    t.append('<td class="spacing"></td>')
            t.append('<td class="spacing"></td>')
            t.append('<td>' + ''.join(map(to_char, array[(i * 16) : (i * 16 + 16)])) + '</td>')
            previous_was_empty = False
        t.append('</tr>')

    t.append('</tbody></table>')
    print(''.join(t))


#
# Data management
#


SECTOR_SZ = 0x200


def from16(data, pos):
    return data[pos] | (data[pos+1] << 8)


def from32(data, pos):
    return data[pos] | (data[pos+1] << 8) | (data[pos+2] << 16) | (data[pos+3] << 24)


def read(f, sector, n_sectors=1):
    f.seek(sector * SECTOR_SZ)
    return f.read(SECTOR_SZ * n_sectors)


def parse_mbr(mbr, partition_number):
    return from32(mbr, 0x1be + (partition_number * 4) + 8)


def parse_boot_sector(boot_sector):
    class BootSector:
        def __init__(self):
            self.root_dir_sectors = None
            self.root_dir_sec = None
            self.data_sec = None
            self.sectors_per_cluster = None
            self.fat_size = None
            self.reserved_count = None
            self.fat_type = None

        pass
    bs = BootSector()
    bs.sectors_per_cluster = boot_sector[13]
    bs.num_fats = boot_sector[16]
    bs.root_entry_count = from16(boot_sector, 17)
    bs.reserved_count = from16(boot_sector, 14)

    bs.root_dir_sectors = ((bs.root_entry_count * 32) + 511) // 512
    bs.fat_size = from16(boot_sector, 22)
    if bs.fat_size == 0:
        bs.fat_size = from32(boot_sector, 36)
    bs.tot_sec = from16(boot_sector, 19)
    if bs.tot_sec == 0:
        bs.tot_sec = from32(boot_sector, 32)

    bs.data_sec = bs.reserved_count + (bs.num_fats * bs.fat_size) + bs.root_dir_sectors
    bs.root_dir_sec = bs.reserved_count + (bs.num_fats * bs.fat_size)

    cluster_count = (bs.tot_sec - bs.data_sec) // bs.sectors_per_cluster
    if cluster_count < 65525:
        bs.fat_type = 16
    else:
        bs.fat_type = 32
    return bs


def parse_fat(fat, bs):
    clusters = []
    if bs.fat_type == 32:
        for i in range(2, bs.fat_size // 4):
            data = from32(fat, i * 4)
            if data != 0:
                clusters.append(i)
    elif bs.fat_type == 16:
        for i in range(2, bs.fat_size // 2):
            data = from16(fat, i * 2)
            if data != 0:
                clusters.append(i)
    return clusters


#
# MAIN
#

if __name__ == '__main__':

    if len(sys.argv) != 2:
        raise Exception('Usage: ' + sys.argv[0] + ' imagefile.img')
    
    open_html()

    with open(sys.argv[1], 'rb') as f:

        # MBR
        mbr = read(f, 0)
        html_section("MBR", mbr, 0)
        boot_sector_abs = parse_mbr(mbr, 0)

        # Boot sector
        boot_sector = read(f, boot_sector_abs)
        bs = parse_boot_sector(boot_sector)
        html_section("Boot sector", boot_sector, 0, boot_sector_abs, bs)
        print('<pre>' + pp.pformat(vars(bs)) + '</pre>')

        # FSInfo
        if bs.fat_type == 32:
            html_section("FSInfo", read(f, boot_sector_abs + 1), 1, boot_sector_abs, bs)

        # Backups
        if show_backups:
            html_section("Boot sector (backup)", read(f, boot_sector_abs + 6), 6, boot_sector_abs, bs)
            html_section("FSInfo (backup)", read(f, boot_sector_abs + 7), 7, boot_sector_abs, bs)

        # FAT
        fat = read(f, boot_sector_abs + bs.reserved_count, bs.fat_size)
        clusters = parse_fat(fat, bs)
        html_section("FAT", fat, bs.reserved_count, boot_sector_abs, bs)

        # Root directory
        if bs.fat_type == 16:
            html_section("Root directory", read(f, boot_sector_abs + bs.root_dir_sec, bs.root_dir_sectors), bs.root_dir_sec, boot_sector_abs, bs)

        # Data clusters
        for cluster in clusters:
            sector = bs.data_sec + (cluster * bs.sectors_per_cluster)
            html_section('Data cluster 0x%X' % cluster, read(f, boot_sector_abs + sector, bs.sectors_per_cluster), sector, boot_sector_abs, bs)

    close_html()
