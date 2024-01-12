#include <stdio.h>
#include <string.h>
#include "decode.h"
#include "types.h"
#include "common.h"

/* Function Definitions */

/* 
 * Get File pointers for i/p and o/p files
 * Inputs: Src Image file, Secret file and
 * Stego Image file
 * Output: FILE pointer for above files
 * Return Value: e_success or e_failure, on file errors
 */
Status open_files_dec(DecodeInfo *decInfo)
{
	// Enc Image file
	decInfo->fptr_enc_image = fopen(decInfo->enc_image_fname, "r");
	// Do Error handling
	if (decInfo->fptr_enc_image == NULL)
	{
		perror("fopen");
		fprintf(stderr, "ERROR: Unable to open file %s\n", decInfo->enc_image_fname);

		return e_failure;
	}

	// No failure return e_success
	return e_success;
}

//Reading and Validating the arguments for Decoding
Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo)
{
	//Checking the encoded file
	if ( strcmp(strstr(argv[2], "."), ".bmp") == 0 )
	{
		decInfo->enc_image_fname = argv[2];
	}
	else
	{
		return e_failure;
	}	

	//Checking the output/destinatio file, which is optional
	if ( argv[3] == NULL )
	{
		decInfo->secret_fname = "out_secret";
	}
	else
	{
		decInfo->secret_fname = argv[3];
	}	

	//No, failure, return e_success
	return e_success;
}

//Decoding 8 bytes from the encoded file to 1 byte of data
char decode_byte_to_lsb(char *image_buffer)
{
	char data = 0, tem = 0;
	for ( int i = 0; i < MAX_IMAGE_BUF_SIZE; i++ )
	{
		tem = image_buffer[i] & 0x01;
		data = data | (tem << i);
	}

	return data;
}

//Decoding the image data to the secret message
Status decode_image_to_data(int size, FILE *fptr_enc_image, FILE *fptr_secret, DecodeInfo *decInfo)
{
	int i = 0;
	char secret_data[size];
	for ( i = 0; i < size; i++ )
	{
		fread(decInfo->image_data, 1, MAX_IMAGE_BUF_SIZE, decInfo->fptr_enc_image);
		secret_data[i] = decode_byte_to_lsb(decInfo->image_data);
	}
	fwrite(secret_data, 1, size, decInfo->fptr_secret);
}

/* 
 * Decoding the magic string, for checking whether 
 * the input file contains the secret message or not
 */
Status decode_magic_string(DecodeInfo *decInfo)
{
	int i = 0;
	for ( i = 0; i < strlen(MAGIC_STRING); i++ )
	{
		fread(decInfo->image_data, 1, 8, decInfo->fptr_enc_image);
		decInfo->magic_string[i] = decode_byte_to_lsb(decInfo->image_data);
	}

	decInfo->magic_string[i] = '\0';
	return e_success;
}

//Decoding the size of the secret file extension
Status decode_secret_file_extn_size(DecodeInfo *decInfo)
{
	char size_buff[32];
	int data = 0, tem = 0;

	fread(size_buff, 1, 32, decInfo->fptr_enc_image);

	for ( int i = 0; i < 32; i++ )
	{
		tem = size_buff[i] & 0x01;
		data = data | (tem << i);
	}

	decInfo->extn_size_secret_file = data;
	return e_success;
}

//Decoding the secret file extension
Status decode_secret_file_extn(DecodeInfo *decInfo)
{
	int i = 0;
	for ( i = 0; i < decInfo->extn_size_secret_file; i++ )
	{
		fread(decInfo->image_data, 1, 8, decInfo->fptr_enc_image);
		decInfo->extn_secret_file[i] = decode_byte_to_lsb(decInfo->image_data);
	}
	decInfo->extn_secret_file[i] = '\0';

	return e_success;
}

//Decoding the size of the secret file
Status decode_secret_file_size(DecodeInfo *decInfo)
{
	char size_buff[32];
	int data = 0, tem = 0;

	fread(size_buff, 1, 32, decInfo->fptr_enc_image);

	for ( int i = 0; i < 32; i++ )
	{
		tem = size_buff[i] & 0x01;
		data = data | (tem << i);
	}
	decInfo->size_secret_file = data;

	return e_success;
}

//Decoding the secret data from the image data
//and writing it to the separate file of specified extension
Status decode_secret_file_data(DecodeInfo *decInfo)
{
	fseek(decInfo->fptr_secret, 0, SEEK_SET);

	decode_image_to_data(decInfo->size_secret_file, decInfo->fptr_enc_image, decInfo->fptr_secret, decInfo);

	return e_success;
}

/* Initializing the Decoding opreation */
Status do_decoding(DecodeInfo *decInfo)
{
	/*
	 * Calling all the required functions for decoding
	 * and returning a value to the main function
	 * baesd on the return value of each functions
	 */

	//opening files
	if ( open_files_dec(decInfo) == e_success )
	{
		printf("INFO : Opening files success\n");
	}
	else
	{
		printf("INFO : Opening files failure\n");
		return e_failure;
	}
	
	//Skipping the header information, which is of 54 bytes
	fseek(decInfo->fptr_enc_image, 54, SEEK_SET); 

	if ( decode_magic_string(decInfo) == e_success )
	{
		printf("INFO : Magic string decoding success\n");
	}
	else
	{
		printf("INFO : Magic string decoding failure\n");
		return e_failure;
	}

	//Comparing the decoded magic string and the encoded magic string
	//If both ar equal, the decoding operation will continue further
	if ( strcmp(decInfo->magic_string, MAGIC_STRING) == 0 )
	{
		if ( decode_secret_file_extn_size(decInfo) == e_success )
		{
			printf("INFO : Secret file entension size decoding success\n");
		}
		else
		{
			printf("INFO : Secret file extension size decoding failure\n");
			return e_failure;
		}

		if ( decode_secret_file_extn(decInfo) == e_success )
		{
			printf("INFO : Secret file extension decoding success\n");
		}
		else
		{
			printf("INFO : Secret file extension decoding failure\n");
			return e_failure;
		}

		//Generating the output file name based on the 
		//encoded extension and opening the file
		{
			char output_name[25];
			int i = 0;
			while ( decInfo->secret_fname[i] )
			{
				output_name[i] = decInfo->secret_fname[i];
				i++;
			}
			output_name[i] = '\0';
			strcat(output_name, decInfo->extn_secret_file);

			decInfo->fptr_secret = fopen(output_name, "w");
		}

		if ( decode_secret_file_size(decInfo) == e_success )
		{
			printf("INFO : Secret file size decoding success\n");
		}
		else
		{
			printf("INFO : Secret file size decoding failure\n");
			return e_failure;
		}

		if ( decode_secret_file_data(decInfo) == e_success )
		{
			printf("INFO : Secret file data decoding success\n");
		}
		else
		{
			printf("INFO : Secret file data decoding failure\n");
			return e_failure;
		}
	}

	//Error, if both magic strings are mismatched
	else
	{
		printf("INFO : Key not matching\n");
		return e_failure;
	}

	return e_success;
}
