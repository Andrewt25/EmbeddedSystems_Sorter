#ifndef _SORTER_H_
#define _SORTER_H_

void Sorter_startSorting(void);
void Sorter_stopSorting(void);

void Sorter_setArraySize(int newSize);
int Sorter_getArrayLength(void);

int* Sorter_getArrayData(int *length);

long long Sorter_getNumberArraysSorted(void);

#endif