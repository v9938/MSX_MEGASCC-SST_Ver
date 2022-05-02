#include <stdio.h>
#include <msx.h>
#include <sys/ioctl.h>

#define VERSION "V1.0"
#define DATE "2022/05"

#define BUF_SIZE 128

//#include <msx/gfx.h>

unsigned char SelectSlot;		//Select Slot number
unsigned char eseBank;			//eseSCC 6000-7fffh Bank number
unsigned char *addressWrite;	//Write pointer
unsigned char eseSlot;			//eseSCC Slot number
unsigned char maxBank;			//ROM Size
int findId;						//Flash ID


//Assembler SubRoutine
void asmCalls(){
#asm
chengePageSlot:
	di
    ld a,(_SelectSlot)	;Slot Number
    ld hl,04000h		;Select page 1
    call 0024h			;call ENASLT

    ld a,(_SelectSlot)	;Slot Number
    ld hl,08000h		;Select page 2
    call 0024h			;call ENASLT

	ld hl,04AAAh		;Flash Address 1st 0x2AAA
	ld de,06555h		;Flash Address 2nd 0x5555

	ld a,(_eseBank)		;Bank Select A000h-Bfffh
	ld (0B000h),a

	ld a,001h			;
    ld (05000h),a		;Bank 0 Page2

	ld a,006h			;
    ld (07000h),a		;Bank 1 Page5
    
	ret

restoreRAMPage:
	ld a,(0f342h)		;Restore RAM Page (RAMAD1)
    ld hl,04000h		;Select page 1
    call 0024h			;call ENASLT

	ld a,(0f343h)		;Restore RAM Page (RAMAD2)
    ld hl,08000h		;Select page 2
    call 0024h			;call ENASLT
	ei
	ret
#endasm
}
void eraseEseSCC() __z88dk_fastcall __naked {
#asm
	call chengePageSlot
    
	ld a,0aah			; Chip-Erase Command 1st
    ld (de),a			; aa-55-80
	ld a,055h
    ld (hl),a
	ld a,080h
    ld (de),a

	ld a,0aah			; Chip-Erase Command 2nd
    ld (de),a			; aa-55-10
	ld a,055h
    ld (hl),a
	ld a,010h
    ld (de),a

	call restoreRAMPage

    ld hl,0fc9eh		;JIFFY address
	ld a,(hl)			;Load INTCNT
	add 06h				;16.6ms x 6 = 99.6ms
busyWait:
	cp	(hl)			;Check INTCNT
	jr nz,busyWait		;Wait Loop
	ret
#endasm
}

void writeEseSCC(unsigned char *bufferAddress)
{
#asm
	push hl					;Backup Buffer address
	call chengePageSlot		;4000-bfffh Change eseSCC Slot 
	pop hl

	ld de,(_addressWrite)	;Get buffer address
	ld bc,BUF_SIZE			;Set buffer size
writeLoop:
	ld a,001h				;
    ld (05000h),a			;Bank 0 Page5
	ld a,006h				;
    ld (07000h),a			;Bank 1 Page2

	ld a,0aah				; Flash Byte-Program
    ld (06555h),a			; aa-55-a0
	ld a,055h
    ld (04AAAh),a
	ld a,0A0h
    ld (06555h),a
    ld a,(hl)				; Load Buffer Data
    ld (de),a				; Flash program data
	ld a,(_eseBank)			; Reselect Bank A000h-Bfffh
	ld (0B000h),a
writeWait:	
    ld a,(de)				; Verify program data
    cp (hl)					; Verify check
    jr nz,writeWait			; Wait Byte-Program Time
	inc hl					; Buffer address ++
	inc de					; Flash address ++
	dec bc					; Counter --
	ld a,b					; bc == 00?
	or c 
    jr nz,writeLoop			; Continue loop
	ld (_addressWrite),de	
	call restoreRAMPage
	ret
#endasm
}

int chkEseSCC() __z88dk_fastcall __naked {
#asm
	call chengePageSlot	; 4000-bfffh Change eseSCC Slot 

	ld a,0aah			; Software Product ID Entry aa-55-90-read0-1
    ld (de),a
	ld a,055h
    ld (hl),a
	ld a,090h
    ld (de),a
	ld a,(04000h)		; Read Maker ID
    ld b,a
	ld a,(04001h)		; Read Device ID
    ld c,a
	push bc				; Return value

	ld a,001h				;
    ld (05000h),a			;Bank 0 Page5
	ld a,006h				;
    ld (07000h),a			;Bank 1 Page2
	
	ld a,0aah			; Reset Command sequence aa-55-f0
    ld (de),a
	ld a,055h
    ld (hl),a
	ld a,0f0h
    ld (de),a

	call restoreRAMPage	; 4000-bfffh Restore RAM Slot
	pop hl
	ret
#endasm
}

void findEseSCC(void){
	unsigned char i;
	unsigned int chipId;
	
	if ((SelectSlot & 0xf0) == 0x80){
		//Expantion Slot Check
		for (i=0;i<4;i++){
			chipId = chkEseSCC();
			if ((chipId & 0xff00) == 0xbf00) {		//0xBF SST Maker ID
				eseSlot = SelectSlot;
				findId = chipId;
			}
			SelectSlot = SelectSlot + 4;
		}
	}else{
		//Master Slot Check
		chipId = chkEseSCC();
		if ((chipId & 0xff00) == 0xbf00) {			//0xBF SST Maker ID
			eseSlot = SelectSlot;
			findId = chipId;
		}
	}
}
	
int main(int argc,char *argv[])
{

	FILE *fp;
	unsigned char ReadData[BUF_SIZE];
	int i;
	unsigned char dat;
	
	
	printf("MEGA SCC SST Writer %s\n",VERSION);
	printf("Copyrigth %s @v9938\n\n",DATE);

	if (argc<2){
		printf( "sstscc [rom files]\n");
		printf( "This program will write the selected file to the flash ROM.\n");
		return 0;
	}

	printf("Search Flash ... ");
	eseSlot = 0;
	//Slot 1 Search
	SelectSlot = *((unsigned char *)0xfcc2) | 0x01;		//EXPTBL (SLOT1)
	findEseSCC();
	//Slot 2 Search
	SelectSlot = *((unsigned char *)0xfcc3) | 0x02;		//EXPTBL (SLOT2)
	findEseSCC();
	//Slot 3 Search
	SelectSlot = *((unsigned char *)0xfcc4) | 0x03;		//EXPTBL (SLOT3)
	findEseSCC();

	if (eseSlot == 0) {								// Not find eseSCC
		printf("NOT find\n");
		printf("Bye...\n");
		return -1;
	}else{
		printf("Find!\n");
		printf("\nSlot: %02x",eseSlot);
	}

	//Flash Type Check
	printf("\nFlash Type: ");
	if ((findId & 0x00ff) == 0xb5){
		printf("SST39SF010A\n");
		maxBank = 0xf;
	}else if ((findId & 0x00ff) == 0xb6){
		printf("SST39SF020A\n");
		maxBank = 0x1f;
	}else if ((findId & 0x00ff) == 0xb7){
		printf("SST39SF040A\n");
		maxBank = 0x3f;
	}else{
		printf("Unknown Flash\n");
		printf("Bye...\n");
		return -1;
	}

	//ROM File Open
	fp =fopen(argv[1],"rb");
    if( fp == NULL ){
    	printf( "File can't open... %s\n", argv[1]);
    	return -1;
    }

	//Bank set
	SelectSlot = eseSlot;
	eseBank = 0;
	addressWrite = (unsigned char *)0xA000;

	//Flash Erase
	printf("\nFlash Erase ... ");
	eraseEseSCC();
	printf("Done.");

	//Flash Write
	printf("\nFlash Write");
	printf("\n     0123456789ABCDEF");
	printf("\n0x00 .");
	
	while (1){

		fread(ReadData,sizeof ReadData, 1, fp);		//Read Buffer

		if (feof(fp) != NULL) break;				//File End ?
		if (ferror(fp) != NULL) {					//EIO Error?
			printf("File IO Error!\n");
			fclose(fp);
			return -1;
		}

		//Parameter Check
		if (addressWrite >= 0xC000){					// 6000-7fffh Write finsh?
			addressWrite = (unsigned char *)0xA000;		// Yes! Return write start address
			if (eseBank < maxBank) {					// ROM size check?
				eseBank++;								// OK! write next bank
				if ((eseBank & 0x0f)== 0){				// Display progress
					printf("\n0x%02x .",eseBank);		//
				}else{
					printf(".");						// Display progress
				}

			}else{
				printf("\nROM size is FULL.");			// Ummm.ROM is FULL
				break;									//
			}
		}
		writeEseSCC(ReadData);						//Bank Write Data
	}
	fclose(fp);

	dat = 0;

	while(1){
		

		if (addressWrite >= 0xC000){					// 6000-7fffh Write finsh?
			addressWrite = (unsigned char *)0xA000;		// Yes! Return write start address
			if (eseBank < maxBank) {					// ROM size check?
				eseBank++;								// OK! write next bank
				for (i=0;i<BUF_SIZE;i++){
					ReadData[i]=dat;
				}
				dat++;
				if ((eseBank & 0x0f)== 0){				// Display progress
					printf("\n0x%02x -",eseBank);		//
				}else{
					printf("-");						// Display progress
				}

			}else{
				break;
			}
		}
		writeEseSCC(ReadData);						//Bank Write Data
	}
	
	
	printf("\n\nDone. Thank you using!\n");
	return 0;

}
