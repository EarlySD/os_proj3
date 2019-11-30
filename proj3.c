#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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
int writeBytesToOffset(FILE *fptr, int byteToWrite, int numOfBytes, int Offset);
void getClusterLocations(FILE *ptr, int FATRegionStart, int firstFileCluster);
void printTextFromSector(FILE *ptr, int clusterStart, int bytesPerSec);
struct fileStruct parseFileEntry(FILE *ptr, int fileEntryAddress);
void printFileEntry(FILE *ptr, int fileEntryAddress);
//void getFilesFromDirCluster(FILE *ptr, int clusterStart, int bytesPerSec);
void getFilesFromDirCluster(FILE *ptr, int firstDirFatNumber, int bytesPerSec, int dataSectionStart, int FatSectionStart);
void getSizeOfFileName(FILE *ptr, int firstDirFatNumber, int bytesPerSec, int dataSectionStart, int FatSectionStart, char * fname);
void setSizeOfFileName(FILE *ptr, int firstDirFatNumber, int bytesPerSec, int dataSectionStart, int FatSectionStart, char * fname, int newSize);
void printDirContents(FILE *ptr, int firstCurDirFatNumber, int bytesPerSec, int dataSectionStart, int FatSectionStart, char * dirName);
struct fileStruct searchDirForFile(FILE *ptr, int firstDirFatNumber, int bytesPerSec, int dataSectionStart, int FatSectionStart, char * fname);
struct fileStruct searchDirForDir(FILE *ptr, int firstDirFatNumber, int bytesPerSec, int dataSectionStart, int FatSectionStart, char * fname);
int isDirEmpty(FILE *ptr, int firstDirFatNumber, int bytesPerSec, int dataSectionStart, int FatSectionStart);
int searchDirForEmptyFileEntry(FILE *ptr, int firstDirFatNumber, int bytesPerSec, int dataSectionStart, int FatSectionStart);
int searchFATForEmptyEntry(FILE *ptr, int dataSectionStart, int FatSectionStart);
void createEmptyFileAtOffset(FILE *ptr, int offset, char * fileName, int fatEntry, int dataSectionStart, int FatSectionStart);
void createEmptyDirAtOffset(FILE *ptr, int offset, char * fileName, int fatEntry, int fatEntryForParentDir, int dataSectionStart, int FatSectionStart);
void deleteFileEntryFromDir(FILE *ptr, int firstDirFatNumber, int bytesPerSec, int dataSectionStart, int FatSectionStart, char * fname);

void main(int argc, char *argv[])
{
	unsigned int buffer[10];
	FILE *ptr;
	char userInput[255];
	char arg1[255];
	char arg2[255];
	char arg3[255];

	char openFileNames[255][255];//[NumOfFiles][SizeOfFileNames]
	int openFilePermissions[255];//0 = closed;1 = r;2 = w;3 = rw
	int fileTableSize = 255;

	ptr = fopen(argv[1],"r+b");  // r for read, b for binary

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
			else
				printf("File not found\n");
		}
		else if(strcmp(userInput, "ls") == 0)
		{
			scanf("%s", arg1);
			
			struct fileStruct tfile = searchDirForDir(ptr, firstFATforCurDir, BPB_BytesPerSector, FirstDataSector * 512, BPB_ReservedSecCnt * 512, arg1);
			//getSizeOfFileName(ptr, firstFATforCurDir, BPB_BytesPerSector, FirstDataSector * 512, BPB_ReservedSecCnt * 512, arg1);
			if(strcmp(tfile.name, "") != 0)
			{
				if(tfile.FstClusLO == 0 && (strcmp(tfile.name, "..") == 0))
					getFilesFromDirCluster(ptr, 0x2, 512, FirstDataSector * 512, BPB_ReservedSecCnt * 512);
				else
					getFilesFromDirCluster(ptr, tfile.FstClusLO, 512, FirstDataSector * 512, BPB_ReservedSecCnt * 512);
			}
			else
				printf("Directory not found\n");
		}
		else if(strcmp(userInput, "cd") == 0)
		{
			scanf("%s", arg1);
			
			struct fileStruct tfile = searchDirForDir(ptr, firstFATforCurDir, BPB_BytesPerSector, FirstDataSector * 512, BPB_ReservedSecCnt * 512, arg1);
			//getSizeOfFileName(ptr, firstFATforCurDir, BPB_BytesPerSector, FirstDataSector * 512, BPB_ReservedSecCnt * 512, arg1);
			if(strcmp(tfile.name, "") != 0)
			{
				if(tfile.FstClusLO == 0 && (strcmp(tfile.name, "..") == 0))
					firstFATforCurDir = 0x2;
				else
					firstFATforCurDir = tfile.FstClusLO;
			}
			else
				printf("Directory not found\n");
		}
		else if(strcmp(userInput, "creat") == 0)
		{
			scanf("%s", arg1);
			
			struct fileStruct tfile = searchDirForFile(ptr, firstFATforCurDir, BPB_BytesPerSector, FirstDataSector * 512, BPB_ReservedSecCnt * 512, arg1);
			
			if(strcmp(tfile.name, "") != 0)
				printf("File already exists in this directory\n");
			else
			{
				int locOffset = searchDirForEmptyFileEntry(ptr, firstFATforCurDir, BPB_BytesPerSector, FirstDataSector * 512, BPB_ReservedSecCnt * 512);
				int fatEntry = searchFATForEmptyEntry(ptr, FirstDataSector * 512, BPB_ReservedSecCnt * 512);

				createEmptyFileAtOffset(ptr, locOffset, arg1, fatEntry, FirstDataSector * 512, BPB_ReservedSecCnt * 512);
			}
		}
		else if(strcmp(userInput, "mkdir") == 0)
		{
			scanf("%s", arg1);
			
			struct fileStruct tfile = searchDirForDir(ptr, firstFATforCurDir, BPB_BytesPerSector, FirstDataSector * 512, BPB_ReservedSecCnt * 512, arg1);
			
			if(strcmp(tfile.name, "") != 0)
				printf("Directory already exists in this directory\n");
			else
			{
				int locOffset = searchDirForEmptyFileEntry(ptr, firstFATforCurDir, BPB_BytesPerSector, FirstDataSector * 512, BPB_ReservedSecCnt * 512);
				int fatEntry = searchFATForEmptyEntry(ptr, FirstDataSector * 512, BPB_ReservedSecCnt * 512);

				createEmptyDirAtOffset(ptr, locOffset, arg1, fatEntry, firstFATforCurDir, FirstDataSector * 512, BPB_ReservedSecCnt * 512);
			}
		}
		else if(strcmp(userInput, "open") == 0)
		{
			scanf("%s", arg1);
			scanf("%s", arg2);
			
			struct fileStruct tfile = searchDirForFile(ptr, firstFATforCurDir, BPB_BytesPerSector, FirstDataSector * 512, BPB_ReservedSecCnt * 512, arg1);

			if(strcmp(tfile.name, "") != 0)
			{
				//Appends directory fat number entry to end of file in order to make it unique in the file table to other files with the same name but in different directories
				char openFileDir[8];
				snprintf(openFileDir, 8, "%x", firstFATforCurDir);
				strcat(openFileDir, arg1);
		
				int i;
				for(i = 0;i < fileTableSize;++i)
				{
					if(openFilePermissions[i] != 0 && strcmp(openFileDir, openFileNames[i]) == 0)
					{
						printf("File already open\n");
						break;
					}
				}

				if(i == fileTableSize)
				{
					for(i = 0;i < fileTableSize;++i)
					{
						//Found open slot in file table
						if(openFilePermissions[i] == 0)
						{
							//Appends directory fat number entry to end of file in order to make it unique in the file table to other files with the same name but in different directories
							char openFileDir[8];
							snprintf(openFileDir, 8, "%x", firstFATforCurDir);
							strcat(openFileDir, arg1);
							strcpy(openFileNames[i], openFileDir);

							//Set file permission based on argument
							if(strcmp(arg2, "r") == 0)
								openFilePermissions[i] = 1;
							else if(strcmp(arg2, "w") == 0)
								openFilePermissions[i] = 2;
							else if(strcmp(arg2, "rw") == 0 || strcmp(arg2, "wr") == 0)
								openFilePermissions[i] = 3;
							else
								printf("Invalid file permission\n");

							break;
						}
					}
				}
			}
			else
				printf("File not found\n");
		}
		else if(strcmp(userInput, "close") == 0)
		{
			scanf("%s", arg1);
			
			struct fileStruct tfile = searchDirForFile(ptr, firstFATforCurDir, BPB_BytesPerSector, FirstDataSector * 512, BPB_ReservedSecCnt * 512, arg1);

			if(strcmp(tfile.name, "") != 0)
			{
				//Appends directory fat number entry to end of file in order to make it unique in the file table to other files with the same name but in different directories
				char openFileDir[8];
				snprintf(openFileDir, 8, "%x", firstFATforCurDir);
				strcat(openFileDir, arg1);
		
				int i;
				for(i = 0;i < fileTableSize;++i)
				{
					if(openFilePermissions[i] != 0 && strcmp(openFileDir, openFileNames[i]) == 0)
					{
						openFilePermissions[i] = 0;
					}
				}
			}
			else
				printf("File not found\n");
		}
		else if(strcmp(userInput, "printFTable") == 0)
		{
			int i;
			for(i = 0;i < fileTableSize;++i)
			{
				printf("%d: %s %d\n", i, openFileNames[i], openFilePermissions[i]);
			}
		}
		else if(strcmp(userInput, "read") == 0)
		{
			int intarg2;
			int intarg3;

			scanf("%s", arg1);
			scanf("%d", &intarg2);
			scanf("%d", &intarg3);
			
			struct fileStruct tfile = searchDirForFile(ptr, firstFATforCurDir, BPB_BytesPerSector, FirstDataSector * 512, BPB_ReservedSecCnt * 512, arg1);

			//If file exists
			if(strcmp(tfile.name, "") != 0)
			{
				//Appends directory fat number entry to end of file in order to make it unique in the file table to other files with the same name but in different directories
				char openFileDir[263];
				snprintf(openFileDir, 8, "%x", firstFATforCurDir);
				strcat(openFileDir, arg1);
		
				int i;
				for(i = 0;i < fileTableSize;++i)
				{
					//If file is open for reading
					if((openFilePermissions[i] == 1 || openFilePermissions[i] == 3) && strcmp(openFileDir, openFileNames[i]) == 0)
					{
						if(tfile.FileSize < intarg2)
						{
							printf("Offset larger than file size\n");
							break;
						}
						else
						{
							int j;
							int curFileDataCluster = tfile.FstClusLO;

							for(j = 0;j < (intarg2 / 512);++j)
							{
								curFileDataCluster = getBytesFromOffset(ptr, 4, (BPB_ReservedSecCnt * 512) + (curFileDataCluster * 4));
							}

							int k = intarg2 % 512;

							for(j = 0;j < intarg3 && j + intarg2 < tfile.FileSize;++j)
							{
								printf("%c", getBytesFromOffset(ptr, 1, (FirstDataSector * 512) + ((curFileDataCluster - 2) * 512) + k));

								if(++k >= 512)
								{
									k = 0;
									curFileDataCluster = getBytesFromOffset(ptr, 4, (BPB_ReservedSecCnt * 512) + (curFileDataCluster * 4));
								}
							}	
						}

						break;
					}
				}

				if(i == fileTableSize)
					printf("File not open for reading\n");
			}
			else
				printf("File not found\n");
		}
		else if(strcmp(userInput, "write") == 0)
		{
			int intarg2;
			int intarg3;
			char arg4[255];

			scanf("%s", arg1);
			scanf("%d", &intarg2);
			scanf("%d", &intarg3);
			scanf("%s", arg4);
			
			struct fileStruct tfile = searchDirForFile(ptr, firstFATforCurDir, BPB_BytesPerSector, FirstDataSector * 512, BPB_ReservedSecCnt * 512, arg1);

			//If file exists
			if(strcmp(tfile.name, "") != 0)
			{
				//Appends directory fat number entry to end of file in order to make it unique in the file table to other files with the same name but in different directories
				char openFileDir[263];
				snprintf(openFileDir, 8, "%x", firstFATforCurDir);
				strcat(openFileDir, arg1);
		
				int i;
				for(i = 0;i < fileTableSize;++i)
				{
					//If file is open for reading
					if((openFilePermissions[i] == 2 || openFilePermissions[i] == 3) && strcmp(openFileDir, openFileNames[i]) == 0)
					{
						if(tfile.FileSize < intarg2 + intarg3)
						{
							printf("Offset + size larger than file size\n");

							setSizeOfFileName(ptr, firstFATforCurDir, 512, FirstDataSector * 512, BPB_ReservedSecCnt * 512, arg1, intarg2 + intarg3);

							int j;

							for(j = 0;j < (((intarg2 + intarg3) - tfile.FileSize) / 512.0) + 1;++j)
							{
								int clustNum = tfile.FstClusLO;
								int curClustNum = tfile.FstClusLO;

								while(clustNum != 0x0FFFFFF8 && clustNum != 0x0FFFFFFF)
								{
									curClustNum = clustNum;
									clustNum = getBytesFromOffset(ptr, 4, (BPB_ReservedSecCnt * 512) + (clustNum * 4));
								}

								int fatEntry = searchFATForEmptyEntry(ptr, FirstDataSector * 512, BPB_ReservedSecCnt * 512);

								writeBytesToOffset(ptr, fatEntry, 4, (BPB_ReservedSecCnt * 512) + ((curClustNum) * 4));
								writeBytesToOffset(ptr, 0x0FFFFFF8, 4, (BPB_ReservedSecCnt * 512) + ((fatEntry) * 4));

								for(i = 0;i < 512;++i)
								{
									writeBytesToOffset(ptr, 0x0, 1, (FirstDataSector * 512) + ((fatEntry - 2) * 512) + i);
								}
							}
						}


						int j;
						int curFileDataCluster = tfile.FstClusLO;

						//Sets the cluster to the starting offset point
						for(j = 0;j < (intarg2 / 512);++j)
						{
							curFileDataCluster = getBytesFromOffset(ptr, 4, (BPB_ReservedSecCnt * 512) + (curFileDataCluster * 4));
						}

						int k = intarg2 % 512;

						for(j = 0;j < intarg3;++j)
						{
							if(j < strlen(arg4))
								writeBytesToOffset(ptr, arg4[j], 1, (FirstDataSector * 512) + ((curFileDataCluster - 2) * 512) + k);
							else
								writeBytesToOffset(ptr, 0x0, 1, (FirstDataSector * 512) + ((curFileDataCluster - 2) * 512) + k);

							if(++k >= 512)
							{
								k = 0;
								curFileDataCluster = getBytesFromOffset(ptr, 4, (BPB_ReservedSecCnt * 512) + (curFileDataCluster * 4));
							}
						}	

						break;
					}
				}

				if(i == fileTableSize)
					printf("File not open for writing\n");
			}
			else
				printf("File not found\n");
		}
		else if(strcmp(userInput, "rm") == 0)
		{
			scanf("%s", arg1);
			
			struct fileStruct tfile = searchDirForFile(ptr, firstFATforCurDir, BPB_BytesPerSector, FirstDataSector * 512, BPB_ReservedSecCnt * 512, arg1);
			
			if(strcmp(tfile.name, "") != 0)
			{
				int j;
				int curClustNum = 0;

				while(tfile.FstClusLO != curClustNum)
				{
					int clustNum = tfile.FstClusLO;
					int prevClustNum = 0;

					while(clustNum != 0x0FFFFFF8 && clustNum != 0x0FFFFFFF)
					{
						prevClustNum = curClustNum;
						curClustNum = clustNum;
						clustNum = getBytesFromOffset(ptr, 4, (BPB_ReservedSecCnt * 512) + (clustNum * 4));
					}

					int fatEntry = searchFATForEmptyEntry(ptr, FirstDataSector * 512, BPB_ReservedSecCnt * 512);

					writeBytesToOffset(ptr, 0x0, 4, (BPB_ReservedSecCnt * 512) + ((curClustNum) * 4));
					writeBytesToOffset(ptr, 0x0FFFFFF8, 4, (BPB_ReservedSecCnt * 512) + ((prevClustNum) * 4));

					int i;
					for(i = 0;i < 512;++i)
					{
						writeBytesToOffset(ptr, 0x0, 1, (FirstDataSector * 512) + ((curClustNum - 2) * 512) + i);
					}
				}

				deleteFileEntryFromDir(ptr, firstFATforCurDir, BPB_BytesPerSector, FirstDataSector * 512, BPB_ReservedSecCnt * 512, arg1);
		
			}
			else
				printf("File not found\n");
		}
		else if(strcmp(userInput, "rmdir") == 0)
		{
			scanf("%s", arg1);
			
			struct fileStruct tfile = searchDirForDir(ptr, firstFATforCurDir, BPB_BytesPerSector, FirstDataSector * 512, BPB_ReservedSecCnt * 512, arg1);
			
			if(strcmp(tfile.name, "") != 0 && strcmp(tfile.name, ".") != 0 && strcmp(tfile.name, "..") != 0)
			{
				if(isDirEmpty(ptr, tfile.FstClusLO, BPB_BytesPerSector, FirstDataSector * 512, BPB_ReservedSecCnt * 512) == 1)
				{
					int j;
					int curClustNum = 0;

					while(tfile.FstClusLO != curClustNum)
					{
						int clustNum = tfile.FstClusLO;
						int prevClustNum = 0;

						while(clustNum != 0x0FFFFFF8 && clustNum != 0x0FFFFFFF)
						{
							prevClustNum = curClustNum;
							curClustNum = clustNum;
							clustNum = getBytesFromOffset(ptr, 4, (BPB_ReservedSecCnt * 512) + (clustNum * 4));
						}

						int fatEntry = searchFATForEmptyEntry(ptr, FirstDataSector * 512, BPB_ReservedSecCnt * 512);

						writeBytesToOffset(ptr, 0x0, 4, (BPB_ReservedSecCnt * 512) + ((curClustNum) * 4));
						writeBytesToOffset(ptr, 0x0FFFFFF8, 4, (BPB_ReservedSecCnt * 512) + ((prevClustNum) * 4));

						int i;
						for(i = 0;i < 512;++i)
						{
							writeBytesToOffset(ptr, 0x0, 1, (FirstDataSector * 512) + ((curClustNum - 2) * 512) + i);
						}
					}

					deleteFileEntryFromDir(ptr, firstFATforCurDir, BPB_BytesPerSector, FirstDataSector * 512, BPB_ReservedSecCnt * 512, arg1);
				}
				else
					printf("Directory not empty\n");
			}
			else
				printf("Directory not found\n");
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

int writeBytesToOffset(FILE *fptr, int byteToWrite, int numOfBytes, int Offset)
{
	//Sets ptr to location at Offset bytes
	fseek(fptr, Offset, SEEK_SET);

	//Read number of specified bytes from speicifies offset and return it
	unsigned int temp[1];
	temp[0] = byteToWrite;
	fwrite(temp, numOfBytes, 1, fptr);
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
				{
					//Checks for Long file name entry by checking if FstClusLo is 0 and file name is not ..
					if(getBytesFromOffset(ptr, 2, clusterStart + (i * 32) + 26) != 0x0 || (getBytesFromOffset(ptr, 1, clusterStart + (i * 32)) == '.' && getBytesFromOffset(ptr, 1, clusterStart + (i * 32) + 1) == '.'))
						printFileEntry(ptr, clusterStart + (i * 32));
				}
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
					if(strcmp(sname, fname) == 0 && getBytesFromOffset(ptr, 1, clusterStart + (i * 32) + 11) != 0x10)
					{
						return parseFileEntry(ptr, clusterStart + (i * 32));
					}
				}
				else
				{
					//If filename was not found
					struct fileStruct fs = {""};
					return fs;
					break;
				}
			}
		}

		currentFatNum = getBytesFromOffset(ptr, 4, FatSectionStart + (currentFatNum * 4));
	}while(currentFatNum != 0x0FFFFFF8 && currentFatNum != 0x0FFFFFFF);
}

int isDirEmpty(FILE *ptr, int firstDirFatNumber, int bytesPerSec, int dataSectionStart, int FatSectionStart)
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
					if(strcmp(sname, ".") != 0 && strcmp(sname, "..") != 0)
					{
						return 0;
					}
				}
				else
				{
					return 1;
				}
			}
		}

		currentFatNum = getBytesFromOffset(ptr, 4, FatSectionStart + (currentFatNum * 4));
	}while(currentFatNum != 0x0FFFFFF8 && currentFatNum != 0x0FFFFFFF);
}

void deleteFileEntryFromDir(FILE *ptr, int firstDirFatNumber, int bytesPerSec, int dataSectionStart, int FatSectionStart, char * fname)
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
						writeBytesToOffset(ptr, 0x0, 32, clusterStart + (i * 32));
						writeBytesToOffset(ptr, 0xE5, 1, clusterStart + (i * 32));
					}
				}
				else
				{
					//If filename was not found
					break;
				}
			}
		}

		currentFatNum = getBytesFromOffset(ptr, 4, FatSectionStart + (currentFatNum * 4));
	}while(currentFatNum != 0x0FFFFFF8 && currentFatNum != 0x0FFFFFFF);
}

struct fileStruct searchDirForDir(FILE *ptr, int firstDirFatNumber, int bytesPerSec, int dataSectionStart, int FatSectionStart, char * fname)
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
					if(strcmp(sname, fname) == 0 && getBytesFromOffset(ptr, 1, clusterStart + (i * 32) + 11) == 0x10)
					{
						return parseFileEntry(ptr, clusterStart + (i * 32));
					}
				}
				else
				{
					//If filename was not found
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

void setSizeOfFileName(FILE *ptr, int firstDirFatNumber, int bytesPerSec, int dataSectionStart, int FatSectionStart, char * fname, int newSize)
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
						writeBytesToOffset(ptr, newSize, 4, clusterStart + (i * 32) + 28);
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

int searchDirForEmptyFileEntry(FILE *ptr, int firstDirFatNumber, int bytesPerSec, int dataSectionStart, int FatSectionStart)
{
	int currentFatNum = firstDirFatNumber;	
	int dataInCurrentFat = currentFatNum;

	do
	{
		currentFatNum = dataInCurrentFat;

		//Gets the offset for the datasection cluster for the fat entry provided
		int clusterStart = dataSectionStart + ((currentFatNum - 2) * 512);

		//Loops through each file entry in a given cluster
		int i;
		for(i = 0;i < bytesPerSec / 32;++i)
		{
			//If a file entry starts with 0xE5, skip it because that file is deleted, but there are more files after it
			if(getBytesFromOffset(ptr, 1, clusterStart + (i * 32)) == 0xE5 || getBytesFromOffset(ptr, 1, clusterStart + (i * 32)) == 0x0)
			{
				return clusterStart + (i * 32);
			}
		}

		dataInCurrentFat = getBytesFromOffset(ptr, 4, FatSectionStart + (currentFatNum * 4));
	}while(dataInCurrentFat != 0x0FFFFFF8 && dataInCurrentFat != 0x0FFFFFFF);

	//If there was no extra room in directory clusters, create next cluster for directory
	int newFat = searchFATForEmptyEntry(ptr, dataSectionStart, FatSectionStart);
	
	//Update FAT table info
	writeBytesToOffset(ptr, newFat, 4, FatSectionStart + (currentFatNum * 4));
	writeBytesToOffset(ptr, 0x0FFFFFF8, 4, FatSectionStart + (newFat * 4));

	//Zero out new data section
	int i;
	for(i = 0;i < 512;++i)
	{
		writeBytesToOffset(ptr, 0x0, 1, dataSectionStart + ((newFat - 2) * 512) + i);
	}

	return dataSectionStart + ((newFat - 2) * 512);
}

int searchFATForEmptyEntry(FILE *ptr, int dataSectionStart, int FatSectionStart)
{
	int currentFatNum = 0x2;

	do
	{
		if(getBytesFromOffset(ptr, 4, FatSectionStart + (currentFatNum * 4)) == 0x0)
		{
			return currentFatNum;
		}

		++currentFatNum;
	}while(currentFatNum != dataSectionStart);
}

void createEmptyFileAtOffset(FILE *ptr, int offset, char * fileName, int fatEntry, int dataSectionStart, int FatSectionStart)
{	
	int i;

	//Write file's name to file entry
	for(i = 0;i < 11; ++i)
	{
		writeBytesToOffset(ptr, fileName[i], 1, offset + i);
		if(fileName[i] == '\0')
			break;
	}
	//Write null characters for remaining space after name
	for(i = i;i < 11; ++i)
	{
		writeBytesToOffset(ptr, 0x20, 1, offset + i);
	}

	//Write fat entry to file	
	writeBytesToOffset(ptr, fatEntry & 0xffff, 2, offset + 26);
	writeBytesToOffset(ptr, (fatEntry & 0xffff0000) >> 16, 2, offset + 20);
	writeBytesToOffset(ptr, 0x0FFFFFF8, 4, FatSectionStart + (fatEntry * 4));

	//Write attr to file entry
	writeBytesToOffset(ptr, 0x20, 1, offset + 11);

	//Zero out timestamps, as they are not used in this project
	writeBytesToOffset(ptr, 0x0, 2, offset + 14);//crtTime
	writeBytesToOffset(ptr, 0x0, 2, offset + 16);//crtDate
	writeBytesToOffset(ptr, 0x0, 2, offset + 18);//LstAccDate
	writeBytesToOffset(ptr, 0x0, 2, offset + 22);//WrtTime
	writeBytesToOffset(ptr, 0x0, 2, offset + 24);//WrtDate

	//Write file size to file entry
	writeBytesToOffset(ptr, 0x0, 4, offset + 28);

	for(i = 0;i < 512;++i)
	{
		writeBytesToOffset(ptr, 0x0, 1, dataSectionStart + ((fatEntry - 2) * 512) + i);
	}
	
}

void createEmptyDirAtOffset(FILE *ptr, int offset, char * fileName, int fatEntry, int fatEntryForParentDir, int dataSectionStart, int FatSectionStart)
{	
	int i;

	//Write file's name to file entry
	for(i = 0;i < 11; ++i)
	{
		writeBytesToOffset(ptr, fileName[i], 1, offset + i);
		if(fileName[i] == '\0')
			break;
	}
	//Write null characters for remaining space after name
	for(i = i;i < 11; ++i)
	{
		writeBytesToOffset(ptr, 0x20, 1, offset + i);
	}

	//Write fat entry to file	
	writeBytesToOffset(ptr, fatEntry & 0xffff, 2, offset + 26);
	writeBytesToOffset(ptr, (fatEntry & 0xffff0000) >> 16, 2, offset + 20);
	writeBytesToOffset(ptr, 0x0FFFFFF8, 4, FatSectionStart + (fatEntry * 4));

	//Write attr to file entry
	writeBytesToOffset(ptr, 0x10, 1, offset + 11);

	//Zero out timestamps, as they are not used in this project
	writeBytesToOffset(ptr, 0x0, 2, offset + 14);//crtTime
	writeBytesToOffset(ptr, 0x0, 2, offset + 16);//crtDate
	writeBytesToOffset(ptr, 0x0, 2, offset + 18);//LstAccDate
	writeBytesToOffset(ptr, 0x0, 2, offset + 22);//WrtTime
	writeBytesToOffset(ptr, 0x0, 2, offset + 24);//WrtDate

	//Write file size to file entry
	writeBytesToOffset(ptr, 0x0, 4, offset + 28);

	for(i = 0;i < 512;++i)
	{
		writeBytesToOffset(ptr, 0x0, 1, dataSectionStart + ((fatEntry - 2) * 512) + i);
	}



	//Create . for new dir
	offset = dataSectionStart + ((fatEntry - 2) * 512);

	i = 0;
	writeBytesToOffset(ptr, '.', 1, offset + i++);

	//Write null characters for remaining space after name
	for(i = i;i < 11; ++i)
	{
		writeBytesToOffset(ptr, 0x20, 1, offset + i);
	}

	//Write fat entry to file	
	writeBytesToOffset(ptr, fatEntry & 0xffff, 2, offset + 26);
	writeBytesToOffset(ptr, (fatEntry & 0xffff0000) >> 16, 2, offset + 20);

	//Write attr to file entry
	writeBytesToOffset(ptr, 0x10, 1, offset + 11);

	//Zero out timestamps, as they are not used in this project
	writeBytesToOffset(ptr, 0x0, 2, offset + 14);//crtTime
	writeBytesToOffset(ptr, 0x0, 2, offset + 16);//crtDate
	writeBytesToOffset(ptr, 0x0, 2, offset + 18);//LstAccDate
	writeBytesToOffset(ptr, 0x0, 2, offset + 22);//WrtTime
	writeBytesToOffset(ptr, 0x0, 2, offset + 24);//WrtDate

	//Write file size to file entry
	writeBytesToOffset(ptr, 0x0, 4, offset + 28);




	//Create .. for new dir
	offset += 32;

	i = 0;
	writeBytesToOffset(ptr, '.', 1, offset + i++);
	writeBytesToOffset(ptr, '.', 1, offset + i++);

	//Write null characters for remaining space after name
	for(i = i;i < 11; ++i)
	{
		writeBytesToOffset(ptr, 0x20, 1, offset + i);
	}

	//Write fat entry to file	
	writeBytesToOffset(ptr, fatEntryForParentDir & 0xffff, 2, offset + 26);
	writeBytesToOffset(ptr, (fatEntryForParentDir & 0xffff0000) >> 16, 2, offset + 20);

	//Write attr to file entry
	writeBytesToOffset(ptr, 0x10, 1, offset + 11);

	//Zero out timestamps, as they are not used in this project
	writeBytesToOffset(ptr, 0x0, 2, offset + 14);//crtTime
	writeBytesToOffset(ptr, 0x0, 2, offset + 16);//crtDate
	writeBytesToOffset(ptr, 0x0, 2, offset + 18);//LstAccDate
	writeBytesToOffset(ptr, 0x0, 2, offset + 22);//WrtTime
	writeBytesToOffset(ptr, 0x0, 2, offset + 24);//WrtDate

	//Write file size to file entry
	writeBytesToOffset(ptr, 0x0, 4, offset + 28);

}
