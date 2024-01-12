#include <stdio.h>
#include <string.h>
#include "encode.h"
#include "decode.h"
#include "types.h"

int main(int argc, char *argv[])
{
	//declaring structure variables
	EncodeInfo encInfo;
	DecodeInfo decInfo;

	//checking for the required arguments
	if ( argc < 2 )
	{
		printf("INFO : Please pass arguments\n");
	}
	else
	{
		//checking for the operation type(encoding or decoding)
		if ( check_operation_type(argv) == e_encode )
		{
			printf("Selected Encoding\n");

			if ( argc >= 4 && argc <= 5 )
			{
				//reading and validating the arguments for encoding
				if ( read_and_validate_encode_args(argv, &encInfo) == e_success )
				{
					printf("INFO : Read and validate Success\n");
				}
				else
				{	
					printf("INFO : Read and validate Failure\n");
					return e_failure;
				}

				//starting the encoding process
				if ( do_encoding(&encInfo) == e_success )
				{
					printf("INFO : Encoding Success\n");
				}
				else
				{
					printf("INFO : Encoding Failure\n");
					return e_failure;
				}
			}
			else
			{
				printf("INFO : Arguments missing for encoding\n");
			}
		}

		//checking for the operation type(encoding or decoding)
		else if ( check_operation_type(argv) == e_decode )
		{
			printf("Selected Decoding\n");

			if ( argc >= 3 && argc <= 4 )
			{
				//reading and validating the arguments for encoding
				if ( read_and_validate_decode_args(argv, &decInfo) == e_success )
				{
					printf("INFO : Read and validate Success\n");
				}
				else
				{	
					printf("INFO : Read and validate Failure\n");
					return e_failure;
				}

				//starting the encoding process
				if ( do_decoding(&decInfo) == e_success )
				{
					printf("INFO : Decoding Success\n");
				}
				else
				{
					printf("INFO : Decoding Failure\n");
					return e_failure;
				}
			}
			else
			{
				printf("INFO : Arguments missing for encoding\n");
			}
		}

		//error message for invalid operation type
		else
		{
			printf("Unsupported\n");
		}
	}
	return 0;
}

//function definition for checking the operation type
OperationType check_operation_type(char *argv[])
{
	if ( strcmp(argv[1], "-e") == 0 )
	{
		return e_encode;
	}
	else if ( strcmp(argv[1], "-d") == 0 )
	{
		return e_decode;
	}
	else
	{
		return e_unsupported;
	}
}
