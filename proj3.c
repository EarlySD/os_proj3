#include <stdio.h>
#include <stdlib.h>

int getBytesFromOffset(FILE *ptr, int numOfBytes, int Offset);
void getClusterLocations(FILE *ptr, int FATRegionStart, int firstFileCluster);
void printTextFromSector(FILE *ptr, int clusterStart, int bytesPerSec);

void main(int argc, char *argv[])
{
	unsigned int buffer[10];
	FILE *ptr;

	ptr = fopen(argv[1],"rb");  // r for read, b for binary

	unsigned int BPB_BytesPerSector = getBytesFromOffset(ptr, 2, 11);
	unsigned int BPB_SecPerClus = getBytesFromOffset(ptr, 1, 13);
	unsigned int BPB_ReservedSecCnt = getBytesFromOffset(ptr, 2, 14);
	unsigned int BPB_NumFATs = getBytesFromOffset(ptr, 1, 16);
	unsigned int BPB_RootEntCnt = getBytesFromOffset(ptr, 2, 17);
	unsigned int BPB_FATSz32 = getBytesFromOffset(ptr, 4, 36);
	unsigned int BPB_RootClus = getBytesFromOffset(ptr, 4, 44);

	int RootDirSectors = ((BPB_RootEntCnt * 32) + (BPB_BytesPerSector -1)) / BPB_BytesPerSector;
	int FirstDataSector = BPB_ReservedSecCnt + (BPB_NumFATs * BPB_FATSz32) + RootDirSectors;
	int FirstSectorofCluster = ((BPB_RootClus - 2) * BPB_SecPerClus)  + FirstDataSector;

	printf("BPB_BytesPerSector: %d\n", BPB_BytesPerSector);
	printf("BPB_SecPerClus: %d\n", BPB_SecPerClus);
	printf("BPB_ReservedSecCnt: %d\n", BPB_ReservedSecCnt);
	printf("BPB_NumFATs: %d\n", BPB_NumFATs);
	printf("BPB_RootEntCnt: %d\n", BPB_RootEntCnt);
	printf("BPB_FATSz32: %d\n", BPB_FATSz32);
	printf("BPB_RootClus: %d\n", BPB_RootClus);
	printf("RootDirSectors: %d\n", RootDirSectors);
	printf("FirstDataSector: %d\n", FirstDataSector);
	printf("FirstSectorofCluster: %x\n", FirstSectorofCluster);

	printf("Fat Region: 0x%x\n", BPB_ReservedSecCnt * 512);
	printf("%08x\n", getBytesFromOffset(ptr, 4, (BPB_ReservedSecCnt * 512) + 8));	
	getClusterLocations(ptr, BPB_ReservedSecCnt * 512, 3);

	printf("Data Region: 0x%x\n", FirstDataSector  * 512);
	printTextFromSector(ptr, ((FirstDataSector + 1) * 512), BPB_BytesPerSector);


	fclose(ptr);
}

//Returns a specified number of bytes at a specified offset
int getBytesFromOffset(FILE *fptr, int numOfBytes, int Offset)
{
	//Sets ptr to location at Offset bytes
	fseek(fptr, Offset, SEEK_SET);

	//Read number of specified bytes from speicifies offset and return it
	unsigned int temp[1];
	temp[0] = 0;
	fread(temp, 1, numOfBytes, fptr);
	return temp[0];
}

//Prints out all cluster locations for a given starting cluster of a file in the FAT region
void getClusterLocations(FILE *ptr, int FATRegionStart, int firstFileCluster)
{
	//Print out first file cluster
	printf("0x%08x\n", firstFileCluster);

	int clustNum = getBytesFromOffset(ptr, 4, FATRegionStart + (firstFileCluster * 4));
	
	//Continue loop of cluster pointing to next cluster until end of cluster for file is reached
	while(clustNum != 0x0FFFFFF8 && clustNum != 0x0FFFFFFF)
	{
		printf("0x%08x\n", clustNum);	
		//printTextFromSector(ptr, ((0x100400) + ((clustNum - 2) * 512)), 512);
		clustNum = getBytesFromOffset(ptr, 4, FATRegionStart + (clustNum * 4));
	}
}

//Print out characters byte by byte for a given cluster
void printTextFromSector(FILE *ptr, int clusterStart, int bytesPerSec)
{
	int i;
	for(i = 0;i < bytesPerSec;++i)
	{
		printf("%c", getBytesFromOffset(ptr, 1, clusterStart + i));	
	}
}
