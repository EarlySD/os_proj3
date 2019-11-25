#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct fileStruct 
{ 
	char name[12]; 
	int Attr;
	int crtTime;
	int crtDate;
	int LstAccDate;
	int FstClusHi;
	int WrtTime;
	int WrtDate;
	int FstClusLO; //(FAT location for first cluster)
	int FileSize;
};

int getBytesFromOffset(FILE *ptr, int numOfBytes, int Offset);
void getClusterLocations(FILE *ptr, int FATRegionStart, int firstFileCluster);
void printTextFromSector(FILE *ptr, int clusterStart, int bytesPerSec);
struct fileStruct parseFileEntry(FILE *ptr, int fileEntryAddress);
void printFileEntry(FILE *ptr, int fileEntryAddress);
//void getFilesFromDirCluster(FILE *ptr, int clusterStart, int bytesPerSec);
void getFilesFromDirCluster(FILE *ptr, int firstDirFatNumber, int bytesPerSec, int dataSectionStart, int FatSectionStart);
void getSizeOfFileName(FILE *ptr, int firstDirFatNumber, int bytesPerSec, int dataSectionStart, int FatSectionStart, char * fname);
void printDirContents(FILE *ptr, int firstCurDirFatNumber, int bytesPerSec, int dataSectionStart, int FatSectionStart, char * dirName);
struct fileStruct searchDirForFile(FILE *ptr, int firstDirFatNumber, int bytesPerSec, int dataSectionStart, int FatSectionStart, char * fname);

void main(int argc, char *argv[])
{
	unsigned int buffer[10];
	FILE *ptr;
	char userInput[255];
	char arg1[255];
	char arg2[255];
	char arg3[255];

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

	int firstFATforCurDir = 0x2;

	printf("Fat Region: 0x%x\n", BPB_ReservedSecCnt * 512);
	printf("%08x\n", getBytesFromOffset(ptr, 4, (BPB_ReservedSecCnt * 512) + 8));	
	getClusterLocations(ptr, BPB_ReservedSecCnt * 512, 0x1b2);

	printf("Data Region: 0x%x\n", FirstDataSector  * 512);
	printTextFromSector(ptr, ((FirstDataSector + 431) * 512), BPB_BytesPerSector);

	parseFileEntry(ptr, (FirstDataSector * 512) + 32);

	//getFilesFromDirCluster(ptr, FirstSectorofCluster * 512, 512);

	struct fileStruct test = searchDirForFile(ptr, firstFATforCurDir, BPB_BytesPerSector, FirstDataSector * 512, BPB_ReservedSecCnt * 512, "LONGFILE");
	printf("search res: %s\n", test.name);

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
		else if(strcmp(userInput, "size") == 0)
		{
			scanf("%s", arg1);
			
			struct fileStruct tfile = searchDirForFile(ptr, firstFATforCurDir, BPB_BytesPerSector, FirstDataSector * 512, BPB_ReservedSecCnt * 512, arg1);
			//getSizeOfFileName(ptr, firstFATforCurDir, BPB_BytesPerSector, FirstDataSector * 512, BPB_ReservedSecCnt * 512, arg1);
			if(strcmp(tfile.name, "") != 0)
				printf("File size: %d\n", tfile.FileSize);
		}
		else if(strcmp(userInput, "ls") == 0)
		{
			scanf("%s", arg1);
			
			struct fileStruct tfile = searchDirForFile(ptr, firstFATforCurDir, BPB_BytesPerSector, FirstDataSector * 512, BPB_ReservedSecCnt * 512, arg1);
			//getSizeOfFileName(ptr, firstFATforCurDir, BPB_BytesPerSector, FirstDataSector * 512, BPB_ReservedSecCnt * 512, arg1);
			if(strcmp(tfile.name, "") != 0)
			{
				if(tfile.FstClusLO == 0 && (strcmp(tfile.name, "..") == 0))
					getFilesFromDirCluster(ptr, 0x2, 512, FirstDataSector * 512, BPB_ReservedSecCnt * 512);
				else
					getFilesFromDirCluster(ptr, tfile.FstClusLO, 512, FirstDataSector * 512, BPB_ReservedSecCnt * 512);
					
			}
		}
		else if(strcmp(userInput, "cd") == 0)
		{
			scanf("%s", arg1);
			
			struct fileStruct tfile = searchDirForFile(ptr, firstFATforCurDir, BPB_BytesPerSector, FirstDataSector * 512, BPB_ReservedSecCnt * 512, arg1);
			//getSizeOfFileName(ptr, firstFATforCurDir, BPB_BytesPerSector, FirstDataSector * 512, BPB_ReservedSecCnt * 512, arg1);
			if(strcmp(tfile.name, "") != 0)
			{
				if(tfile.FstClusLO == 0 && (strcmp(tfile.name, "..") == 0))
					firstFATforCurDir = 0x2;
				else
					firstFATforCurDir = tfile.FstClusLO;
			}
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


struct fileStruct parseFileEntry(FILE *ptr, int fileEntryAddress)
{
	struct fileStruct fs;

	char sname[12];

	int i;
	for(i = 0;i < 11;++i)
	{
		if(getBytesFromOffset(ptr, 1, fileEntryAddress + i) == 0x20)
			break;

		sname[i] = getBytesFromOffset(ptr, 1, fileEntryAddress + i);
	}
	sname[i] = '\0';

	strcpy(fs.name, sname);
	fs.Attr = getBytesFromOffset(ptr, 1, fileEntryAddress + 11);
	fs.crtTime = getBytesFromOffset(ptr, 2, fileEntryAddress + 14);
	fs.crtDate = getBytesFromOffset(ptr, 2, fileEntryAddress + 16);
	fs.LstAccDate = getBytesFromOffset(ptr, 2, fileEntryAddress + 18);
	fs.FstClusHi = getBytesFromOffset(ptr, 2, fileEntryAddress + 20);
	fs.WrtTime = getBytesFromOffset(ptr, 2, fileEntryAddress + 22);
	fs.WrtDate = getBytesFromOffset(ptr, 2, fileEntryAddress + 24);
	fs.FstClusLO  = getBytesFromOffset(ptr, 2, fileEntryAddress + 26);
	fs.FileSize = getBytesFromOffset(ptr, 4, fileEntryAddress + 28);

	return fs;
}

void printFileEntry(FILE *ptr, int fileEntryAddress)
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

void getFilesFromDirCluster(FILE *ptr, int firstDirFatNumber, int bytesPerSec, int dataSectionStart, int FatSectionStart)
{
	int currentFatNum = firstDirFatNumber;	

	do
	{
		//Gets the offset for the datasection cluster for the fat entry provided
		int clusterStart = dataSectionStart + ((currentFatNum - 2) * 512);
	
		int i;

		for(i = 0;i < bytesPerSec / 32;++i)
		{
			//Does not print info if first 4 bytes are all 0
			//THIS SHOULD BE TEMPOARARY
			//It should make sure all 32 bytes are zero, but I think they can only be checked 4 bytes at a time
			if(getBytesFromOffset(ptr, 1, clusterStart + (i * 32)) != 0xE5)
			{
				if(getBytesFromOffset(ptr, 1, clusterStart + (i * 32)) != 0x00)
					printFileEntry(ptr, clusterStart + (i * 32));
				else
					break;
			}
		}	
		currentFatNum = getBytesFromOffset(ptr, 4, FatSectionStart + (currentFatNum * 4));
	}while(currentFatNum != 0x0FFFFFF8 && currentFatNum != 0x0FFFFFFF);

}

struct fileStruct searchDirForFile(FILE *ptr, int firstDirFatNumber, int bytesPerSec, int dataSectionStart, int FatSectionStart, char * fname)
{
	int currentFatNum = firstDirFatNumber;	

	do
	{
		//Gets the offset for the datasection cluster for the fat entry provided
		int clusterStart = dataSectionStart + ((currentFatNum - 2) * 512);

		//Loops through each file entry in a given cluster
		int i;
		for(i = 0;i < bytesPerSec / 32;++i)
		{
			//If a file entry starts with 0xE5, skip it because that file is deleted, but there are more files after it
			if(getBytesFromOffset(ptr, 1, clusterStart + (i * 32)) != 0xE5)
			{
				//If a file entry starts with 0x9, then there are no more files in that directory
				if(getBytesFromOffset(ptr, 1, clusterStart + (i * 32)) != 0x00)
				{
					char sname[12] = "";

					//Loop gets the name of the file for a file entry, char by char
					int j;
					for(j = 0;j < 11;++j)
					{
						if(getBytesFromOffset(ptr, 1, clusterStart + (i * 32) + j) == 0x20)
							break;
						sname[j] = getBytesFromOffset(ptr, 1, clusterStart + (i * 32) + j);
					}
					sname[j] = '\0';

					//If the file name matches the file name provided, print out the size of that file
					if(strcmp(sname, fname) == 0)
					{
						return parseFileEntry(ptr, clusterStart + (i * 32));
					}
				}
				else
				{
					//If filename was not found
					printf("File not found\n");
					struct fileStruct fs = {""};
					return fs;
					break;
				}
			}
		}

		currentFatNum = getBytesFromOffset(ptr, 4, FatSectionStart + (currentFatNum * 4));
	}while(currentFatNum != 0x0FFFFFF8 && currentFatNum != 0x0FFFFFFF);
}

//Searches the current directory for a provided filename, and prints the size of that file if it is found
void getSizeOfFileName(FILE *ptr, int firstDirFatNumber, int bytesPerSec, int dataSectionStart, int FatSectionStart, char * fname)
{
	int currentFatNum = firstDirFatNumber;	

	do
	{
		//Gets the offset for the datasection cluster for the fat entry provided
		int clusterStart = dataSectionStart + ((currentFatNum - 2) * 512);

		//Loops through each file entry in a given cluster
		int i;
		for(i = 0;i < bytesPerSec / 32;++i)
		{
			//If a file entry starts with 0xE5, skip it because that file is deleted, but there are more files after it
			if(getBytesFromOffset(ptr, 1, clusterStart + (i * 32)) != 0xE5)
			{
				//If a file entry starts with 0x9, then there are no more files in that directory
				if(getBytesFromOffset(ptr, 1, clusterStart + (i * 32)) != 0x00)
				{
					char sname[12] = "";

					//Loop gets the name of the file for a file entry, char by char
					int j;
					for(j = 0;j < 11;++j)
					{
						if(getBytesFromOffset(ptr, 1, clusterStart + (i * 32) + j) == 0x20)
							break;
						sname[j] = getBytesFromOffset(ptr, 1, clusterStart + (i * 32) + j);
					}
					sname[j] = '\0';

					//If the file name matches the file name provided, print out the size of that file
					if(strcmp(sname, fname) == 0)
					{
						printf("File Size: %d\n", getBytesFromOffset(ptr, 4, clusterStart + (i * 32) + 28));
						break;
					}
				}
				else
				{
					//If filename was not found
					printf("File not found\n");
					break;
				}
			}
		}

		currentFatNum = getBytesFromOffset(ptr, 4, FatSectionStart + (currentFatNum * 4));
	}while(currentFatNum != 0x0FFFFFF8 && currentFatNum != 0x0FFFFFFF);
}

void printDirContents(FILE *ptr, int firstCurDirFatNumber, int bytesPerSec, int dataSectionStart, int FatSectionStart, char * dirName)
{
	int currentFatNum = firstCurDirFatNumber;	

	do
	{
		//Gets the offset for the datasection cluster for the fat entry provided
		int clusterStart = dataSectionStart + ((currentFatNum - 2) * 512);

		//Loops through each file entry in a given cluster
		int i;
		for(i = 0;i < bytesPerSec / 32;++i)
		{
			//If a file entry starts with 0xE5, skip it because that file is deleted, but there are more files after it
			if(getBytesFromOffset(ptr, 1, clusterStart + (i * 32)) != 0xE5)
			{
				//If a file entry starts with 0x9, then there are no more files in that directory
				if(getBytesFromOffset(ptr, 1, clusterStart + (i * 32)) != 0x00)
				{
					char sname[12] = "";

					//Loop gets the name of the file for a file entry, char by char
					int j;
					for(j = 0;j < 11;++j)
					{
						if(getBytesFromOffset(ptr, 1, clusterStart + (i * 32) + j) == 0x20)
							break;
						sname[j] = getBytesFromOffset(ptr, 1, clusterStart + (i * 32) + j);
					}
					sname[j] = '\0';

					//If the file name matches the file name provided, print out the size of that file
					if(strcmp(sname, dirName) == 0)
					{
						//Print files in this dir
						int firstDirCluster = getBytesFromOffset(ptr, 2, clusterStart + (i * 32) + 26);
						break;
					}
				}
				else
				{
					//If filename was not found
					printf("Directory not found\n");
					break;
				}
			}
		}

		currentFatNum = getBytesFromOffset(ptr, 4, FatSectionStart + (currentFatNum * 4));
	}while(currentFatNum != 0x0FFFFFF8 && currentFatNum != 0x0FFFFFFF);
}
