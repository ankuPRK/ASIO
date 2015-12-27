#include<stdio.h>

#define NUM 1024

//structure to read wav file//
struct read_wav{
	FILE *Fp;
	int Size = 0;
	int FormatLength = 0;
	int FormatTag = 0;
	int Channels = 0;
	int SampleRate = 0;
	int AvgBytesSec = 0;
	int BlockAlign = 2;
	int BitsPerSample = 16;
	int DataSize = 0;
	char id[5];
	short KiloBuffer[NUM];
	bool DataComing = true;

	/*This function takes as input a wav file name 
	  and loads it into the readwav structure.
	  The function will return TRUE 
	  if it successfully loads the wav file,
	  otherwise it will return FALSE.
	*/
	bool read(char fName[]){

		Fp = fopen(fName, "rb");
		if (Fp == NULL)
			return false;
		id[4] = '\0';
		fread(id, sizeof(char), 4, Fp);
		if (strcmp("RIFF", id))
			return false;
		fread(&Size, sizeof(unsigned int), 1, Fp);
		fread(id, sizeof(char), 4, Fp);
		if (strcmp("WAVE", id))
			return false;
		fread(id, sizeof(char), 4, Fp);
		if (strcmp("fmt ", id))
			return false;
		fread(&FormatLength, sizeof(unsigned int), 1, Fp);
		fread(&FormatTag, sizeof(unsigned short), 1, Fp);
		fread(&Channels, sizeof(unsigned short), 1, Fp);		//1 for Mono 2 for Stereo
		fread(&SampleRate, sizeof(unsigned int), 1, Fp);
		fread(&AvgBytesSec, sizeof(unsigned int), 1, Fp);
		fread(&BlockAlign, sizeof(unsigned short), 1, Fp);
		fread(&BitsPerSample, sizeof(unsigned short), 1, Fp);
		fread(id, sizeof(char), 4, Fp);
		if (strcmp("data", id))
			return false;
		fread(&DataSize, sizeof(unsigned int), 1, Fp);
		DataComing = true;
		return true;
	}
}read_wav;

/**2. This function takes as input a *.wav file name,
	  its sampling frequency, no of channels, bits per sample and
	  the duration of audio file in milliseconds.
	  It then writes initial 40 Bytes of wav head data into a binary-created file
	  and returns the file pointer. 
	  The user can then write the sound buffer into the wav file 
	  using the file pointer.
*/
FILE *wav_write(char fname[], int SampleRate, int Channels, int BitsPerSample, int MilliSeconds){

		FILE *Fp = fopen(fname, "wb");

		int Size = (MilliSeconds*SampleRate) / 1000 + 40;				//40 + total bytes in buffer 
		int FormatLength = BitsPerSample;		//format length = bitspersample for uncompressed wav files	
		int FormatTag = 1;			//format tag; default value is 1
		int AvgBytesSec = 44100 * (BitsPerSample / 8)*Channels;		//samplerate*channels*bytespersample
		int BlockAlign = Channels;
		int DataSize = Size - 40;		//total bytes in buffer
		char *id;
		short KiloBuffer[NUM];

		id = "RIFF";
		fwrite(id, sizeof(char), 4, Fp);
		fwrite(&Size, sizeof(unsigned int), 1, Fp);
		id = "WAVE";
		fwrite(id, sizeof(char), 4, Fp);
		id = "fmt ";
		fwrite(id, sizeof(char), 4, Fp);
		fwrite(&FormatLength, sizeof(unsigned int), 1, Fp);
		fwrite(&FormatTag, sizeof(unsigned short), 1, Fp);
		fwrite(&Channels, sizeof(unsigned short), 1, Fp);		//1 for Mono 2 for Stereo
		fwrite(&SampleRate, sizeof(unsigned int), 1, Fp);
		fwrite(&AvgBytesSec, sizeof(unsigned int), 1, Fp);
		fwrite(&BlockAlign, sizeof(unsigned short), 1, Fp);
		fwrite(&BitsPerSample, sizeof(unsigned short), 1, Fp);
		id = "data";
		fwrite(id, sizeof(char), 4, Fp);
		fwrite(&DataSize, sizeof(unsigned int), 1, Fp);
		return Fp;
	}
