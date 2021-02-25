#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <3ds.h>

#define SECOND(x) (x*1000ULL*1000ULL*1000ULL)
PrintConsole topScreen, bottomScreen;
AM_TWLPartitionInfo info;
#define DUMP_ALL true
#define SYSTEM_APP_COUNT 6
static u64 systemApps[SYSTEM_APP_COUNT] = { 
					 0x00048005484E4441,
					 0x0004800542383841,
					 0x0004800F484E4841,
					 0x0004800F484E4C41,
					 0x00048005484E4443,
					 0x00048005484E444B
};
bool isBlacklisted(u64 tid) {
	for (int i =0;i<SYSTEM_APP_COUNT;i++) {
		if (tid==systemApps[i]) {
			return true;
		}
	}
	return false;
}

Result uninstall(u64 tid){
	Result res;
	res = AM_DeleteTitle (MEDIATYPE_NAND, tid);
	if (R_SUCCEEDED(res)) {
		res = AM_DeleteTicket(tid);
	}
	return res;
}

void wait() 
{
	printf("\nPress B to continue\n");	
	while (1) 
	{
		gspWaitForVBlank(); 
		hidScanInput(); 
		if (hidKeysDown() & KEY_B) 
		{ 
			break; 
		}
	}
}
void waitOrCancel() 
{
	printf("\nPress B to continue\n");	
	printf("Press Start to exit\n");	
	while (1) 
	{
		gspWaitForVBlank(); 
		hidScanInput(); 
		if (hidKeysDown() & KEY_B) 
		{ 
			break; 
		}
		if (hidKeysDown() & KEY_START) {
			amExit();
			gfxExit();
			exit(0);
		}
	}
}

int main(int argc, char* argv[])
{
	gfxInitDefault();
	consoleInit(GFX_TOP, &topScreen);
	consoleInit(GFX_BOTTOM, &bottomScreen);
	consoleSelect(&bottomScreen);
	u32 BUF_SIZE = 0x20000;
	Result res;
	
	u8 *buf = (u8*)malloc(BUF_SIZE);
	printf("Initializing AM services\n");
	res = amInit();
	if (R_FAILED(res)) {
		printf("Unable to initialize AM service\n");
		svcSleepThread(SECOND(7));
		return 1;
	}
	//printf("amInit: %08X\n",(int)res);
	res = AM_GetTWLPartitionInfo(&info);
	if (R_FAILED(res)) {
		printf("Unable to get DSiWare Parition information.\n");
		svcSleepThread(SECOND(7));
		return 1;
	}
	printf("This application will remove all DSiWare and DSi Mode applications");
	waitOrCancel();
	printf("Retrieving number of titles\n");
	u32 title_count=0;
	u64 dsiTitle[50]={0};
	res = AM_GetTitleCount(MEDIATYPE_NAND, &title_count);
	if (R_FAILED(res)) {
		printf("Failed to get title count.\n");
	}else{
		u64 *titleID; 
		titleID=calloc(title_count,sizeof(u64));
		u32 titles_read=0;
		res = AM_GetTitleList(&titles_read,MEDIATYPE_NAND,title_count,titleID);
		if (R_FAILED(res)) {
			printf("failed to get title ids.\n");
		}else{
			printf("Found %ld titles\n",titles_read);
			title_count=0;
			for (int i=0;i<titles_read;i++) {
				u16 uCategory = (u16)((titleID[i] >> 32) & 0xFFFF);
				if (uCategory==0x8004 || uCategory==0x8005 || uCategory==0x800F || uCategory==0x8015 ) {
					printf("Found %016llx...",titleID[i]);
					if (!isBlacklisted(titleID[i])) {
						printf("Added\n");
						dsiTitle[title_count] = titleID[i];
						title_count+=1;
					}else{
						printf("System App\n");
					}
				}			
			}
		}
		free(titleID);
	}

	int dumpCount = 0;
	for (int i=0;i<title_count;i++) {
		if ((dsiTitle[i] & 0xFFFFFFFF) == 0) {
			continue;
		}
		printf("Attempting to delete %08lx: ",(u32)(dsiTitle[i] & 0xFFFFFFFF));
		res = uninstall(dsiTitle[i]);
		if (R_SUCCEEDED(res)) {
			printf("Success\n");
			dumpCount++;
		}else{
			printf("Failed\n");
		}
	}
	printf("Deleted %d DSiWare titles.\n",dumpCount);
	wait();
	
    free(buf);

	amExit();
	gfxExit();
	return 0;
}