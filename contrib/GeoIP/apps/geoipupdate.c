/* geoipupdate.c
 *
 * Copyright (C) 2006 MaxMind LLC
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "GeoIP.h"
#include "GeoIPUpdate.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef __linux__
#include <getopt.h>
#endif
#include <ctype.h>

#define PRODUCT_ID_TOKEN "ProductIds"
#define USER_ID_TOKEN "UserId"
#define LICENSE_KEY_TOKEN "LicenseKey"
#define LICENSE_KEY_LENGTH 12

const char *GeoIPConfFile = "GeoIP.conf";

void usage() {
  fprintf(stderr,"Usage: geoipupdate [-hv] [-f license_file] [-d custom directory]\n");
}

void my_printf(char * str) {
	printf("%s", str);
}

void print_status (int err, char * license_file) {
	if (err == GEOIP_NO_NEW_UPDATES) {
		fprintf(stdout,"GeoIP Database up to date\n");
	} else if (err == GEOIP_LICENSE_KEY_INVALID_ERR) {
		fprintf(stderr,"Invalid License Key in %s - Please visit http://www.maxmind.com/app/products for a subscription\n",license_file);
	} else if (err == GEOIP_USER_ID_INVALID_ERR){
		fprintf(stderr,"Invalid UserID\n");
	} else if (err == GEOIP_PRODUCT_ID_INVALID_ERR){
		fprintf(stderr,"Invalid product ID or subscription expired\n");
	} else if (err < 0) {
		fprintf(stderr,"Received Error %d (%s) when attempting to update GeoIP Database\n",err, GeoIP_get_error_message(err));
	} else {
		fprintf(stdout,"Updated database\n");
	}
}

int main (int argc, char *argv[]) {
  int verbose = 0;
  char * license_file = NULL;
	FILE * license_fh;
	int n = 40;
	int line_index = 0;
	unsigned char *lineptr = malloc(sizeof(char) * n);
	char *a_license_key_str, *a_ptr;
	char *the_license_key_str = "";
  char * the_reference_empty_license_key_str = the_license_key_str;
	char *a_user_id_str = NULL;
	/* the string that holds the user id */
	char *the_user_id_str = NULL;
	/* the integer that holds the length of the string the_user_id_str */
	int the_user_id_strl = 0;
	/* the integer that holds the alloc length of the string the_user_id_str */
	int the_user_id_stral = 0;
	char *a_product_id_str = NULL;
	char **the_product_id_str = NULL;
	int *the_product_id_strl = NULL;
	int *the_product_id_stral = NULL;
	int num_product_ids = 0;
	char * client_ipaddr = NULL;
	char * custom_directory = NULL;
	int c;
	int err = 0;
	int i;

	opterr = 0;

	while ((c = getopt (argc, argv, "hvf:d:")) != -1)
    switch (c) {
		case 'h':
			usage();
			exit(0);
		case 'v':
			verbose = 1;
                        break;
		case 'f':
			license_file = optarg;
			break;
		case 'd':
			custom_directory = optarg;
			break;
		case '?':
			if (isprint (optopt))
				fprintf (stderr, "Unknown option `-%c'.\n", optopt);
			else
				fprintf (stderr,
								 "Unknown option character `\\x%x'.\n",
								 optopt);
			usage();
			exit(1);
		default:
			abort();
		}

	if (custom_directory != NULL) {
		GeoIP_setup_custom_directory(custom_directory);
	}
	if (license_file == NULL) {
		license_file = malloc(sizeof(char) * (strlen(SYSCONFDIR)+strlen(GeoIPConfFile)+2));
		license_file[0] = '\0';
		strcat(license_file, SYSCONFDIR);
		strcat(license_file, "/");
		strcat(license_file, GeoIPConfFile);
	}

  license_fh = fopen(license_file,"r");
	if (license_fh == NULL) {
		fprintf(stderr,"Error opening GeoIP Configuration file %s\n",license_file);
		exit(1);
	}

	if (verbose == 1)
		printf("Opened License file %s\n", license_file);

	do {
		c = fgetc(license_fh);
		if (line_index >= n) {
			n += 20;
			lineptr = realloc(lineptr, n);
		}
		if (c == 10 || c == EOF) {
			lineptr[line_index++] = '\0';
			line_index = 0;
			if (lineptr[0] == '#')
				continue;
			/* get the product ids from the config file */
			a_product_id_str = strstr((char *)lineptr, PRODUCT_ID_TOKEN);//search for a product id token in the line
			if (a_product_id_str != NULL) {
				a_ptr = a_product_id_str;
				/* set pos at the end of product id token */
				a_ptr += strlen(PRODUCT_ID_TOKEN) + 1;
				while (a_ptr[0] == ' ') {
					/* skip spaces */
					a_ptr++;
				}
				/* alloc the array of product ids */
				the_product_id_str = (char **) malloc((num_product_ids+1) * sizeof(char*)); /* array of strings */
				the_product_id_strl = (int *) malloc((num_product_ids+1) * sizeof(char*));  /* array of string lengths */
				the_product_id_stral = (int *) malloc((num_product_ids+1) * sizeof(char*)); /* array of string alloc lengths */
				while (a_ptr[0] != '\0') {
					/* add new product id to the array of product ids */
					the_product_id_str[num_product_ids] = (char *) malloc(20); /* the string */
					the_product_id_strl[num_product_ids] = 0;                  /* the length of the string */
					the_product_id_stral[num_product_ids] = 20;                /* the alloc length of the string */
					while ((a_ptr[0] != ' ') & (a_ptr[0] != '\0')) {
						if (the_product_id_strl[num_product_ids] >= the_product_id_stral[num_product_ids]) {
							/* if the length of the string is equal or more than 
							 * alloc length of the string then realloc the string and
							 * increase the alloc length by 20 */
							the_product_id_stral[num_product_ids] = the_product_id_stral[num_product_ids] + 20;
							the_product_id_str[num_product_ids] = (char *) realloc(the_product_id_str[num_product_ids],the_product_id_stral[num_product_ids]+4);
						}
						/* read the product id from the line in the config file */
						the_product_id_str[num_product_ids][the_product_id_strl[num_product_ids]] = a_ptr[0];
						the_product_id_strl[num_product_ids]++;
						a_ptr++;
					}
				        the_product_id_str[num_product_ids][the_product_id_strl[num_product_ids]] = 0;
					while ((a_ptr[0] == ' ') & (a_ptr[0] != '\0')) {
						a_ptr++;//skip spaces
					}
					/* new product id add, realloc the arrays */
					num_product_ids = num_product_ids + 1;
					/* array of string */
					the_product_id_str = (char **) realloc(the_product_id_str,(num_product_ids+1) * sizeof(char*));
					/* array of string lengths */
					the_product_id_strl = (int *) realloc(the_product_id_strl,(num_product_ids+1) * sizeof(char*));
					/* array of string alloc lengths */
					the_product_id_stral = (int *) realloc(the_product_id_stral,(num_product_ids+1) * sizeof(char*));
				}
			}

			/* get the user id from the config file */
			a_user_id_str = strstr((char *)lineptr, USER_ID_TOKEN); /* search for a user id token in the line */
			if (a_user_id_str != NULL) {
				a_ptr = a_user_id_str;
				/* set the position at the end of user id token */
				a_ptr += strlen(USER_ID_TOKEN) + 1;
				while (a_ptr[0] == ' ') {
					/* skip spaces */
					a_ptr++;
				}
				/* get the string that has the user id */
				the_user_id_stral = 20;
				the_user_id_str = (char *)malloc(the_user_id_stral);
				/* loop while the chars are numbers */
				while ((a_ptr[0] >= '0') & (a_ptr[0] <= '9')) {
					the_user_id_str[the_user_id_strl++] = a_ptr[0];
					a_ptr++;
					if (the_user_id_strl >= the_user_id_stral) {
						/* if the length of user id string is greater or equal to
						 * the alloc length of user id string then
						 * add 20 to the alloc length and realloc the user id string */
						the_user_id_stral += 20;
						the_user_id_str = realloc(the_user_id_str,the_user_id_stral);
					}
				}
				the_user_id_str[the_user_id_strl] = 0; /* add NUL char */
			}
			a_license_key_str = strstr((char *)lineptr, LICENSE_KEY_TOKEN);
			if (a_license_key_str != NULL) {
				a_ptr = a_license_key_str;
				a_ptr += strlen(LICENSE_KEY_TOKEN) + 1;
				while (a_ptr[0] == ' ') {
					a_ptr++;
				}
				the_license_key_str = malloc(sizeof(char) * (LICENSE_KEY_LENGTH + 1));
				strncpy(the_license_key_str, a_ptr, LICENSE_KEY_LENGTH);
				the_license_key_str[LICENSE_KEY_LENGTH] = '\0';
			}
		} else {
			lineptr[line_index++] = c;
		}
	} while (c != EOF);

	free(lineptr);

	fclose(license_fh);

	if (verbose == 1) {
		printf("Read in license key %s\n", the_license_key_str);
		printf("number of product ids %d \n",num_product_ids);
	}

	if (the_user_id_str != NULL) {
		/* update the databases using the user id string, the license key string and the product id for each database */
		client_ipaddr = NULL;
		for (i = 0; i < num_product_ids; i++) {
			err = GeoIP_update_database_general(the_user_id_str, the_license_key_str, the_product_id_str[i], verbose,&client_ipaddr, &my_printf);
			print_status(err, license_file);
		}
	} else {
		/* Old format with just license key for MaxMind GeoIP Country database updates 
		 * here for backwards compatibility */
		err = GeoIP_update_database(the_license_key_str, verbose, &my_printf);
		print_status(err, license_file);
	}

	if (the_product_id_str != NULL) {
		/* free the product ids */
		for (i = 0; i < num_product_ids; i++ ) {
			free(the_product_id_str[i]);
		}
		free(the_product_id_str);
		free(the_product_id_strl);
		free(the_product_id_stral);
	}

  if ( the_reference_empty_license_key_str != the_license_key_str )
    free(the_license_key_str);

	if (the_user_id_str)
		free(the_user_id_str);

	if (client_ipaddr) {
		free(client_ipaddr);
	}
	exit(err);
}
