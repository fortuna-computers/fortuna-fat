#ifndef FORTUNA_FAT_SCENARIO_H
#define FORTUNA_FAT_SCENARIO_H

#define MAX_SCENARIOS 64

const char* scenario_name;

void scenario_raw_sectors();

void (*scenario_list[MAX_SCENARIOS])();

#endif //FORTUNA_FAT_SCENARIO_H
