#ifndef FORTUNA_FAT_SCENARIO_H
#define FORTUNA_FAT_SCENARIO_H

#define MAX_SCENARIOS 64

const char* scenario_name;

void scenario_raw_sectors();
void scenario_fat16();
void scenario_fat32();
void scenario_fat32_align512();
void scenario_fat32_spc128();
void scenario_fat32_2_partitions();

void (*scenario_list[MAX_SCENARIOS])();

#endif //FORTUNA_FAT_SCENARIO_H
