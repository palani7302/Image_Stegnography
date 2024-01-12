#include <stdio.h>
#include <string.h>
#include "encode.h"
#include "types.h"
#include "common.h"

/* Function Definitions */

/* Get image size
 * Input: Image file ptr
 * Output: width * height * bytes per pixel (3 in our case)
 * Description: In BMP Image, width is stored in offset 18,
 * and height after that. size is 4 bytes
 */
uint get_image_size_for_bmp(FILE *fptr_image)
{
	uint width, height;
	// Seek to 18th byte
	fseek(fptr_image, 18, SEEK_SET);

	// Read the width (an int)
	fread(&width, sizeof(int), 1, fptr_image);
	printf("width = %u\n", width);

	// Read the height (an int)
	fread(&height, sizeof(int), 1, fptr_image);
	printf("height = %u\n", height);

	// Return image capacity
	return width * height * 3;
}

/* 
 * Get File pointers for i/p and o/p files
 * Inputs: Src Image file, Secret file and
 * Stego Image file
 * Output: FILE pointer for above files
 * Return Value: e_success or e_failure, on file errors
 */
Status open_files(EncodeInfo *encInfo)
{
	// Src Image file
	encInfo->fptr_src_image = fopen(encInfo->src_image_fname, "r");
	// Do Error handling
	if (encInfo->fptr_src_image == NULL)
	{
		perror("fopen");
		fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->src_image_fname);

		return e_failure;
	}

	// Secret file
	encInfo->fptr_secret = fopen(encInfo->secret_fname, "r");
	// Do Error handling
	if (encInfo->fptr_secret == NULL)
	{
		perror("fopen");
		fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->secret_fname);

		return e_failure;
	}

	// Stego Image file
	encInfo->fptr_stego_image = fopen(encInfo->stego_image_fname, "w");
	// Do Error handling
	if (encInfo->fptr_stego_image == NULL)
	{
		perror("fopen");
		fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->stego_image_fname);

		return e_failure;
	}

	// No failure return e_success
	return e_success;
}

//Getting file size
uint get_file_size(FILE *fptr)
{
	fseek(fptr, 0, SEEK_END);

	int size = ftell(fptr);

	fseek(fptr, 0, SEEK_SET);

	return size;
}

//Reading Validating the arguments for encoding operation
Status read_and_validate_encode_args(char *argv[], EncodeInfo *encInfo)
{
	//checking the source file
	if ( strcmp(strstr(argv[2], "."), ".bmp") == 0 )
	{
		encInfo->src_image_fname = argv[2];
	}
	else
	{
		return e_failure;
	}	

	//checking the secret file for multiple file types
	int count = 0, i = 0;
	char *extn[] = {".txt", ".c", ".h", ".sh"};
	for ( i = 0; i < 4; i++ )
	{
		if ( strcmp(strstr(argv[3], "."), extn[i]) == 0 )
		{
			count++;
			break;
		}
	}
	if ( count == 1 )
	{
		encInfo->secret_fname = argv[3];
	}
	else
	{
		return e_failure;
	}	

	//checking the output/destination file, which is optional
	if ( argv[4] == NULL )
	{
		encInfo->stego_image_fname = "stego.bmp";
	}
	else
	{
		if ( strcmp(strstr(argv[4], "."), ".bmp") == 0 )
		{
			encInfo->stego_image_fname = argv[4];
		}
		else
		{
			return e_failure;
		}
	}	

	//no failure, return e_success
	return e_success;
}

/*
 * Checking the image capacity, whether the image is 
 * capable to store the secret information
 */
Status check_capacity(EncodeInfo *encInfo)
{
	//obtaining the extension of the secret file
	char *extn = strstr(encInfo->secret_fname, ".");
	int i = 0;

	//storing the extension of the secret file to the structure member
	while ( extn[i] )
	{
		encInfo->extn_secret_file[i] = extn[i];
		i++;
	}

	//obtaining the size of the secret file
	encInfo->size_secret_file = get_file_size(encInfo->fptr_secret);

	//checking the capacity
	if ( (get_file_size(encInfo->fptr_src_image) - 54) >= ((strlen(MAGIC_STRING) + 4 + strlen(encInfo->extn_secret_file) + 4 + encInfo->size_secret_file) * 8) )
	{
		return e_success;
	}

	//No success, return e_failure
	return e_failure;
}

//Copying the header info from the source to destination
Status copy_bmp_header(FILE *fptr_src_image, FILE *fptr_stego_image)
{
	char header[54];

	fseek(fptr_src_image,0,SEEK_SET);
	fseek(fptr_stego_image,0,SEEK_SET);

	fread(header, 1, 54, fptr_src_image);
	fwrite(header, 1, 54, fptr_stego_image);

	return e_success;	
}

//Encoding 1 byte of data to the 8 bytes of data in the image
Status encode_byte_to_lsb(char data, char *image_buffer)
{
	for ( int i = 0; i < MAX_IMAGE_BUF_SIZE; i++ )
	{
		image_buffer[i] = ((unsigned)(data & (1 << i)) >> i) | (image_buffer[i] & 0xFE);
	}
}

//Encoding a string data or an array of characters
Status encode_data_to_image(const char *data, int size, FILE *fptr_src_image, FILE *fptr_stego_image, EncodeInfo *encInfo)
{
	for ( int i = 0; i < size; i++ )
	{
		fread(encInfo->image_data, 1, MAX_IMAGE_BUF_SIZE, encInfo->fptr_src_image);

		encode_byte_to_lsb(data[i], encInfo->image_data);

		fwrite(encInfo->image_data, 1, MAX_IMAGE_BUF_SIZE, encInfo->fptr_stego_image);
	}
}

/* 
 * Encoding the magic string, which is used to identify
 * whether the image is stegged or not while decoding
 */
Status encode_magic_string(const char *magic_string, EncodeInfo *encInfo)
{
	encode_data_to_image(magic_string, strlen(magic_string), encInfo->fptr_src_image, encInfo->fptr_stego_image, encInfo);

	return e_success;
}

//Encoding the size of the secret file extension
Status encode_secret_file_extn_size(long file_size, EncodeInfo *encInfo)
{
	char size_buff[32];

	fread(size_buff, 1, 32, encInfo->fptr_src_image);

	for ( int i = 0; i < 32; i++ )
	{
		size_buff[i] = ((unsigned)(file_size & (1 << i)) >> i) | (size_buff[i] & 0xFE);
	}

	fwrite(size_buff, 1, 32, encInfo->fptr_stego_image);

	return e_success;
}

//Encoding the secret file extension
Status encode_secret_file_extn(const char *file_extn, EncodeInfo *encInfo)
{
	encode_data_to_image(file_extn, strlen(file_extn), encInfo->fptr_src_image, encInfo->fptr_stego_image, encInfo);

	return e_success;
}

//Encoding the size of the secret file
Status encode_secret_file_size(long file_size, EncodeInfo *encInfo)
{
	encode_secret_file_extn_size(file_size, encInfo);

	return e_success;
}

//Encoding the secret file data
Status encode_secret_file_data(EncodeInfo *encInfo)
{
	fseek(encInfo->fptr_secret, 0, SEEK_SET);

	char secret_buff[encInfo->size_secret_file];

	fread(secret_buff, 1, encInfo->size_secret_file, encInfo->fptr_secret);

	encode_data_to_image(secret_buff, encInfo->size_secret_file, encInfo->fptr_src_image, encInfo->fptr_stego_image, encInfo);

	return e_success;
}

//Copying the remaining data of the source file to the destination file
Status copy_remaining_img_data(FILE *fptr_src_image, FILE *fptr_stego_image)
{

	char ch[1] = {'0'};

	while ( fread(ch, 1, 1, fptr_src_image) != 0 )
	{
		fwrite(ch, 1, 1, fptr_stego_image);
	}

	return e_success;
}

//Initialization of Encoding operation
Status do_encoding(EncodeInfo *encInfo)
{
	/* 
	 * Calling all the required functions for encoding
	 * and returning a value to the main function 
	 * based on the return values of each functions
	 */

	//opening files
	if ( open_files(encInfo) == e_success )
	{
		printf("INFO : Opening files success\n");
	}
	else
	{
		printf("INFO : Opening files failure\n");
		return e_failure;
	}

	if ( check_capacity(encInfo) == e_success )
	{
		printf("INFO : Check capacity success\n");
	}
	else
	{
		printf("INFO : Check capacity failure\n");
		return e_failure;
	}

	if ( copy_bmp_header(encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_success )
	{
		printf("INFO : Copying BMP header success\n");
	}
	else
	{
		printf("INFO : Copying BMP header failure\n");
		return e_failure;
	}

	if ( encode_magic_string(MAGIC_STRING, encInfo) == e_success )
	{
		printf("INFO : Magic string encoding success\n");
	}
	else
	{
		printf("INFO : Magic string encoding failure\n");
		return e_failure;
	}

	if ( encode_secret_file_extn_size(strlen(encInfo->extn_secret_file), encInfo) == e_success )
	{
		printf("INFO : Secret file entension size encoding success\n");
	}
	else
	{
		printf("INFO : Secret file extension size encoding failure\n");
		return e_failure;
	}

	if ( encode_secret_file_extn(encInfo->extn_secret_file, encInfo) == e_success )
	{
		printf("INFO : Secret file extension encoding success\n");
	}
	else
	{
		printf("INFO : Secret file extension encoding failure\n");
		return e_failure;
	}

	if ( encode_secret_file_size(encInfo->size_secret_file, encInfo) == e_success )
	{
		printf("INFO : Secret file size encoding success\n");
	}
	else
	{
		printf("INFO : Secret file size encoding failure\n");
		return e_failure;
	}

	if ( encode_secret_file_data(encInfo) == e_success )
	{
		printf("INFO : Secret file data encoding success\n");
	}
	else
	{
		printf("INFO : Secret file data encoding failure\n");
		return e_failure;
	}

	if ( copy_remaining_img_data(encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_success )
	{
		printf("INFO : Copying remaining data success\n");
	}
	else
	{
		printf("INFO : Copying remaining data failure\n");
		return e_failure;
	}

	return e_success;
}
