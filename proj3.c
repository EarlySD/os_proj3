#include <stdio.h>
#include <stdlib.h>

int getBytesFromOffset(FILE *ptr, int numOfBytes, int Offset);
void getClusterLocations(FILE *ptr, int FATRegionStart, int firstFileCluster);
void printTextFromSector(FILE *ptr, int clusterStart, int bytesPerSec);
void parseFileEntry(FILE *ptr, int fileEntryAddress);
void getFilesFromDirCluster(FILE *ptr, int clusterStart, int bytesPerSec);

void main(int argc, char *argv[])
{
	unsigned int buffer[10];
	FILE *ptr;
	char userInput[255];

	ptr = fopen(argv[1],"rb");  // r for read, b for binary

	unsigned int BPB_BytesPerSector = getBytesFromOffset(ptr, 2, 11);
	unsigned int BPB_SecPerClus = getBytesFromOffset(ptr, 1, 13);
	unsigned int BPB_ReservedSecCnt = getBytesFromOffset(ptr, 2, 14);
	unsigned int BPB_NumFATs = getBytesFromOffset(ptr, 1, 16);
	unsigned int BPB_RootEntCnt = getBytesFromOffset(ptr, 2, 17);
	unsigned int BPB_TotSec = getBytesFromOffset(ptr, 4, 32);
	unsigned int BPB_FATSz32 = getBytesFromOffset(ptr, 4, 36);
	unsigned int BPB_RootClus = getBytesFromOffset(ptr, 4, 44);

	int RootDirSectors = ((BPB_RootEntCnt * 32) + (BPB_BytesPerSector -1)) / BPB_BytesPerSector;
	int FirstDataSector = BPB_ReservedSecCnt + (BPB_NumFATs * BPB_FATSz32) + RootDirSectors;
	int FirstSectorofCluster = ((BPB_RootClus - 2) * BPB_SecPerClus)  + FirstDataSector;

	printf("Fat Region: 0x%x\n", BPB_ReservedSecCnt * 512);
	printf("%08x\n", getBytesFromOffset(ptr, 4, (BPB_ReservedSecCnt * 512) + 8));	
	getClusterLocations(ptr, BPB_ReservedSecCnt * 512, 0x1b2);

	printf("Data Region: 0x%x\n", FirstDataSector  * 512);
	printTextFromSector(ptr, ((FirstDataSector + 431) * 512), BPB_BytesPerSector);

	parseFileEntry(ptr, (FirstDataSector * 512) + 32);

	getFilesFromDirCluster(ptr, FirstSectorofCluster * 512, 512);


	while(strcmp(userInput, "exit") != 0)
	{
		scanf("%s", userInput);

		if(strcmp(userInput, "info") == 0)
		{
			printf("BPB_BytesPerSector: %d\n", BPB_BytesPerSector);
			printf("BPB_SecPerClus: %d\n", BPB_SecPerClus);
			printf("BPB_ReservedSecCnt: %d\n", BPB_ReservedSecCnt);
			printf("BPB_NumFATs: %d\n", BPB_NumFATs);
			printf("BPB_RootEntCnt: %d\n", BPB_RootEntCnt);
			printf("BPB_FATSz32: %d\n", BPB_FATSz32);
			printf("BPB_RootClus: %d\n", BPB_RootClus);
			printf("BPB_TotSec: %d\n", BPB_TotSec);
			printf("RootDirSectors: %d\n", RootDirSectors);
			printf("FirstDataSector: %d\n", FirstDataSector);
			printf("FirstSectorofCluster: %x\n", FirstSectorofCluster);
		}
	}

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
	printTextFromSector(ptr, ((0x100400) + ((firstFileCluster - 2) * 512)), 512);

	int clustNum = getBytesFromOffset(ptr, 4, FATRegionStart + (firstFileCluster * 4));
	
	//Continue loop of cluster pointing to next cluster until end of cluster for file is reached
	while(clustNum != 0x0FFFFFF8 && clustNum != 0x0FFFFFFF)
	{
		printf("0x%08x\n", clustNum);	
		printTextFromSector(ptr, ((0x100400) + ((clustNum - 2) * 512)), 512);
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

void parseFileEntry(FILE *ptr, int fileEntryAddress)
{
	char sname[12];

	int i;
	for(i = 0;i < 11;++i)
	{
		sname[i] = getBytesFromOffset(ptr, 1, fileEntryAddress + i);
	}
	sname[11] = '\0';

	printf("ShortName: %s\n", sname);
	printf("DIR_Attr: 0x%x\n", getBytesFromOffset(ptr, 1, fileEntryAddress + 11));
	int crtTime = getBytesFromOffset(ptr, 2, fileEntryAddress + 14);
	printf("DIR_CrtTime: %d\n", getBytesFromOffset(ptr, 2, fileEntryAddress + 14));
	printf("DIR_CrtTime: %d:%d:%d\n", ((crtTime&0xf800) >> 11), (crtTime&0x7e0) >> 5, (crtTime&0x1f) * 2);
	int crtDate = getBytesFromOffset(ptr, 2, fileEntryAddress + 16);
	printf("DIR_CrtDate: %d\n", getBytesFromOffset(ptr, 2, fileEntryAddress + 16));
	printf("DIR_CrtDate: %d/%d/%d\n", crtDate&0x1f, (crtDate&0x1e0) >> 5, ((crtDate&0xfe00) >> 9) + 1980);
	printf("DIR_LstAccDate: %d\n", getBytesFromOffset(ptr, 2, fileEntryAddress + 18));
	printf("DIR_FstClusHi: %d\n", getBytesFromOffset(ptr, 2, fileEntryAddress + 20));
	printf("DIR_WrtTime: %d\n", getBytesFromOffset(ptr, 2, fileEntryAddress + 22));
	printf("DIR_WrtDate: %d\n", getBytesFromOffset(ptr, 2, fileEntryAddress + 24));
	printf("DIR_FstClusLO (FAT location for first cluster): 0x%x\n", getBytesFromOffset(ptr, 2, fileEntryAddress + 26));
	printf("DIR_FileSize: %d\n", getBytesFromOffset(ptr, 4, fileEntryAddress + 28));
}

void getFilesFromDirCluster(FILE *ptr, int clusterStart, int bytesPerSec)
{
	int i;

	for(i = 0;i < bytesPerSec / 32;++i)
	{
		//Does not print info if first 4 bytes are all 0
		//THIS SHOULD BE TEMPOARARY
		//It should make sure all 32 bytes are zero, but I think they can only be checked 4 bytes at a time
		if(getBytesFromOffset(ptr, 4, clusterStart + (i * 32)) != 0)
			parseFileEntry(ptr, clusterStart + (i * 32));
	}
}
